/***********************************************************************
 *
 * Copyright (C) 2008, 2010 Graeme Gott <graeme@gottcode.org>
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
#include <QImageReader>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QWheelEvent>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <cmath>
#include <cstdlib>
#include <ctime>

//-----------------------------------------------------------------------------

namespace
{

int powerOfTwo(int value)
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

const float scale_levels[] = { 0.125, 0.15625, 0.1875, 0.25, 0.3125, 0.40625, 0.5, 0.625, 0.78125, 1.0 };

}

//-----------------------------------------------------------------------------

Board::Board(QWidget* parent)
	: QGLWidget(parent),
	m_id(0),
	m_difficulty(0),
	m_image(0),
	m_image_ts(0),
	m_columns(0),
	m_rows(0),
	m_total_pieces(0),
	m_completed(0),
	m_pos(0, 0),
	m_scale_level(9),
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

	// Create overview dialog
	m_overview = new Overview(parent);
	connect(m_overview, SIGNAL(toggled(bool)), this, SIGNAL(overviewToggled(bool)));
}

//-----------------------------------------------------------------------------

Board::~Board()
{
	cleanup();
	deleteTexture(m_shadow);
	deleteTexture(m_success);
}

//-----------------------------------------------------------------------------

QList<Piece*> Board::findCollidingPieces(Piece* piece) const
{
	QList<Piece*> list;
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		Piece* parent = m_pieces.at(i);
		if (parent != piece && piece->collidesWith(parent)) {
			list.append(parent);
		}
	}
	return list;
}

//-----------------------------------------------------------------------------

Piece* Board::findCollidingPiece(Piece* piece) const
{
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		Piece* parent = m_pieces.at(i);
		if (parent != piece && piece->collidesWith(parent)) {
			return parent;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------

void Board::removePiece(Piece* piece)
{
	m_pieces.removeAll(piece);
	delete piece;
	piece = 0;
}

//-----------------------------------------------------------------------------

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
	foreach (QString file, QDir("saves/").entryList(QDir::Files)) {
		m_id = qMax(m_id, file.section(".", 0, 0).toInt());
	}
	m_id++;

	// Create textures
	m_difficulty = difficulty;
	m_image_path = image;
	loadImage();

	// Create tiles
	QVector< QVector<Tile*> > tiles = QVector< QVector<Tile*> >(m_columns, QVector<Tile*>(m_rows));
	for (int c = 0; c < m_columns; ++c) {
		for (int r = 0; r < m_rows; ++r) {
			tiles[c][r] = new Tile(c, r);
		}
	}

	// Create pieces
	std::srand(std::time(0));
	Solver solver(m_columns, m_rows);
	int full_width = m_columns * Tile::size() * 2;
	int full_height = m_rows * Tile::size() * 2;
	QList< QList<QPoint> > pieces = solver.pieces();
	foreach (const QList<QPoint>& group, pieces) {
		// Find tiles for piece
		QList<Tile*> children;
		foreach (const QPoint& tile, group) {
			children.append( tiles[tile.x()][tile.y()] );
		}

		// Create piece
		Tile* tile = children.first();
		Piece* piece = new Piece(QPoint(Tile::size() * tile->column(), Tile::size() * tile->row()), rand() % 4, children, this);
		m_pieces.append(piece);

		// Position piece
		piece->moveBy(QPoint(rand() % full_width, rand() % full_height) - piece->scenePos());
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

//-----------------------------------------------------------------------------

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
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	QXmlStreamReader xml(&file);

	// Load textures
	while (!xml.isStartElement()) {
		xml.readNext();
	}
	QXmlStreamAttributes attributes = xml.attributes();
	int board_zoom = 0;
	unsigned int version = attributes.value("version").toString().toUInt();
	if (xml.name() == QLatin1String("tetzle") && version == 4) {
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
		loadImage();
	} else {
		xml.raiseError(tr("Unknown data format"));
	}

	// Load pieces
	QPoint pos;
	int rotation = -1;
	QList<Tile*> tiles;

	while (!xml.atEnd()) {
		xml.readNext();

		if (xml.isEndElement() && xml.name() == QLatin1String("piece")) {
			m_pieces.append( new Piece(pos, rotation, tiles, this) );
		}
		if (!xml.isStartElement()) {
			continue;
		}

		if (xml.name() == QLatin1String("tile")) {
			attributes = xml.attributes();
			tiles.append( new Tile(attributes.value("column").toString().toInt(), attributes.value("row").toString().toInt()) );
		} else if (xml.name() == QLatin1String("piece")) {
			attributes = xml.attributes();
			pos = QPoint(attributes.value("x").toString().toInt(), attributes.value("y").toString().toInt());
			rotation = attributes.value("rotation").toString().toInt();
			tiles.clear();
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

//-----------------------------------------------------------------------------

void Board::saveGame()
{
	if (m_pieces.count() <= 1) {
		return;
	}

	QFile file(QString("saves/%1.xml").arg(m_id));
	if (!file.open(QIODevice::WriteOnly)) {
		return;
	}

	QXmlStreamWriter xml(&file);
	xml.setAutoFormatting(true);
	xml.writeStartDocument();

	xml.writeStartElement("tetzle");
	xml.writeAttribute("version", "4");
	xml.writeAttribute("image", m_image_path);
	xml.writeAttribute("difficulty", QString::number(m_difficulty));
	xml.writeAttribute("pieces", QString::number(m_total_pieces));
	xml.writeAttribute("complete", QString::number(m_completed));
	xml.writeAttribute("zoom", QString::number(m_scale_level));
	xml.writeAttribute("x", QString::number(m_pos.x()));
	xml.writeAttribute("y", QString::number(m_pos.y()));

	foreach (Piece* piece, m_pieces) {
		piece->save(xml);
	}

	xml.writeEndElement();

	xml.writeEndDocument();
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

void Board::zoomIn()
{
	zoom(m_scale_level + 1);
}

//-----------------------------------------------------------------------------

void Board::zoomOut()
{
	zoom(m_scale_level - 1);
}

//-----------------------------------------------------------------------------

void Board::zoomFit()
{
	// Find bounding rectangle
	QRect bounds(0, 0, 0, 0);
	foreach (Piece* piece, m_pieces) {
		bounds = bounds.united(piece->boundingRect());
	}
	m_pos = bounds.center();

	// Find scale factor
	float sx = static_cast<float>(width()) / static_cast<float>(bounds.width());
	float sy = static_cast<float>(height()) / static_cast<float>(bounds.height());
	float scale = qMin(1.0f, qMin(sx, sy));

	// Find closest scale level
	int scale_level = 9;
	for (int i = 0; i < 9; ++i) {
		if ((scale - scale_levels[i]) < 0.0f) {
			scale_level = (i - 1);
			break;
		}
	}
	zoom(scale_level);
}

//-----------------------------------------------------------------------------

void Board::zoom(int level)
{
	QPoint old_pos = mapCursorPosition();

	// Calculate new scale value
	m_scale_level = qBound(0, level, 9);
	m_scale = scale_levels[m_scale_level];

	// Update mouse cursor position
	QPoint new_pos = mapCursorPosition();
	for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
		i.key()->moveBy(new_pos - old_pos);
	}
	updateCursor();

	// Update scene
	updateGL();
	emit zoomChanged(m_scale_level, m_scale);
	emit zoomOutAvailable(m_scale_level > 0);
	emit zoomInAvailable(m_scale_level < 9);
}

//-----------------------------------------------------------------------------

void Board::toggleOverview()
{
	bool visible = !m_overview->isVisible();
	m_overview->setVisible(visible);
	QSettings().setValue("Overview/Visible", visible);
}

//-----------------------------------------------------------------------------

void Board::initializeGL()
{
	glDisable(GL_DEPTH_TEST);
	glColor4f(1, 1, 1, 1);
	glClearColor(1, 1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Load shadow image
	m_shadow = bindTexture(QImage(":/shadow.png"));

	// Create success message
	QFont font("Sans", 24);
	QFontMetrics metrics(font);
	int width = metrics.width(tr("Success"));
	int height = metrics.height();
	m_success_size = QSize(width + height, height * 2);
	QImage success(powerOfTwo(m_success_size.width()), powerOfTwo(m_success_size.height()), QImage::Format_ARGB32);
	{
		QPainter painter(&success);
		painter.fillRect(success.rect(), Qt::transparent);
		painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(0, 0, 0, 200));
		painter.drawRoundedRect(0, 0, width + height, height * 2, 10, 10);

		painter.setFont(font);
		painter.setPen(Qt::white);
		painter.drawText(height / 2, height / 2 + metrics.ascent(), tr("Success"));
	}
	m_success = bindTexture(success.mirrored(false, true));
}

//-----------------------------------------------------------------------------

void Board::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -2, 3);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

//-----------------------------------------------------------------------------

void Board::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	// Draw pieces
	glPushMatrix();
	glScalef(m_scale, m_scale, 0);
	glTranslatef((width() / (2 * m_scale)) - m_pos.x(), (height() / (2 * m_scale)) - m_pos.y(), 0);
	for (int i = 0; i < m_pieces.count(); ++i) {
		m_pieces.at(i)->draw();
	}
	glPopMatrix();

	// Draw selection rectangle
	if (m_selecting) {
		glBindTexture(GL_TEXTURE_2D, 0);
		glPushAttrib(GL_CURRENT_BIT);

		QRect box = QRect(m_cursor_pos, m_select_pos).normalized();

		glColor4f(0, 0, 1, 0.25);
		glBegin(GL_QUADS);
			glVertex2i(box.x(), box.y());
			glVertex2i(box.x(), box.y() + box.height());
			glVertex2i(box.x() + box.width(), box.y() + box.height());
			glVertex2i(box.x() + box.width(), box.y());
		glEnd();

		glColor3f(0, 0, 1);
		glBegin(GL_LINE_STRIP);
			glVertex2i(box.x(), box.y());
			glVertex2i(box.x(), box.y() + box.height());
			glVertex2i(box.x() + box.width(), box.y() + box.height());
			glVertex2i(box.x() + box.width(), box.y());
			glVertex2i(box.x(), box.y());
		glEnd();

		glPopAttrib();
	}

	// Draw success message
	if (m_finished) {
		glBindTexture(GL_TEXTURE_2D, m_success);

		int w = powerOfTwo(m_success_size.width());
		int h = powerOfTwo(m_success_size.height());
		int x = (width() >> 1) - (m_success_size.width() >> 1);
		int y = (height() >> 1) - (m_success_size.height() >> 1);

		glBegin(GL_QUADS);
			glTexCoord2i(0, 0);
			glVertex2i(x, y);

			glTexCoord2i(0, 1);
			glVertex2i(x, y + h);

			glTexCoord2i(1, 1);
			glVertex2i(x + w, y + h);

			glTexCoord2i(1, 0);
			glVertex2i(x + w, y);
		glEnd();
	}
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

void Board::keyReleaseEvent(QKeyEvent* event)
{
	if (!event->isAutoRepeat()) {
		m_action_key = 0;
	}
	QGLWidget::keyReleaseEvent(event);
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

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

		// Attach neighbors if only one piece is active
		if (m_active_tiles.size() == 1) {
			m_active_tiles.constBegin().key()->attachNeighbors();
			updateCompleted();
		}

		// Handle finishing game
		if (m_pieces.count() == 1) {
			finishGame();
		}
	}

	if (!m_selecting && m_action_button == Qt::LeftButton && m_action_key == 0) {
		m_selecting = (event->pos() - m_select_pos).manhattanLength() >= 7;
	}

	updateGL();

	m_cursor_pos = event->pos();

	if (!m_scrolling) {
		updateCursor();
	}
}

//-----------------------------------------------------------------------------

void Board::wheelEvent(QWheelEvent* event)
{
	if (event->delta() > 0) {
		zoomIn();
	} else {
		zoomOut();
	}

	QGLWidget::wheelEvent(event);
}

//-----------------------------------------------------------------------------

void Board::startScrolling()
{
	m_scrolling = true;
	setCursor(Qt::SizeAllCursor);
}

//-----------------------------------------------------------------------------

void Board::stopScrolling()
{
	m_scrolling = false;
	updateCursor();
}

//-----------------------------------------------------------------------------

void Board::scroll(const QPoint& delta)
{
	m_pos -= delta;
	for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
		i.key()->moveBy(-delta);
	}
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------

void Board::moveCursor(const QPoint& delta)
{
	QCursor::setPos(cursor().pos() + delta);
	updateCursor();
}

//-----------------------------------------------------------------------------

void Board::grabPiece()
{
	if (m_scrolling || m_finished) {
		return;
	}

	Tile* tile = tileUnderCursor(false);
	if (tile == 0) {
		return;
	}
	Q_ASSERT(!m_active_tiles.contains(tile->parent()));
	m_active_tiles.insert(tile->parent(), tile);

	Piece* piece = tile->parent();
	m_pieces.removeAll(piece);
	m_pieces.append(piece);
	updateCursor();

	updateGL();
}

//-----------------------------------------------------------------------------

void Board::releasePieces()
{
	if (m_scrolling || m_finished) {
		return;
	}

	for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
		i.key()->attachNeighbors();
		i.key()->pushNeighbors();
	}
	m_active_tiles.clear();
	updateCursor();
	updateCompleted();

	if (m_pieces.count() == 1) {
		finishGame();
	}

	updateGL();
}

//-----------------------------------------------------------------------------

void Board::rotatePiece()
{
	if (m_scrolling || m_finished) {
		return;
	}

	if (m_active_tiles.isEmpty()) {
		Tile* child = tileUnderCursor();
		if (child == 0) {
			return;
		}
		Piece* piece = child->parent();

		piece->rotateAround(child);
		piece->attachNeighbors();
		if (m_active_tiles.isEmpty()) {
			piece->pushNeighbors();
		}
	} else {
		for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
			i.key()->rotateAround(i.value());
		}
	}
	updateCompleted();

	if (m_pieces.count() == 1) {
		finishGame();
	}

	updateGL();
}

//-----------------------------------------------------------------------------

void Board::selectPieces()
{
	m_selecting = false;

	QPoint cursor = mapPosition(m_cursor_pos);
	QRect rect = QRect(cursor, mapPosition(m_select_pos)).normalized();
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		Piece* piece = m_pieces.at(i);
		if (rect.contains(piece->boundingRect())) {
			Tile* tile = piece->children().at(rand() % piece->children().count());
			piece->moveBy(cursor - tile->scenePos() - QPoint(rand() % Tile::size(), rand() % Tile::size()));
			m_active_tiles.insert(piece, tile);
			m_pieces.removeAll(piece);
			m_pieces.append(piece);
		}
	}

	updateGL();
	updateCursor();
}

//-----------------------------------------------------------------------------

void Board::loadImage()
{
	// Record currently open image
	QSettings().setValue("OpenGame/Image", m_image_path);

	// Load puzzle image
	QImageReader source("images/" + m_image_path);

	// Find image size
	QSize size = source.size();
	GLint max_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	max_size /= 2;
	if (max_size < qMax(size.width(), size.height())) {
		size.scale(max_size, max_size, Qt::KeepAspectRatio);
	}

	// Find puzzle dimensions
	m_columns = size.width();
	m_rows = size.height();
	int tile_size = Tile::size();
	if (m_columns > m_rows) {
		float ratio = static_cast<float>(m_rows) / static_cast<float>(m_columns);
		m_columns = 4 * m_difficulty;
		m_rows = qRound(m_columns * ratio);
		tile_size = qMin(tile_size, size.width() / m_columns);
	} else {
		float ratio = static_cast<float>(m_columns) / static_cast<float>(m_rows);
		m_rows = 4 * m_difficulty;
		m_columns = qRound(m_rows * ratio);
		tile_size = qMin(tile_size, size.height() / m_rows);
	}
	m_total_pieces = (m_columns * m_rows) / 4;

	// Create puzzle texture
	QSize scaled_size = size;
	size = QSize(m_columns * tile_size, m_rows * tile_size);
	scaled_size.scale(size, Qt::KeepAspectRatioByExpanding);
	source.setScaledSize(scaled_size);
	source.setScaledClipRect(QRect((scaled_size.width() - size.width()) / 2, (scaled_size.height() - size.height()) / 2, size.width(), size.height()));

	int image_texture_size = powerOfTwo(qMax(size.width(), size.height()));
	m_image_ts = static_cast<float>(tile_size) / static_cast<float>(image_texture_size);
	QImage texture(image_texture_size, image_texture_size, QImage::Format_RGB32);
	texture.fill(QColor(Qt::darkGray).rgb());
	{
		QPainter painter(&texture);
		painter.drawImage(0, 0, source.read(), 0, 0, size.width(), size.height(), Qt::AutoColor | Qt::AvoidDither);
	}
	m_image = bindTexture(texture.mirrored(false, true));

	// Create overview
	m_overview->load(texture.copy(0, 0, size.width(), size.height()));

	// Create corners
	m_corners[0][0] = QPointF(0,0);
	m_corners[0][1] = QPointF(m_image_ts,0);
	m_corners[0][2] = QPointF(m_image_ts,m_image_ts);
	m_corners[0][3] = QPointF(0,m_image_ts);

	m_corners[1][0] = m_corners[0][3];
	m_corners[1][1] = m_corners[0][0];
	m_corners[1][2] = m_corners[0][1];
	m_corners[1][3] = m_corners[0][2];

	m_corners[2][0] = m_corners[0][2];
	m_corners[2][1] = m_corners[0][3];
	m_corners[2][2] = m_corners[0][0];
	m_corners[2][3] = m_corners[0][1];

	m_corners[3][0] = m_corners[0][1];
	m_corners[3][1] = m_corners[0][2];
	m_corners[3][2] = m_corners[0][3];
	m_corners[3][3] = m_corners[0][0];

	// Show overview
	m_overview->setVisible(QSettings().value("Overview/Visible", true).toBool());
}

//-----------------------------------------------------------------------------

void Board::updateCursor()
{
	int state = 0;
	if (!m_finished) {
		state = (tileUnderCursor(false) != 0 || m_selecting) | (!m_active_tiles.isEmpty() * 2);
	}

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

//-----------------------------------------------------------------------------

QPoint Board::mapCursorPosition() const
{
	return mapPosition(m_cursor_pos);
}

//-----------------------------------------------------------------------------

QPoint Board::mapPosition(const QPoint& position) const
{
	return (position / m_scale) - (QPoint(width() >> 1, height() >> 1) / m_scale) + m_pos;
}

//-----------------------------------------------------------------------------

void Board::updateCompleted()
{
	int t = 100 * (m_pieces.count() - 1);
	int T = m_total_pieces - 1;
	m_completed = 100 - (t / T);
	emit statusMessage(tr("%1% complete").arg(m_completed));
}

//-----------------------------------------------------------------------------

Tile* Board::tileAt(const QPoint& pos, bool include_active) const
{
	Piece* piece;
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		piece = m_pieces.at(i);
		if (!include_active && m_active_tiles.contains(piece)) {
			continue;
		}
		if (piece->boundingRect().contains(pos)) {
			foreach (Tile* tile, piece->children()) {
				if (tile->boundingRect().contains(pos)) {
					return tile;
				}
			}
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------

Tile* Board::tileUnderCursor(bool include_active)
{
	if (include_active && !m_active_tiles.isEmpty()) {
		return m_active_tiles.constBegin().value();
	} else {
		return tileAt(mapCursorPosition(), include_active);
	}
}

//-----------------------------------------------------------------------------

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

	QFile::remove(QString("saves/%1.xml").arg(m_id));
	QSettings().remove("OpenGame/Image");

	emit finished();
}

//-----------------------------------------------------------------------------

void Board::cleanup()
{
	deleteTexture(m_image);

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

//-----------------------------------------------------------------------------
