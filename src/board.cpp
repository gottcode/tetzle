/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "board.h"

#include "overview.h"
#include "piece.h"
#include "solver.h"
#include "tile.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QWheelEvent>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <cmath>
#include <ctime>

#if defined(Q_OS_WIN32)
#include <GL/glext.h>
PFNGLACTIVETEXTUREARBPROC glActiveTexture = 0;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2f = 0;
#endif

/*****************************************************************************/

static int powerOfTwo(int value)
{
	value--;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value++;
	return value;
}

/*****************************************************************************/

Board::Board(QWidget* parent)
:	QGLWidget(parent),
	m_id(0),
	m_difficulty(0),
	m_letterbox(false),
	m_borders_visible(QSettings().value("Board/BordersVisible", true).toBool()),
	m_image_width(0),
	m_image_height(0),
	m_tile_size(0),
	m_bumpmap_size(0),
	m_image(0),
	m_bumpmap(0),
	m_image_ts(0),
	m_bumpmap_ts(0),
	m_total_pieces(0),
	m_completed(0),
	m_pos(0, 0),
	m_scale_level(0),
	m_scale_level_min(0),
	m_scale_level_max(0),
	m_scale(0),
	m_scrolling(false),
	m_selecting(false),
	m_finished(false),
	m_action_key(0),
	m_action_button(Qt::NoButton)
{
	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	setMouseTracking(true);
	glInit();
#if defined(Q_OS_WIN32)
	glActiveTexture = (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress("glActiveTextureARB");
	glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FARBPROC) wglGetProcAddress("glMultiTexCoord2fARB");
#endif
	generateSuccessImage();

	// Create overview dialog
	m_overview = new Overview;
	connect(m_overview, SIGNAL(toggled(bool)), this, SIGNAL(overviewToggled(bool)));
}

/*****************************************************************************/

Board::~Board()
{
	cleanup();
	deleteTexture(m_success);
	delete m_overview;
}

/*****************************************************************************/

void Board::removePiece(Piece* piece)
{
	m_pieces.removeAll(piece);
	delete piece;
	piece = 0;
}

/*****************************************************************************/

QList<Piece*> Board::findCollidingPieces(Piece* piece) const
{
	QList<Piece*> list;
	QRect rect = piece->marginRect();
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		Piece* parent = m_pieces.at(i);
		if (parent != piece && piece->collidesWith(parent))
			list.append(parent);
	}
	return list;
}

/*****************************************************************************/

Piece* Board::findCollidingPiece(Piece* piece) const
{
	QRect rect = piece->marginRect();
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		Piece* parent = m_pieces.at(i);
		if (parent != piece && piece->collidesWith(parent))
			return parent;
	}
	return 0;
}

/*****************************************************************************/

