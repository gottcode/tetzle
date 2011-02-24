/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include "appearance_dialog.h"
#include "message.h"
#include "overview.h"
#include "path.h"
#include "piece.h"
#include "solver.h"
#include "tile.h"
#include "zoom_slider.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMatrix4x4>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QWheelEvent>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

#include <cmath>
#include <cstdlib>
#include <ctime>

//-----------------------------------------------------------------------------

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

	m_message = new Message(this);

	// Create overview dialog
	m_overview = new Overview(parent);
	connect(m_overview, SIGNAL(toggled(bool)), this, SIGNAL(overviewToggled(bool)));
}

//-----------------------------------------------------------------------------

Board::~Board()
{
	cleanup();
	deleteTexture(m_shadow);
	delete m_message;
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

void Board::setColors(const QPalette& palette)
{
	qglClearColor(palette.color(QPalette::Base).darker(150));
	setPalette(palette);
	foreach (Piece* piece, m_pieces) {
		piece->setSelected(piece->selected());
	}
}

//-----------------------------------------------------------------------------

void Board::updateSceneRectangle(Piece* piece)
{
	m_scene = m_scene.united(marginRect(piece->boundingRect()));
}

//-----------------------------------------------------------------------------

void Board::newGame(const QString& image, int difficulty)
{
	// Remove any previous textures and tiles
	cleanup();

	// Prevent starting a game with a missing image
	if (!QFileInfo(Path::image(image)).exists()) {
		QMessageBox::warning(this, tr("Error"), tr("Missing image."));
		return;
	}

	// Update player about status
	emit completionChanged(0);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_message->setText(tr("Please Wait"));
	m_message->setVisible(true);

	// Generate ID
	m_id = 0;
	foreach (QString file, QDir(Path::saves()).entryList(QDir::Files)) {
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
	QList< QList<QPoint> > pieces = solver.pieces();
	std::random_shuffle(pieces.begin(), pieces.end());
	foreach (const QList<QPoint>& group, pieces) {
		// Find tiles for piece
		QList<Tile*> children;
		foreach (const QPoint& tile, group) {
			children.append( tiles[tile.x()][tile.y()] );
		}

		// Create piece
		Piece* piece = new Piece(QPoint(0, 0), rand() % 4, children, this);
		m_pieces.append(piece);
		piece->pushNeighbors();
	}

	// Draw tiles
	m_message->setVisible(false);
	QApplication::restoreOverrideCursor();
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
	emit completionChanged(0);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_message->setText(tr("Please Wait"));
	m_message->setVisible(true);

	// Open saved game file
	m_id = id;
	QFile file(Path::save(m_id));
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
	QRect rect;
	unsigned int version = attributes.value("version").toString().toUInt();
	if (xml.name() == QLatin1String("tetzle") && version == 4) {
		m_image_path = attributes.value("image").toString();
		if (!QFileInfo(Path::image(m_image_path)).exists()) {
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, tr("Error"), tr("Missing image."));
			cleanup();
			return;
		}
		m_difficulty = attributes.value("difficulty").toString().toInt();
		board_zoom = attributes.value("zoom").toString().toInt();
		m_pos.setX(attributes.value("x").toString().toInt());
		m_pos.setY(attributes.value("y").toString().toInt());
		QStringList values = attributes.value("rect").toString().split(",");
		rect.setRect(values.value(0).toInt(),
			values.value(1). toInt(),
			values.value(2).toInt(),
			values.value(3).toInt());
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
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, tr("Error"), tr("Error parsing XML file.\n\n%1").arg(xml.errorString()));
		cleanup();
		return;
	}

	// Load scene rectangle
	updateSceneRectangle();
	if (rect.contains(m_scene)) {
		m_scene = rect;
	}

	// Draw tiles
	m_message->setVisible(false);
	QApplication::restoreOverrideCursor();
	zoom(board_zoom);
	updateCompleted();
	emit retrievePiecesAvailable(true);
}

//-----------------------------------------------------------------------------

void Board::saveGame()
{
	if ((m_pieces.count() + m_active_tiles.count()) <= 1) {
		return;
	}

	QFile file(Path::save(m_id));
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
	xml.writeAttribute("rect", QString("%1,%2,%3,%4").
		arg(m_scene.x())
		.arg(m_scene.y())
		.arg(m_scene.width())
		.arg(m_scene.height()));

	foreach (Piece* piece, m_pieces) {
		piece->save(xml);
	}

	xml.writeEndElement();

	xml.writeEndDocument();
}

//-----------------------------------------------------------------------------