void Board::newGame(const QString& image, int difficulty)
{
	// Remove any previous textures and tiles
	cleanup();

	// Prevent starting a game with a missing image
	if (!QFileInfo("images/" + image).exists()) {
		QMessageBox::warning(this, tr("Error"), tr("Missing image."));
		return;
	}

	// Update player about status
	emit statusMessage("");
	window()->setCursor(Qt::WaitCursor);
	QLabel dialog(tr("Creating puzzle; please wait."), this, Qt::Dialog);
	dialog.setMargin(12);
	dialog.show();
	qApp->processEvents();

	// Generate ID
	m_id = 0;
	int id;
	foreach (QString file, QDir("saves/").entryList(QDir::Files)) {
		id = file.section(".", 0, 0).toInt();
		if (id > m_id)
			m_id = id;
	}
	m_id++;

	// Create textures
	m_letterbox = QSettings().value("NewGame/Letterbox").toBool();
	m_difficulty = difficulty;
	m_image_path = image;
	loadImage();

	// Create tiles
	int columns = m_image_width / m_tile_size / 8 * 8;
	int rows = m_image_height / m_tile_size / 8 * 8;
	QVector< QVector<Tile*> > tiles = QVector< QVector<Tile*> >(columns, QVector<Tile*>(rows));
	for (int c = 0; c < columns; ++c) {
		for (int r = 0; r < rows; ++r) {
			tiles[c][r] = new Tile(c, r, QPoint(m_tile_size * c, m_tile_size * r), this);
		}
	}

	// Create pieces
	int groups_wide = columns / 8;
	int groups_tall = rows / 8;
	srand(time(0));
	Solver solver(8, 8, groups_wide * groups_tall);
	Piece* piece = 0;
	Tile* tile = 0;
	int rotations = 0;
	int full_width = m_image_width * 2;
	int full_height = m_image_height * 2;
	for (int c = 0; c < groups_wide; ++c) {
		for (int r = 0; r < groups_tall; ++r) {
			foreach (const QList<QPoint>& group, solver.solutions[r * groups_wide + c]) {
				tile = tiles.at(group.at(0).x() + (c * 8)).at(group.at(0).y() + (r * 8));

				// Create piece
				piece = new Piece(0, tile->pos(), this);
				m_pieces.append(piece);

				// Add tiles to piece
				tile->setPos(QPoint(0, 0));
				piece->attach(tile);
				for (int i = 1; i < 4; ++i) {
					tile = tiles.at(group.at(i).x() + (c * 8)).at(group.at(i).y() + (r * 8));
					tile->setPos(tile->pos() - piece->scenePos());
					piece->attach(tile);
				}

				// Rotate piece
				rotations = rand() % 4;
				for (int i = 0; i < rotations; ++i)
					piece->rotateAround(piece->children().first());

				// Position piece
				piece->moveBy(QPoint(rand() % full_width, rand() % full_height) - piece->scenePos());
			}
		}
	}

	// Don't cover other pieces
	m_scale = 1;
	foreach (Piece* piece, m_pieces) {
		piece->pushNeighbors();
	}

	// Draw tiles
	window()->unsetCursor();
	zoomFit();
	updateCompleted();
	emit retrievePiecesAvailable(true);
}

/*****************************************************************************/

void Board::openGame(int id)
{
	// Remove any previous textures and tiles
	cleanup();

	// Update player about status
	emit statusMessage("");
	window()->setCursor(Qt::WaitCursor);
	QLabel dialog(tr("Loading puzzle; please wait."), this, Qt::Dialog);
	dialog.setMargin(12);
	dialog.show();
	qApp->processEvents();

	// Open saved game file
	m_id = id;
	QFile file(QString("saves/%1.xml").arg(m_id));
	if (!file.open(QIODevice::ReadOnly))
		return;
	QXmlStreamReader xml(&file);

	// Load textures
	while (!xml.isStartElement())
		xml.readNext();
	QXmlStreamAttributes attributes = xml.attributes();
	int board_zoom = 0;
	unsigned int version = attributes.value("version").toString().toUInt();
	if (xml.name() == QLatin1String("tetzle") && version <= 3) {
		m_image_path = attributes.value("image").toString();
		if (!QFileInfo("images/" + m_image_path).exists()) {
			dialog.hide();
			QMessageBox::warning(this, tr("Error"), tr("Missing image."));
			cleanup();
			return;
		}
		m_difficulty = attributes.value("difficulty").toString().toInt();
		board_zoom = attributes.value("zoom").toString().toInt();
		m_pos.setX(attributes.value("x").toString().toInt());
		m_pos.setY(attributes.value("y").toString().toInt());
		m_letterbox = attributes.value("letterbox").toString().toInt();
		loadImage();
		if (version < 3) {
			float old_scale = (16 * m_difficulty * pow(1.25, board_zoom * 0.5)) / qMax(m_image_width, m_image_height);
			board_zoom = log(old_scale * (m_tile_size * 0.25)) / log(1.25) * 2;
		}
	} else {
		xml.raiseError(tr("Unknown data format"));
	}

	// Load tiles
	Piece* piece = 0;
	Tile* tile = 0;
	QPoint pos;
	int rotation = -1;
	while (!xml.atEnd()) {
		xml.readNext();
		if (!xml.isStartElement())
			continue;
		if (xml.name() == QLatin1String("tile")) {
			attributes = xml.attributes();
			pos = QPoint(attributes.value("x").toString().toInt(), attributes.value("y").toString().toInt());
			if (!piece) {
				piece = new Piece(rotation != -1 ? rotation : attributes.value("rotation").toString().toInt(), pos, this);
				m_pieces.append(piece);
				pos = QPoint(0, 0);
			}
			tile = new Tile(attributes.value("column").toString().toInt(), attributes.value("row").toString().toInt(), pos, this);
			piece->attach(tile);
		} else if (xml.name() == QLatin1String("group")) {
			piece = 0;
			QStringRef r = xml.attributes().value("rotation");
			rotation = !r.isEmpty() ? r.toString().toInt() : -1;
		} else if (xml.name() != QLatin1String("overview")) {
			xml.raiseError(tr("Unknown element '%1'").arg(xml.name().toString()));
		}
	}
	if (xml.hasError()) {
		dialog.hide();
		QMessageBox::warning(this, tr("Error"), tr("Error parsing XML file.\n\n%1").arg(xml.errorString()));
		cleanup();
		return;
	}

	// Draw tiles
	window()->unsetCursor();
	zoom(board_zoom);
	updateCompleted();
	emit retrievePiecesAvailable(true);
}

/*****************************************************************************/

void Board::saveGame()
{
	if (m_pieces.count() <= 1)
		return;

	QFile file(QString("saves/%1.xml").arg(m_id));
	if (!file.open(QIODevice::WriteOnly))
		return;

	QXmlStreamWriter xml(&file);
	xml.setAutoFormatting(true);
	xml.writeStartDocument();

	xml.writeStartElement("tetzle");
	xml.writeAttribute("version", "3");
	xml.writeAttribute("image", m_image_path);
	xml.writeAttribute("difficulty", QString::number(m_difficulty));
	xml.writeAttribute("pieces", QString::number(m_total_pieces));
	xml.writeAttribute("complete", QString::number(m_completed));
	xml.writeAttribute("zoom", QString::number(m_scale_level));
	xml.writeAttribute("x", QString::number(m_pos.x()));
	xml.writeAttribute("y", QString::number(m_pos.y()));
	xml.writeAttribute("letterbox", QString::number(m_letterbox));

	foreach (Piece* piece, m_pieces)
		piece->save(xml);

	xml.writeEndElement();

	xml.writeEndDocument();
}

/*****************************************************************************/

void Board::retrievePieces()
{
	// Make sure all pieces are free
	if (!m_active_tiles.isEmpty()) {
		releasePieces();
	}

	// Move all pieces to center of view
	foreach (Piece* piece, m_pieces) {
		piece->moveBy(m_pos - piece->boundingRect().center());
		piece->pushNeighbors();
	}

	// Update view
	zoomFit();
}

/*****************************************************************************/

void Board::zoomIn()
{
	zoom(m_scale_level + 1);
}

/*****************************************************************************/

void Board::zoomOut()
{
	zoom(m_scale_level - 1);
}

/*****************************************************************************/

void Board::zoomFit()
{
	// Find bounding rectangle
	QRect bounds(0, 0, 0, 0);
	QRect tile_bounds;
	foreach (Piece* piece, m_pieces) {
		bounds = bounds.united(piece->boundingRect());
	}
	m_pos = bounds.center();

	// Find scale level to show bounding rectangle
	QSize bounds_scaled = bounds.size();
	bounds_scaled.scale(size(), Qt::KeepAspectRatio);
	float new_tile_scale = static_cast<float>(bounds_scaled.width()) / static_cast<float>(bounds.width());
	int scale_level = 2 * (log(new_tile_scale * m_tile_size * 0.25) / log(1.25));
	if (scale_level < m_scale_level_min)
		scale_level = m_scale_level_min;
	if (scale_level > m_scale_level_max)
		scale_level = m_scale_level_max;

	zoom(scale_level);
}

/*****************************************************************************/