void Board::retrievePieces()
{
	// Inform user this will take awhile
	m_message->setText(tr("Please Wait"));
	m_message->setVisible(true);
	QApplication::setOverrideCursor(Qt::WaitCursor);

	// Make sure all pieces are free
	QList<Piece*> pieces = m_pieces + m_active_tiles.keys();
	m_pieces.clear();
	m_active_tiles.clear();

	// Move all pieces to center of view
	std::random_shuffle(pieces.begin(), pieces.end());
	foreach (Piece* piece, pieces) {
		m_pieces.append(piece);
		piece->moveBy(m_pos - piece->boundingRect().center());
		piece->setSelected(false);
		piece->pushNeighbors();
	}

	// Reset scene rectangle
	updateSceneRectangle();

	// Clear message and cursor
	m_message->setVisible(false);
	QApplication::restoreOverrideCursor();

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
	m_pos = m_scene.center();
	float sx = static_cast<float>(width()) / static_cast<float>(m_scene.width());
	float sy = static_cast<float>(height()) / static_cast<float>(m_scene.height());
	zoom(ZoomSlider::scaleLevel(qMin(sx, sy)));
}

//-----------------------------------------------------------------------------

void Board::zoom(int level)
{
	QPoint old_pos = mapCursorPosition();

	// Calculate new scale value
	m_scale_level = qBound(0, level, 9);
	m_scale = ZoomSlider::scaleFactor(m_scale_level);

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
	// Disable unused OpenGL features
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	// Enable OpenGL features
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	// Set OpenGL parameters
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1, 1, 1, 1);
	glFrontFace(GL_CCW);

	// Load shadow image
	m_shadow = bindTexture(QImage(":/shadow.png"), GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);

	// Load colors
	AppearanceDialog dialog;
	dialog.accept();
	setColors(dialog.colors());
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

	// Transform viewport
	QRect viewport = rect();
	QMatrix4x4 matrix;
	matrix.scale(m_scale, m_scale);
	matrix.translate((width() / (2 * m_scale)) - m_pos.x(), (height() / (2 * m_scale)) - m_pos.y());

	glPushMatrix();
	glMultMatrixd(matrix.constData());

	// Draw scene rectangle
	glPushAttrib(GL_CURRENT_BIT);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);

	qglColor(palette().color(QPalette::Base));
	int x1 = m_scene.x();
	int y1 = m_scene.y();
	int x2 = x1 + m_scene.width();
	int y2 = y1 + m_scene.height();
	GLint verts[] = { x1,y1, x1,y2, x2,y2, x2,y1 };
	glVertexPointer(2, GL_INT, 0, &verts);
	glDrawArrays(GL_QUADS, 0, 4);

	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glPopAttrib();

	// Draw pieces
	for (int i = 0; i < m_pieces.count(); ++i) {
		QRect r = matrix.mapRect(m_pieces.at(i)->boundingRect());
		if (viewport.intersects(r)) {
			m_pieces.at(i)->draw();
		}
	}
	for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
		i.key()->draw();
	}
	glPopMatrix();

	// Draw selection rectangle
	if (m_selecting) {
		glPushAttrib(GL_CURRENT_BIT);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);

		QRect box = QRect(m_cursor_pos, m_select_pos).normalized();
		int x1 = box.x();
		int y1 = box.y();
		int x2 = x1 + box.width();
		int y2 = y1 + box.height();
		GLint verts[] = { x1,y1, x1,y2, x2,y2, x2,y1 };
		glVertexPointer(2, GL_INT, 0, &verts);

		QColor highlight = palette().color(QPalette::Highlight);
		QColor fill = highlight;
		fill.setAlpha(48);

		qglColor(fill);
		glDrawArrays(GL_QUADS, 0, 4);

		qglColor(highlight.darker());
		glDrawArrays(GL_LINE_LOOP, 0, 4);

		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glPopAttrib();
	}

	// Draw message
	m_message->draw();
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
		if (m_pieces.count() + m_active_tiles.count() == 1) {
			finishGame();
		}
	}

	if (!m_selecting && m_action_button == Qt::LeftButton && m_action_key == 0) {
		m_selecting = (event->pos() - m_select_pos).manhattanLength() >= QApplication::startDragDistance();
	}
	if (m_selecting) {
		QRect rect = QRect(mapPosition(event->pos()), mapPosition(m_select_pos)).normalized();
		for (int i = 0; i < m_pieces.count(); ++i) {
			Piece* piece = m_pieces.at(i);
			piece->setSelected(rect.intersects(piece->boundingRect()));
		}
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
	piece->setSelected(true);
	updateCursor();

	updateGL();
}

//-----------------------------------------------------------------------------