void Board::zoom(int value)
{
	if (value > m_scale_level_max) {
		value = m_scale_level_max;
	} else if (value < m_scale_level_min) {
		value = m_scale_level_min;
	}

	QPoint old_pos = mapCursorPosition();

	// Calculate new scale value
	m_scale_level = value;
	m_scale = pow(1.25, m_scale_level * 0.5) / (m_tile_size * 0.25);
	m_scale = floor(m_scale * m_tile_size) / m_tile_size;
	if (m_scale > 1.0f)
		m_scale = 1.0f;
	if (m_scale * m_tile_size < 4.0f)
		m_scale = 4.0f / m_tile_size;

	// Create tile bumpmap texture
	m_bumpmap_size = qRound(m_tile_size * m_scale);
	int bumpmap_texture_size = powerOfTwo(m_bumpmap_size);
	m_bumpmap_ts = static_cast<float>(m_bumpmap_size) / static_cast<float>(bumpmap_texture_size);
	QImage bumpmap(bumpmap_texture_size, bumpmap_texture_size, QImage::Format_RGB32);
	{
		QPainter painter(&bumpmap);
		painter.fillRect(bumpmap.rect(), QColor(128,128,128));

		if (m_borders_visible) {
			painter.setPen(QColor(224,224,224));
			painter.drawLine(0, 0, m_bumpmap_size - 1, 0);
			painter.drawLine(0, 1, 0, m_bumpmap_size - 2);
			painter.setPen(QColor(32,32,32));
			painter.drawLine(0, m_bumpmap_size - 1, m_bumpmap_size - 1, m_bumpmap_size - 1);
			painter.drawLine(m_bumpmap_size - 1, 1, m_bumpmap_size - 1, m_bumpmap_size - 2);

			painter.setPen(QColor(160,160,160));
			painter.drawLine(1, 1, m_bumpmap_size - 2, 1);
			painter.drawLine(1, 2, 1, m_bumpmap_size - 3);
			painter.setPen(QColor(96,96,96));
			painter.drawLine(1, m_bumpmap_size - 2, m_bumpmap_size - 2, m_bumpmap_size - 2);
			painter.drawLine(m_bumpmap_size - 2, 2, m_bumpmap_size - 2, m_bumpmap_size - 3);
		}
	}
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	glDeleteTextures(1, &m_bumpmap);
	glGenTextures(1, &m_bumpmap);
	glBindTexture(GL_TEXTURE_2D, m_bumpmap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	QImage buf = convertToGLFormat(bumpmap.mirrored(false, true));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bumpmap_texture_size, bumpmap_texture_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf.bits());

	// Update mouse cursor position
	QPoint new_pos = mapCursorPosition();
	for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
		i.key()->moveBy(new_pos - old_pos);
	}
	updateCursor();

	// Update scene
	updateGL();
	emit zoomChanged(m_scale_level);
	emit zoomOutAvailable(m_scale_level > m_scale_level_min);
	emit zoomInAvailable(m_scale_level < m_scale_level_max);
}

/*****************************************************************************/

void Board::toggleOverview()
{
	bool visible = !m_overview->isVisible();
	m_overview->setVisible(visible);
	QSettings().setValue("Overview/Visible", visible);
}

/*****************************************************************************/

void Board::toggleBorders()
{
	m_borders_visible = !m_borders_visible;
	QSettings().setValue("Board/BordersVisible", m_borders_visible);
	zoom(m_scale_level);
	emit bordersToggled(m_borders_visible);
}

/*****************************************************************************/

void Board::initializeGL()
{
	glEnable(GL_DEPTH_TEST);
	glColor4f(1, 1, 1, 1);
	glClearColor(1, 1, 1, 1);
}

/*****************************************************************************/

void Board::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -2, 3);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/*****************************************************************************/

void Board::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glPushMatrix();

	glTranslatef((width() >> 1) - qRound(m_pos.x() * m_scale), (height() >> 1) - qRound(m_pos.y() * m_scale), 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_image);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED);
	glBindTexture(GL_TEXTURE_2D, m_bumpmap);

	float depth = 0;
	float depth_diff = 1.0f / m_pieces.count();
	glBegin(GL_QUADS);
		foreach (Piece* piece, m_pieces) {
			foreach (Tile* tile, piece->children()) {
				draw(tile, tile->scenePos() * m_scale, depth);
			}
			depth += depth_diff;
		}
	glEnd();

	glPopMatrix();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (m_selecting) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPushAttrib(GL_CURRENT_BIT);

		QRect box = QRect(m_cursor_pos, m_select_pos).normalized();

		glColor4f(0, 0, 1, 0.25);
		glBegin(GL_QUADS);
			glVertex3f(box.x(), box.y(), 1.9);
			glVertex3f(box.x(), box.y() + box.height(), 1.9);
			glVertex3f(box.x() + box.width(), box.y() + box.height(), 1.9);
			glVertex3f(box.x() + box.width(), box.y(), 1.9);
		glEnd();

		glColor3f(0, 0, 1);
		glBegin(GL_LINE_STRIP);
			glVertex3f(box.x(), box.y(), 2);
			glVertex3f(box.x(), box.y() + box.height(), 2);
			glVertex3f(box.x() + box.width(), box.y() + box.height(), 2);
			glVertex3f(box.x() + box.width(), box.y(), 2);
			glVertex3f(box.x(), box.y(), 2);
		glEnd();

		glPopAttrib();
		glDisable(GL_BLEND);
	}

	if (m_finished) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_success);

		int w = powerOfTwo(m_success_size.width());
		int h = powerOfTwo(m_success_size.height());
		int x = (width() >> 1) - (m_success_size.width() >> 1);
		int y = (height() >> 1) - (m_success_size.height() >> 1);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex3f(x, y, 2);

			glTexCoord2f(0, 1);
			glVertex3f(x, y + h, 2);

			glTexCoord2f(1, 1);
			glVertex3f(x + w, y + h, 2);

			glTexCoord2f(1, 0);
			glVertex3f(x + w, y, 2);
		glEnd();

		glDisable(GL_BLEND);
	}
}

/*****************************************************************************/

void Board::keyPressEvent(QKeyEvent* event)
{
	int offset = (event->modifiers() & Qt::ControlModifier) ? 1 : 10;
	switch (event->key()) {
		// Scroll left
		case Qt::Key_Left:
			scroll(QPoint(2 * offset, 0));
			updateGL();
			updateCursor();
			break;
		// Scroll up
		case Qt::Key_Up:
			scroll(QPoint(0, 2 * offset));
			updateGL();
			updateCursor();
			break;
		// Scroll right
		case Qt::Key_Right:
			scroll(QPoint(-2 * offset, 0));
			updateGL();
			updateCursor();
			break;
		// Scroll down
		case Qt::Key_Down:
			scroll(QPoint(0, -2 * offset));
			updateGL();
			updateCursor();
			break;
		// Grab or release piece
		case Qt::Key_Space:
			togglePiecesUnderCursor();
			break;
		// Rotate piece
		case Qt::Key_R:
			rotatePiece();
			break;
		// Move cursor up
		case Qt::Key_W:
			moveCursor(QPoint(0, -offset));
			break;
		// Move cursor left
		case Qt::Key_A:
			moveCursor(QPoint(-offset, 0));
			break;
		// Move cursor down
		case Qt::Key_S:
			moveCursor(QPoint(0, offset));
			break;
		// Move cursor right
		case Qt::Key_D:
			moveCursor(QPoint(offset, 0));
			break;
		default:
			if (!event->isAutoRepeat()) {
				m_action_key = event->key();
			}
	}
	QGLWidget::keyPressEvent(event);
}

/*****************************************************************************/

void Board::keyReleaseEvent(QKeyEvent* event)
{
	if (!event->isAutoRepeat()) {
		m_action_key = 0;
	}
	QGLWidget::keyReleaseEvent(event);
}

/*****************************************************************************/

void Board::mousePressEvent(QMouseEvent* event)
{
	if (m_action_button != Qt::NoButton) {
		return;
	}

	m_action_button = event->button();
	if (m_action_button == Qt::MidButton || (m_action_button == Qt::LeftButton && m_action_key == Qt::Key_Shift)) {
		startScrolling();
	} else if (m_action_button == Qt::LeftButton) {
		m_select_pos = event->pos();
	}

	QGLWidget::mousePressEvent(event);
}

/*****************************************************************************/

void Board::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != m_action_button) {
		return;
	}

	switch (m_action_button) {
	case Qt::LeftButton:
		togglePiecesUnderCursor();
		break;
	case Qt::RightButton:
		rotatePiece();
		break;
	case Qt::MidButton:
		stopScrolling();
		break;
	default:
		break;
	}
	m_action_button = Qt::NoButton;

	QGLWidget::mouseReleaseEvent(event);
}

/*****************************************************************************/

void Board::mouseMoveEvent(QMouseEvent* event)
{
	QPoint delta = (event->pos() / m_scale) - (m_cursor_pos / m_scale);

	if (m_scrolling) {
		scroll(delta);
	}

	if (!m_active_tiles.isEmpty()) {
		for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
			i.key()->moveBy(delta);
		}
		if (m_active_tiles.size() == 1) // If exactly one tile is active, try attachNeighbors
			m_active_tiles.constBegin().key()->attachNeighbors();
		updateCompleted();

		// Handle finishing game
		if (m_pieces.count() == 1)
			finishGame();
	}

	if (!m_selecting && m_action_button == Qt::LeftButton && m_action_key == 0) {
		m_selecting = (event->pos() - m_select_pos).manhattanLength() >= 7;
	}

	updateGL();

	m_cursor_pos = event->pos();

	if (!m_scrolling)
		updateCursor();
}