void Board::releasePieces()
{
	if (m_scrolling || m_finished) {
		return;
	}

	if (m_active_tiles.count() == 1) {
		m_active_tiles.constBegin().key()->attachNeighbors();
		updateCompleted();
	}

	for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
		m_pieces.append(i.key());
		i.key()->setSelected(false);
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

		piece->rotate(mapCursorPosition());
		piece->attachNeighbors();
		if (m_active_tiles.isEmpty()) {
			piece->pushNeighbors();
		}
	} else {
		for (QHash<Piece*, Tile*>::const_iterator i = m_active_tiles.constBegin(); i != m_active_tiles.constEnd(); ++i) {
			i.key()->rotate(mapCursorPosition());
		}
	}
	updateCompleted();

	if (m_pieces.count() + m_active_tiles.count() == 1) {
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
		if (rect.intersects(piece->boundingRect())) {
			Tile* tile = piece->children().at(rand() % piece->children().count());
			piece->moveBy(cursor - tile->scenePos() - QPoint(rand() % Tile::size(), rand() % Tile::size()));
			m_active_tiles.insert(piece, tile);
			m_pieces.removeAll(piece);
			piece->setSelected(true);
		} else {
			piece->setSelected(false);
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
	QImageReader source(Path::image(m_image_path));

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
		m_rows = qMax(qRound(m_columns * ratio), 1);
		tile_size = qMin(tile_size, size.width() / m_columns);
	} else {
		float ratio = static_cast<float>(m_columns) / static_cast<float>(m_rows);
		m_rows = 4 * m_difficulty;
		m_columns = qMax(qRound(m_rows * ratio), 1);
		tile_size = qMin(tile_size, size.height() / m_rows);
	}
	m_total_pieces = (m_columns * m_rows) / 4;

	// Create puzzle texture
	QSize scaled_size = size;
	size = QSize(m_columns * tile_size, m_rows * tile_size);
	scaled_size.scale(size, Qt::KeepAspectRatioByExpanding);
	source.setScaledSize(scaled_size);
	source.setScaledClipRect(QRect((scaled_size.width() - size.width()) / 2, (scaled_size.height() - size.height()) / 2, size.width(), size.height()));
	QImage image = source.read();

	int image_texture_size = powerOfTwo(qMax(size.width(), size.height()));
	m_image_ts = static_cast<float>(tile_size) / static_cast<float>(image_texture_size);
	QImage texture(image_texture_size, image_texture_size, QImage::Format_ARGB32);
	texture.fill(QColor(Qt::darkGray).rgba());
	{
		QPainter painter(&texture);
		painter.drawImage(0, 0, image, 0, 0, image.width(), image.height(), Qt::AutoColor | Qt::AvoidDither);
	}
	m_image = bindTexture(texture, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);

	// Create overview
	m_overview->load(image.scaled(image.size() * 0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation));

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
	int t = 100 * (m_pieces.count() + m_active_tiles.count() - 1);
	int T = m_total_pieces - 1;
	m_completed = 100 - (t / T);
	emit completionChanged(m_completed);
}

//-----------------------------------------------------------------------------

void Board::updateSceneRectangle()
{
	m_scene = QRect(0,0,0,0);
	foreach (Piece* piece, m_pieces) {
		updateSceneRectangle(piece);
	}
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

	// Drop remaining piece
	if (!m_active_tiles.isEmpty()) {
		m_pieces.append(m_active_tiles.constBegin().key());
	}
	m_active_tiles.clear();

	// Rotate completed board to face up
	Piece* piece = m_pieces.first();
	if (piece->rotation() > 0) {
		for (int i = piece->rotation(); i < 4; ++i) {
			piece->rotate();
		}
	}
	updateSceneRectangle();
	piece->setSelected(false);

	m_overview->hide();
	unsetCursor();
	zoomFit();
	emit retrievePiecesAvailable(false);

	QFile::remove(Path::save(m_id));
	QSettings().remove("OpenGame/Image");

	emit finished();

	m_message->setText(tr("Success"));
	m_message->setVisible(true);
}

//-----------------------------------------------------------------------------

void Board::cleanup()
{
	deleteTexture(m_image);

	m_overview->scene()->clear();
	m_message->setVisible(false);
	m_active_tiles.clear();
	qDeleteAll(m_pieces);
	m_pieces.clear();
	m_completed = 0;
	m_id = 0;

	m_scene = QRect(0,0,0,0);
	m_scrolling = false;
	m_pos = QPoint(0, 0);
	m_finished = false;

	QSettings().remove("OpenGame/Image");
}

//-----------------------------------------------------------------------------