/*****************************************************************************/

void Board::wheelEvent(QWheelEvent* event)
{
	if (event->delta() > 0) {
		zoomIn();
	} else {
		zoomOut();
	}

	QGLWidget::wheelEvent(event);
}

/*****************************************************************************/

void Board::startScrolling()
{
	m_scrolling = true;
	setCursor(Qt::SizeAllCursor);
}

/*****************************************************************************/

void Board::stopScrolling()
{
	m_scrolling = false;
	updateCursor();
}

/*****************************************************************************/

void Board::scroll(const QPoint& delta)
{
	m_pos -= delta;
	for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
		i.key()->moveBy(-delta);
	}
}

/*****************************************************************************/

void Board::togglePiecesUnderCursor() {
	switch (m_action_key) {
	case 0:
		if (!m_selecting) {
			if (tileUnderCursor(false)) {
				grabPiece();
			} else {
				releasePieces();
			}
		} else {
			selectPieces();
		}
		break;
	case Qt::Key_Shift:
		stopScrolling();
		break;
#if !defined(Q_OS_MAC)
	case Qt::Key_Control:
#else
	case Qt::Key_Meta:
#endif
		rotatePiece();
		break;
	default:
		break;
	}
}

/*****************************************************************************/

void Board::moveCursor(const QPoint& delta)
{
	QCursor::setPos(cursor().pos() + delta);
	updateCursor();
}

/*****************************************************************************/

void Board::grabPiece()
{
	if (m_scrolling || m_finished)
		return;

	Tile* tile = tileUnderCursor(false);
	if (tile == 0)
		return;
	Q_ASSERT(!m_active_tiles.contains(tile->parent()));
	m_active_tiles.insert(tile->parent(), tile);

	Piece* piece = tile->parent();
	m_pieces.removeAll(piece);
	m_pieces.append(piece);
	updateCursor();

	updateGL();
}

/*****************************************************************************/

void Board::releasePieces()
{
	if (m_scrolling || m_finished)
		return;

	for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
		i.key()->attachNeighbors();
		i.key()->pushNeighbors();
	}
	m_active_tiles.clear();
	updateCursor();
	updateCompleted();

	if (m_pieces.count() == 1)
		finishGame();

	updateGL();
}

/*****************************************************************************/

void Board::rotatePiece()
{
	if (m_scrolling || m_finished)
		return;

	if (m_active_tiles.isEmpty()) {
		Tile* child = tileUnderCursor();
		if (child == 0)
			return;
		Piece* piece = child->parent();

		piece->rotateAround(child);
		piece->attachNeighbors();
		if (m_active_tiles.isEmpty())
			piece->pushNeighbors();
	} else {
		for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
			i.key()->rotateAround(i.value());
		}
	}
	updateCompleted();

	if (m_pieces.count() == 1)
		finishGame();

	updateGL();
}

/*****************************************************************************/

void Board::selectPieces()
{
	m_selecting = false;

	QPoint cursor = mapPosition(m_cursor_pos);
	QRect rect = QRect(cursor, mapPosition(m_select_pos)).normalized();
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		Piece* piece = m_pieces.at(i);
		if (rect.contains(piece->boundingRect())) {
			Tile* tile = piece->children().at(rand() % piece->children().count());
			piece->moveBy(cursor - tile->scenePos() - QPoint(rand() % m_tile_size, rand() % m_tile_size));
			m_active_tiles.insert(piece, tile);
			m_pieces.removeAll(piece);
			m_pieces.append(piece);
		}
	}

	updateGL();
	updateCursor();
}

/*****************************************************************************/

void Board::loadImage()
{
	// Record currently open image
	QSettings().setValue("OpenGame/Image", m_image_path);

	// Load puzzle image
	QImage source("images/" + m_image_path);

	// Find tile sizes
	GLint max_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	QSize size = source.size();
	if (max_size < qMax(size.width(), size.height())) {
		size.scale(max_size, max_size, Qt::KeepAspectRatio);
	}
	m_tile_size = qMin(size.width(), size.height()) / (8 * m_difficulty);

	// Find puzzle and texture sizes
	int groups_wide = size.width() / (8 * m_tile_size);
	int groups_tall = size.height() / (8 * m_tile_size);
	m_total_pieces = groups_wide * groups_tall * 16;
	m_image_width = groups_wide * 8 * m_tile_size;
	m_image_height = groups_tall * 8 * m_tile_size;
	int x = 0;
	int y = 0;
	int sx = (size.width() - m_image_width) >> 1;
	int sy = (size.height() - m_image_height) >> 1;
	int sw = m_image_width;
	int sh = m_image_height;

	// Adjust values if showing entire image
	if (m_letterbox) {
		int width_tile_size = size.width() / (8 * groups_wide);
		int height_tile_size = size.height() / (8 * groups_tall);
		m_tile_size = qMax(width_tile_size, height_tile_size);
		m_image_width = groups_wide * 8 * m_tile_size;
		m_image_height = groups_tall * 8 * m_tile_size;

		QSize scaled_size = source.size();
		scaled_size.scale(m_image_width, m_image_height, Qt::KeepAspectRatio);
		size = scaled_size;

		x = (m_image_width - size.width()) >> 1;
		y = (m_image_height - size.height()) >> 1;
		sx = 0;
		sy = 0;
		sw = size.width();
		sh = size.height();
	}

	// Create puzzle texture
	source = source.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	int image_texture_size = powerOfTwo(qMax(m_image_width, m_image_height));
	m_image_ts = static_cast<float>(m_tile_size) / static_cast<float>(image_texture_size);
	QImage texture(image_texture_size, image_texture_size, QImage::Format_RGB32);
	{
		QPainter painter(&texture);
		painter.fillRect(texture.rect(), Qt::darkGray);
		painter.drawImage(x, y, source, sx, sy, sw, sh, Qt::AutoColor | Qt::AvoidDither);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_image = bindTexture(texture.mirrored(false, true));

	// Create overview
	m_overview->load(texture.copy(0, 0, m_image_width, m_image_height));

	// Create corners
	m_corners[0][0] = QPointF(0,0);
	m_corners[0][1] = QPointF(0,m_image_ts);
	m_corners[0][2] = QPointF(m_image_ts,m_image_ts);
	m_corners[0][3] = QPointF(m_image_ts,0);

	m_corners[1][0] = m_corners[0][1];
	m_corners[1][1] = m_corners[0][2];
	m_corners[1][2] = m_corners[0][3];
	m_corners[1][3] = m_corners[0][0];

	m_corners[2][0] = m_corners[0][2];
	m_corners[2][1] = m_corners[0][3];
	m_corners[2][2] = m_corners[0][0];
	m_corners[2][3] = m_corners[0][1];

	m_corners[3][0] = m_corners[0][3];
	m_corners[3][1] = m_corners[0][0];
	m_corners[3][2] = m_corners[0][1];
	m_corners[3][3] = m_corners[0][2];

	// Calculate zoom range
	m_scale_level_min = 2;
	m_scale_level_max = ceil(2 * (log(m_tile_size * 0.25) / log(1.25)));
	emit zoomRangeChanged(m_scale_level_min, m_scale_level_max);

	// Show overview
	m_overview->setVisible(QSettings().value("Overview/Visible", true).toBool());
}

/*****************************************************************************/

void Board::generateSuccessImage()
{
	QFontMetrics metrics(QFont("Sans", 24));
	int width = metrics.width(tr("Success"));
	int height = metrics.height();
	m_success_size = QSize(width + height, height * 2);
	QImage success(powerOfTwo(m_success_size.width()), powerOfTwo(m_success_size.height()), QImage::Format_ARGB32);
	{
		QPainter painter(&success);
		painter.fillRect(success.rect(), QColor(0, 0, 0, 0));
		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(0, 0, 0, 200));
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.drawRoundRect(0, 0, width + height, height * 2, 10);
		painter.setFont(QFont("Sans", 24));
		painter.setPen(Qt::white);
		painter.setRenderHint(QPainter::TextAntialiasing, true);
		painter.drawText(height / 2, height / 2 + metrics.ascent(), tr("Success"));
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	m_success = bindTexture(success.mirrored(false, true));
}

/*****************************************************************************/

void Board::updateCursor()
{
	int state = 0;
	if (!m_finished)
		state = (tileUnderCursor(false) != 0 || m_selecting) | (!m_active_tiles.isEmpty() * 2);

	switch (state) {
	case 1:
		setCursor(Qt::OpenHandCursor);
		break;
	case 2:
		setCursor(Qt::ClosedHandCursor);
		break;
	case 3:
		setCursor(Qt::PointingHandCursor);
		break;
	default:
		unsetCursor();
		break;
	}
}

/*****************************************************************************/

QPoint Board::mapCursorPosition() const
{
	return mapPosition(m_cursor_pos);
}

/*****************************************************************************/

QPoint Board::mapPosition(const QPoint& position) const
{
	return (position / m_scale) - (QPoint(width() >> 1, height() >> 1) / m_scale) + m_pos;
}

/*****************************************************************************/

void Board::draw(Tile* tile, const QPoint& pos, float depth) const
{
	int x1 = pos.x();
	int y1 = pos.y();
	int x2 = x1 + m_bumpmap_size;
	int y2 = y1 + m_bumpmap_size;

	const QPointF* corners = m_corners[tile->parent()->rotation()];
	float tx = tile->column() * m_image_ts;
	float ty = tile->row() * m_image_ts;

	glMultiTexCoord2f(GL_TEXTURE0, tx + corners[0].x(), ty + corners[0].y());
	glMultiTexCoord2f(GL_TEXTURE1, 0, 0);
		glVertex3f(x1, y1, depth);
	glMultiTexCoord2f(GL_TEXTURE0, tx + corners[1].x(), ty + corners[1].y());
	glMultiTexCoord2f(GL_TEXTURE1, 0, m_bumpmap_ts);
		glVertex3f(x1, y2, depth);
	glMultiTexCoord2f(GL_TEXTURE0, tx + corners[2].x(), ty + corners[2].y());
	glMultiTexCoord2f(GL_TEXTURE1, m_bumpmap_ts, m_bumpmap_ts);
		glVertex3f(x2, y2, depth);
	glMultiTexCoord2f(GL_TEXTURE0, tx + corners[3].x(), ty + corners[3].y());
	glMultiTexCoord2f(GL_TEXTURE1, m_bumpmap_ts, 0);
		glVertex3f(x2, y1, depth);
}

/*****************************************************************************/

void Board::updateCompleted()
{
	int t = 100 * (m_pieces.count() - 1);
	int T = m_total_pieces - 1;
	m_completed = 100 - (t / T);
	emit statusMessage(tr("%1% complete").arg(m_completed));
}

/*****************************************************************************/

Tile* Board::tileAt(const QPoint& pos, bool include_active) const
{
	Piece* piece;
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		piece = m_pieces.at(i);
		if (!include_active && m_active_tiles.contains(piece))
			continue;
		if (piece->boundingRect().contains(pos)) {
			foreach (Tile* tile, piece->children()) {
				if (tile->boundingRect().contains(pos))
					return tile;
			}
		}
	}
	return 0;
}

/*****************************************************************************/

Tile* Board::tileUnderCursor(bool include_active)
{
	if (include_active && !m_active_tiles.isEmpty())
		return m_active_tiles.constBegin().value();
	else
		return tileAt(mapCursorPosition(), include_active);
}

/*****************************************************************************/

void Board::finishGame()
{
	m_finished = true;

	// Rotate completed board to face up
	Piece* piece = m_pieces.first();
	if (piece->rotation() > 0) {
		for (int i = piece->rotation(); i < 4; ++i) {
			piece->rotateAround(0);
		}
	}

	m_overview->hide();
	m_active_tiles.clear();
	unsetCursor();
	zoomFit();
	emit retrievePiecesAvailable(false);

	QFile(QString("saves/%1.xml").arg(m_id)).remove();
	QSettings().remove("OpenGame/Image");

	emit finished();
}

/*****************************************************************************/

void Board::cleanup()
{
	deleteTexture(m_image);
	glDeleteTextures(1, &m_bumpmap);

	m_active_tiles.clear();
	qDeleteAll(m_pieces);
	m_pieces.clear();
	m_completed = 0;
	m_id = 0;

	m_scrolling = false;
	m_pos = QPoint(0, 0);
	m_finished = false;

	QSettings().remove("OpenGame/Image");
}

/*****************************************************************************/
