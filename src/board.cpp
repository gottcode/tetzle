/*
	SPDX-FileCopyrightText: 2008-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "board.h"

#include "appearance_dialog.h"
#include "generator.h"
#include "message.h"
#include "overview.h"
#include "path.h"
#include "piece.h"
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
#include <QOpenGLTexture>
#include <QPainter>
#include <QSettings>
#include <QWheelEvent>
#include <QVector2D>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

#include <cmath>
#include <cstdlib>

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

namespace
{

struct PieceDetails
{
	QPoint pos;
	int rotation;
	QList<Tile*> tiles;

	PieceDetails(const QPoint& pos_, int rotation_, const QList<Tile*>& tiles_)
		: pos(pos_), rotation(rotation_), tiles(tiles_)
	{
	}
};

}

//-----------------------------------------------------------------------------

Board::Board(QWidget* parent)
	: QOpenGLWidget(parent)
	, m_id(0)
	, m_load_bevels(true)
	, m_has_bevels(true)
	, m_has_shadows(true)
	, m_image(nullptr)
	, m_image_ts(0)
	, m_columns(0)
	, m_rows(0)
	, m_total_pieces(0)
	, m_completed(0)
	, m_pos(0, 0)
	, m_scale_level(9)
	, m_scale(0)
	, m_scrolling(false)
	, m_selecting(false)
	, m_finished(false)
	, m_action_key(0)
	, m_action_button(Qt::NoButton)
	, m_random(QRandomGenerator::securelySeeded())
{
	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	setMouseTracking(true);

	m_message = new Message(this);

	// Create overview dialog
	m_overview = new Overview(parent);
	connect(m_overview, &Overview::toggled, this, &Board::overviewToggled);
}

//-----------------------------------------------------------------------------

Board::~Board()
{
	cleanup();
	delete m_bumpmap_image;
	delete m_shadow_image;
	delete m_message;
	delete graphics_layer;
	graphics_layer = nullptr;
}

//-----------------------------------------------------------------------------

Piece* Board::findCollidingPiece(Piece* piece) const
{
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		Piece* other = m_pieces.at(i);
		if (other != piece && piece->collidesWith(other)) {
			return other;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------

void Board::removePiece(Piece* piece)
{
	m_pieces.removeAll(piece);
	delete piece;
	piece = nullptr;
}

//-----------------------------------------------------------------------------

void Board::setAppearance(const AppearanceDialog& dialog)
{
	makeCurrent();

	m_has_bevels = dialog.hasBevels();
	m_has_shadows = dialog.hasShadows();

	QPalette palette = dialog.colors();
	graphics_layer->setClearColor(palette.color(QPalette::Base).darker(150));
	setPalette(palette);
	for (Piece* piece : std::as_const(m_pieces)) {
		piece->setSelected(piece->isSelected());
	}
}

//-----------------------------------------------------------------------------

void Board::updateSceneRectangle(Piece* piece)
{
	int size = Tile::size / 2;
	m_scene = m_scene.united(piece->boundingRect().adjusted(-size, -size, size, size));
	updateArray(m_scene_array, m_scene, 0);
}

//-----------------------------------------------------------------------------

void Board::newGame(const QString& image, int difficulty)
{
	// Remove any previous textures and tiles
	cleanup();
	zoom(0);

	// Prevent starting a game with a missing image
	if (!QFileInfo::exists(Path::image(image))) {
		QMessageBox::warning(this, tr("Error"), tr("Missing image."));
		return;
	}

	// Update player about status
	Q_EMIT completionChanged(0);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_message->setText(tr("Please Wait"));
	m_message->setVisible(true);

	// Generate ID
	m_id = 0;
	const QStringList files = QDir(Path::saves()).entryList(QDir::Files);
	for (const QString& file : files) {
		m_id = std::max(m_id, file.section(".", 0, 0).toInt());
	}
	m_id++;

	// Find puzzle dimensions
	QSizeF size = QImageReader(Path::image(image)).size();
	if (size.width() > size.height()) {
		m_columns = 4 * difficulty;
		m_rows = std::max(std::lround(m_columns * size.height() / size.width()), 1L);
	} else {
		m_rows = 4 * difficulty;
		m_columns = std::max(std::lround(m_rows * size.width() / size.height()), 1L);
	}
	m_total_pieces = (m_columns * m_rows) / 4;

	// Create textures
	updateStatusMessage(tr("Loading image..."));
	m_image_path = image;
	m_load_bevels = true;
	loadImage();

	// Generate puzzle
	updateStatusMessage(tr("Generating puzzle..."));
	Generator generator(m_columns, m_rows, m_random);
	QList<QList<Tile*>> pieces = generator.pieces();
	std::shuffle(pieces.begin(), pieces.end(), m_random);

	updateStatusMessage(tr("Creating pieces..."));
	int count = pieces.count();
	int step = (count > 25) ? (count / 25) : 1;
	for (int i = 0; i < count; ++i) {
		// Create piece
		Piece* piece = new Piece(QPoint(0, 0), randomInt(4), pieces.at(i), this);
		m_pieces.append(piece);
		piece->setPosition(m_pos - QRect(QPoint(0,0), piece->boundingRect().size()).center());
		piece->pushNeighbors();

		// Show pieces
		if ((i % step) == 0) {
			m_pos = m_scene.center();
			update();
			updateStatusMessage(tr("Creating pieces..."));
		}
	}

	for (int i = 0; i < count; ++i) {
		m_pieces.at(i)->findNeighbors(m_pieces);
	}
	Q_EMIT clearMessage();

	// Draw tiles
	m_message->setVisible(false);
	zoomFit();
	QApplication::restoreOverrideCursor();
	updateCompleted();
	Q_EMIT retrievePiecesAvailable(true);
}

//-----------------------------------------------------------------------------

void Board::openGame(int id)
{
	// Remove any previous textures and tiles
	cleanup();

	// Update player about status
	Q_EMIT completionChanged(0);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	m_message->setText(tr("Please Wait"));
	m_message->setVisible(true);

	// Open saved game file
	QFile file(Path::save(id));
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	m_id = id;
	QXmlStreamReader xml(&file);

	// Load puzzle details
	updateStatusMessage(tr("Loading puzzle..."));
	while (!xml.isStartElement()) {
		xml.readNext();
	}
	QXmlStreamAttributes attributes = xml.attributes();
	int board_zoom = 0;
	QRect rect;
	unsigned int version = attributes.value("version").toUInt();
	if (xml.name() == QLatin1String("tetzle") && version <= 5) {
		m_image_path = attributes.value("image").toString();
		if (!QFileInfo::exists(Path::image(m_image_path))) {
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, tr("Error"), tr("Missing image."));
			cleanup();
			return;
		}
		board_zoom = attributes.value("zoom").toInt();
		m_pos.setX(attributes.value("x").toInt());
		m_pos.setY(attributes.value("y").toInt());
		QStringList values = attributes.value("rect").toString().split(",");
		rect.setRect(values.value(0).toInt(),
			values.value(1). toInt(),
			values.value(2).toInt(),
			values.value(3).toInt());
	} else {
		xml.raiseError(tr("Unknown data format"));
	}

	// Load piece details
	QList<PieceDetails> pieces;
	QPoint pos;
	int rotation = -1;
	QList<Tile*> tiles;
	bool piece = (version > 3);
	m_load_bevels = false;

	while (!xml.atEnd()) {
		xml.readNext();

		if (xml.isEndElement() && (xml.name() == QLatin1String("piece") || xml.name() == QLatin1String("group"))) {
			pieces.append(PieceDetails(pos, rotation, tiles));
		}
		if (!xml.isStartElement()) {
			continue;
		}

		if (xml.name() == QLatin1String("tile")) {
			attributes = xml.attributes();
			if (!piece) {
				piece = true;
				pos = QPoint(attributes.value("x").toInt(), attributes.value("y").toInt());
				rotation = (rotation != -1) ? rotation : attributes.value("rotation").toInt();
			}
			int column = attributes.value("column").toInt();
			m_columns = std::max(m_columns, column);
			int row = attributes.value("row").toInt();
			m_rows = std::max(m_rows, row);
			int bevel = attributes.value("bevel").toInt();
			if (bevel) {
				m_load_bevels = true;
			}
			Tile* tile = new Tile(column, row);
			tile->setBevel(bevel);
			tiles.append(tile);
		} else if (xml.name() == QLatin1String("piece")) {
			attributes = xml.attributes();
			pos = QPoint(attributes.value("x").toInt(), attributes.value("y").toInt());
			rotation = attributes.value("rotation").toInt();
			tiles.clear();
		} else if (xml.name() == QLatin1String("group")) {
			piece = false;
			auto r = xml.attributes().value("rotation");
			rotation = !r.isEmpty() ? r.toInt() : -1;
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
	m_total_pieces = (++m_columns * ++m_rows) / 4;

	// Load image
	updateStatusMessage(tr("Loading image..."));
	loadImage();

	// Load pieces
	updateStatusMessage(tr("Loading pieces..."));
	int count = pieces.count();
	for (int i = 0; i < count; ++i) {
		const PieceDetails& details = pieces.at(i);
		m_pieces.append( new Piece(details.pos, details.rotation, details.tiles, this) );
	}
	for (int i = 0; i < count; ++i) {
		m_pieces.at(i)->findNeighbors(m_pieces);
	}
	Q_EMIT clearMessage();

	// Load scene rectangle
	updateSceneRectangle();
	if (rect.contains(m_scene)) {
		m_scene = rect;
		updateArray(m_scene_array, m_scene, 0);
	}

	// Draw tiles
	m_message->setVisible(false);
	if (version > 3) {
		zoom(board_zoom);
	} else {
		zoom(0);
		retrievePieces();
	}
	QApplication::restoreOverrideCursor();
	updateCompleted();
	Q_EMIT retrievePiecesAvailable(true);
}

//-----------------------------------------------------------------------------

void Board::saveGame()
{
	if (pieceCount() <= 1) {
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
	xml.writeAttribute("version", "5");
	xml.writeAttribute("image", m_image_path);
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

	for (Piece* piece : std::as_const(m_pieces)) {
		piece->save(xml);
	}
	for (Piece* piece : std::as_const(m_selected_pieces)) {
		piece->save(xml);
	}
	for (Piece* piece : std::as_const(m_active_pieces)) {
		piece->save(xml);
	}

	xml.writeEndElement();

	xml.writeEndDocument();
}

//-----------------------------------------------------------------------------

void Board::retrievePieces()
{
	// Inform user this will take awhile
	updateStatusMessage(tr("Retrieving pieces..."));
	QApplication::setOverrideCursor(Qt::WaitCursor);

	// Make sure all pieces are free
	QList<Piece*> pieces = m_pieces + m_active_pieces + m_selected_pieces;
	m_pieces.clear();
	m_active_pieces.clear();
	m_selected_pieces.clear();

	// Clear view while retrieving pieces
	m_pos = QPoint(0,0);
	m_scene = QRect(0,0,0,0);

	// Move all pieces to center of view
	std::shuffle(pieces.begin(), pieces.end(), m_random);
	for (Piece* piece : std::as_const(pieces)) {
		m_pieces.append(piece);
		piece->setPosition(m_pos - QRect(QPoint(0,0), piece->boundingRect().size()).center());
		piece->setSelected(false);
		piece->pushNeighbors();
	}

	// Update view
	zoomFit();

	// Clear message and cursor
	Q_EMIT clearMessage();
	QApplication::restoreOverrideCursor();
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
	// Find new scale level
	float sx = static_cast<float>(width()) / static_cast<float>(m_scene.width());
	float sy = static_cast<float>(height()) / static_cast<float>(m_scene.height());
	float factor = qBound(0.0f, std::min(sx, sy), 1.0f);
	int level = 0;
	for (int i = 9; i >= 0; --i) {
		if (ZoomSlider::scaleFactor(i) <= factor) {
			level = i;
			break;
		}
	}

	m_pos = m_scene.center();
	if (m_scale_level == level) {
		update();
		return;
	}

	// Animate zoom
	int delta = (level > m_scale_level) ? 1 : -1;
	int count = abs(level - m_scale_level);
	for (int i = 0; i < count; ++i) {
		zoom(m_scale_level + delta);
	}
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
	int count = m_active_pieces.count();
	for (int i = 0; i < count; ++i) {
		m_active_pieces.at(i)->moveBy(new_pos - old_pos);
	}
	updateCursor();

	// Update scene
	update();
	Q_EMIT zoomChanged(m_scale_level, m_scale);
	Q_EMIT zoomOutAvailable(m_scale_level > 0);
	Q_EMIT zoomInAvailable(m_scale_level < 9);
}

//-----------------------------------------------------------------------------

void Board::toggleOverview()
{
	bool visible = !m_overview->isVisible();
	m_overview->setVisible(visible);
	if (visible) {
		activateWindow();
	}
	QSettings().setValue("Overview/Visible", visible);
}

//-----------------------------------------------------------------------------

void Board::initializeGL()
{
	// Configure OpenGL
	GraphicsLayer::init();

	// Load static images
	m_bumpmap_image = new QOpenGLTexture(QImage(":/bumpmap.png"));
	m_bumpmap_image->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	m_bumpmap_image->setMagnificationFilter(QOpenGLTexture::Linear);

	m_shadow_image = new QOpenGLTexture(QImage(":/shadow.png"));
	m_shadow_image->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	m_shadow_image->setMagnificationFilter(QOpenGLTexture::Linear);

	// Load colors
	AppearanceDialog dialog;
	dialog.accept();
	setAppearance(dialog);
}

//-----------------------------------------------------------------------------

void Board::resizeGL(int w, int h)
{
	const qreal pixelratio = devicePixelRatioF();

	graphics_layer->setViewport(0, 0, w * pixelratio, h * pixelratio);

	QMatrix4x4 matrix;
	matrix.ortho(0, w * pixelratio, h * pixelratio, 0, -4000, 3);
	graphics_layer->setProjection(matrix);

	m_message->setViewport(size());
}

//-----------------------------------------------------------------------------

void Board::paintGL()
{
	graphics_layer->clear();

	graphics_layer->uploadData();

	// Transform viewport
	const qreal pixelratio = devicePixelRatioF();
	QRect viewport = rect();
	viewport.setSize(viewport.size() * pixelratio);
	QMatrix4x4 matrix;
	matrix.scale(m_scale * pixelratio, m_scale * pixelratio);
	matrix.translate((width() / (2 * m_scale)) - m_pos.x(), (height() / (2 * m_scale)) - m_pos.y());
	graphics_layer->setModelview(matrix);

	// Draw scene rectangle
	QColor fill = palette().color(QPalette::Base);
	QColor border = fill.lighter(125);
	if (m_scene.isValid()) {
		drawArray(m_scene_array, fill, border);
	}

	// Draw pieces
	if (m_image) {
		graphics_layer->bindTexture(0, m_image->textureId());
		if (m_has_bevels && m_load_bevels) {
			graphics_layer->setTextureUnits(2);
			graphics_layer->bindTexture(1, m_bumpmap_image->textureId());
		}

		int count = m_pieces.count();
		for (int i = 0; i < count; ++i) {
			QRect r = matrix.mapRect(m_pieces.at(i)->boundingRect());
			if (viewport.intersects(r)) {
				m_pieces.at(i)->drawTiles();
			}
		}

		count = m_selected_pieces.count();
		for (int i = 0; i < count; ++i) {
			m_selected_pieces.at(i)->drawTiles();
		}

		count = m_active_pieces.count();
		for (int i = 0; i < count; ++i) {
			m_active_pieces.at(i)->drawTiles();
		}

		if (m_has_bevels) {
			graphics_layer->setTextureUnits(1);
		}
	}

	// Draw shadows
	graphics_layer->setBlended(true);
	if (m_image && m_has_shadows) {
		graphics_layer->bindTexture(0, m_shadow_image->textureId());

		graphics_layer->setColor(palette().color(QPalette::Text));
		int count = m_pieces.count();
		for (int i = 0; i < count; ++i) {
			QRect r = matrix.mapRect(m_pieces.at(i)->boundingRect());
			if (viewport.intersects(r)) {
				m_pieces.at(i)->drawShadow();
			}
		}

		graphics_layer->setColor(palette().color(QPalette::Highlight));
		count = m_selected_pieces.count();
		for (int i = 0; i < count; ++i) {
			m_selected_pieces.at(i)->drawShadow();
		}

		count = m_active_pieces.count();
		for (int i = 0; i < count; ++i) {
			m_active_pieces.at(i)->drawShadow();
		}

		graphics_layer->setColor(Qt::white);
	}

	// Untransform viewport
	if (qFuzzyCompare(pixelratio, 1.0)) {
		graphics_layer->setModelview(QMatrix4x4());
	} else {
		QMatrix4x4 matrix;
		matrix.scale(pixelratio, pixelratio);
		graphics_layer->setModelview(matrix);
	}

	// Draw selection rectangle
	if (m_selecting) {
		fill = border = palette().color(QPalette::Highlight);
		fill.setAlpha(48);
		drawArray(m_selection_array, fill, border);
	}

	// Draw message
	m_message->draw();
	graphics_layer->setBlended(false);
}

//-----------------------------------------------------------------------------

void Board::keyPressEvent(QKeyEvent* event)
{
	int offset = (event->modifiers() & Qt::ControlModifier) ? 1 : 10;
	switch (event->key()) {
	// Scroll left
	case Qt::Key_Left:
		scroll(QPoint(2 * offset, 0));
		update();
		updateCursor();
		break;

	// Scroll up
	case Qt::Key_Up:
		scroll(QPoint(0, 2 * offset));
		update();
		updateCursor();
		break;

	// Scroll right
	case Qt::Key_Right:
		scroll(QPoint(-2 * offset, 0));
		update();
		updateCursor();
		break;

	// Scroll down
	case Qt::Key_Down:
		scroll(QPoint(0, -2 * offset));
		update();
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
	QOpenGLWidget::keyPressEvent(event);
}

//-----------------------------------------------------------------------------

void Board::keyReleaseEvent(QKeyEvent* event)
{
	if (!event->isAutoRepeat()) {
		m_action_key = 0;
	}
	QOpenGLWidget::keyReleaseEvent(event);
}

//-----------------------------------------------------------------------------

void Board::mousePressEvent(QMouseEvent* event)
{
	if (m_finished || m_action_button != Qt::NoButton) {
		return;
	}

	m_action_button = event->button();
	if (m_action_button == Qt::MiddleButton || (m_action_button == Qt::LeftButton && m_action_key == Qt::Key_Shift)) {
		startScrolling();
	} else if (m_action_button == Qt::LeftButton) {
		m_select_pos = event->pos();
	}

	QOpenGLWidget::mousePressEvent(event);
}

//-----------------------------------------------------------------------------

void Board::mouseReleaseEvent(QMouseEvent* event)
{
	if (m_finished || event->button() != m_action_button) {
		return;
	}

	switch (m_action_button) {
	case Qt::LeftButton:
		togglePiecesUnderCursor();
		break;

	case Qt::RightButton:
		rotatePiece();
		break;

	case Qt::MiddleButton:
		stopScrolling();
		break;

	default:
		break;
	}
	m_action_button = Qt::NoButton;

	QOpenGLWidget::mouseReleaseEvent(event);
}

//-----------------------------------------------------------------------------

void Board::mouseMoveEvent(QMouseEvent* event)
{
	QPoint delta = (event->pos() / m_scale) - (m_cursor_pos / m_scale);

	if (m_scrolling) {
		scroll(delta);
	}

	if (!m_active_pieces.isEmpty()) {
		int count = m_active_pieces.count();
		for (int i = 0; i < count; ++i) {
			m_active_pieces.at(i)->moveBy(delta);
		}

		// Attach neighbors if only one piece is active
		if (m_active_pieces.count() == 1) {
			m_active_pieces.first()->attachNeighbors();
			updateCompleted();
		}

		// Handle finishing game
		if (pieceCount() == 1) {
			finishGame();
		}
	}

	if (!m_selecting && m_action_button == Qt::LeftButton && m_action_key == 0) {
		m_selecting = QVector2D(event->pos() - m_select_pos).length() >= QApplication::startDragDistance();
	}
	if (m_selecting) {
		QRect rect = QRect(mapPosition(event->pos()), mapPosition(m_select_pos)).normalized();

		// Check for pieces that are now selected
		for (int i = 0; i < m_pieces.count(); ++i) {
			Piece* piece = m_pieces.at(i);
			if (rect.intersects(piece->boundingRect())) {
				piece->setSelected(true);
				m_selected_pieces += m_pieces.takeAt(i);
				i--;
			}
		}

		// Check for pieces that are no longer selected
		for (int i = 0; i < m_selected_pieces.count(); ++i) {
			Piece* piece = m_selected_pieces.at(i);
			if (!rect.intersects(piece->boundingRect())) {
				piece->setSelected(false);
				m_pieces += m_selected_pieces.takeAt(i);
				i--;
			}
		}

		updateArray(m_selection_array, QRect(event->pos(), m_select_pos).normalized(), 3000);
	}

	update();

	m_cursor_pos = event->pos();

	if (!m_scrolling) {
		updateCursor();
	}
}

//-----------------------------------------------------------------------------

void Board::wheelEvent(QWheelEvent* event)
{
	if (event->angleDelta().y() > 0) {
		zoomIn();
	} else {
		zoomOut();
	}

	QOpenGLWidget::wheelEvent(event);
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
	int count = m_active_pieces.count();
	for (int i = 0; i < count; ++i) {
		m_active_pieces.at(i)->moveBy(-delta);
	}
}

//-----------------------------------------------------------------------------

void Board::togglePiecesUnderCursor() {
	switch (m_action_key) {
	case 0:
		if (!m_selecting) {
			if (pieceUnderCursor()) {
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

	Piece* piece = pieceUnderCursor();
	if (!piece) {
		return;
	}
	m_active_pieces.append(piece);
	m_pieces.removeAll(piece);
	piece->setDepth(m_active_pieces.count() + 1);
	piece->setSelected(true);
	updateCursor();

	update();
}

//-----------------------------------------------------------------------------

void Board::releasePieces()
{
	if (m_scrolling || m_finished) {
		return;
	}

	// Inform user this may take awhile
	updateStatusMessage(tr("Placing pieces..."));
	QApplication::setOverrideCursor(Qt::WaitCursor);

	// Attach to closest piece
	int count = m_active_pieces.count();
	if (count == 1) {
		m_active_pieces.first()->attachNeighbors();
		updateCompleted();
	}

	// Place pieces
	Piece* piece;
	for (int i = 0; i < count; ++i) {
		piece = m_active_pieces.at(i);
		m_pieces.append(piece);
		piece->setDepth(0);
		piece->setSelected(false);
		piece->pushNeighbors();
	}
	m_active_pieces.clear();

	updateCursor();
	updateCompleted();

	// Clear message and cursor
	Q_EMIT clearMessage();
	QApplication::restoreOverrideCursor();

	// Check if game is over
	if (pieceCount() == 1) {
		finishGame();
	} else {
		update();
	}
}

//-----------------------------------------------------------------------------

void Board::rotatePiece()
{
	if (m_scrolling || m_finished) {
		return;
	}

	if (m_active_pieces.isEmpty()) {
		Piece* piece = pieceUnderCursor();
		if (!piece) {
			return;
		}
		piece->rotate(mapCursorPosition());
		piece->attachNeighbors();
		piece->pushNeighbors();
	} else {
		int count = m_active_pieces.count();
		for (int i = 0; i < count; ++i) {
			m_active_pieces.at(i)->rotate(mapCursorPosition());
		}
	}
	updateCompleted();

	if (pieceCount() == 1) {
		finishGame();
	}

	update();
}

//-----------------------------------------------------------------------------

void Board::selectPieces()
{
	m_selecting = false;

	QPoint cursor = mapCursorPosition();
	int depth = m_active_pieces.count() + 1;
	int count = m_selected_pieces.count();
	for (int i = 0; i < count; ++i) {
		Piece* piece = m_selected_pieces.at(i);
		piece->setDepth(depth + i);
		if (!piece->contains(cursor)) {
			piece->moveBy(cursor - piece->randomPoint());
		}
	}
	m_active_pieces += m_selected_pieces;
	m_selected_pieces.clear();

	update();
	updateCursor();
}

//-----------------------------------------------------------------------------

void Board::drawArray(const Region& region, const QColor& fill, const QColor& border)
{
	graphics_layer->setTextureUnits(0);

	graphics_layer->setColor(fill);
	graphics_layer->draw(region.fill);

	graphics_layer->setColor(border);
	graphics_layer->draw(region.border, GL_LINE_LOOP);

	graphics_layer->setColor(Qt::white);

	graphics_layer->setTextureUnits(1);
}

//-----------------------------------------------------------------------------

void Board::loadImage()
{
	// Record currently open image
	QSettings settings;
	settings.setValue("OpenGame/Image", m_image_path);

	// Load puzzle image
	if (!m_overview->isVisible() && settings.value("Overview/Visible", true).toBool()) {
		m_overview->show();
		m_overview->repaint();
		activateWindow();
	}
	QImageReader source(Path::image(m_image_path));

	// Find image size
	QSize size = source.size();
	const GLint max_size = graphics_layer->getMaxTextureSize() / 2;
	if (max_size < std::max(size.width(), size.height())) {
		size.scale(max_size, max_size, Qt::KeepAspectRatio);
	}

	// Create puzzle texture
	int tile_size = Tile::size;
	if (m_columns > m_rows) {
		tile_size = std::min(tile_size, size.width() / m_columns);
	} else {
		tile_size = std::min(tile_size, size.height() / m_rows);
	}
	QSize scaled_size = size;
	size = QSize(m_columns * tile_size, m_rows * tile_size);
	scaled_size.scale(size, Qt::KeepAspectRatioByExpanding);
	source.setScaledSize(scaled_size);
	source.setScaledClipRect(QRect((scaled_size.width() - size.width()) / 2, (scaled_size.height() - size.height()) / 2, size.width(), size.height()));
	QImage image = source.read();

	int image_texture_size = powerOfTwo(std::max(size.width(), size.height()));
	m_image_ts = static_cast<float>(tile_size) / static_cast<float>(image_texture_size);
	QImage texture(image_texture_size, image_texture_size, QImage::Format_ARGB32);
	texture.fill(QColor(Qt::darkGray).rgba());
	{
		QPainter painter(&texture);
		painter.drawImage(0, 0, image, 0, 0, image.width(), image.height(), Qt::AutoColor | Qt::AvoidDither);
	}
	m_image = new QOpenGLTexture(texture);
	m_image->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	m_image->setMagnificationFilter(QOpenGLTexture::Linear);
	m_image->setWrapMode(QOpenGLTexture::ClampToEdge);

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

	// Create overview
	m_overview->load(image, devicePixelRatioF());
}

//-----------------------------------------------------------------------------

void Board::updateCursor()
{
	int state = 0;
	if (!m_finished) {
		state = (pieceUnderCursor() || m_selecting) | (!m_active_pieces.isEmpty() * 2);
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
	int t = 100 * (pieceCount() - 1);
	int T = m_total_pieces - 1;
	m_completed = 100 - (t / T);
	Q_EMIT completionChanged(m_completed);
}

//-----------------------------------------------------------------------------

void Board::updateArray(Region& region, const QRect& rect, int z)
{
	int x1 = rect.x();
	int y1 = rect.y();
	int x2 = x1 + rect.width();
	int y2 = y1 + rect.height();

	graphics_layer->updateArray(region.fill,
	{
		Vertex::init(x1,y1,z),
		Vertex::init(x1,y2,z),
		Vertex::init(x2,y1,z),
		Vertex::init(x2,y1,z),
		Vertex::init(x1,y2,z),
		Vertex::init(x2,y2,z)
	});

	graphics_layer->updateArray(region.border,
	{
		Vertex::init(x1,y1,z),
		Vertex::init(x1,y2,z),
		Vertex::init(x2,y2,z),
		Vertex::init(x2,y1,z)
	});
}

//-----------------------------------------------------------------------------

void Board::updateSceneRectangle()
{
	m_scene = QRect(0,0,0,0);
	for (Piece* piece : std::as_const(m_pieces)) {
		updateSceneRectangle(piece);
	}
}

//-----------------------------------------------------------------------------

void Board::updateStatusMessage(const QString& message)
{
	Q_EMIT showMessage(message);
}

//-----------------------------------------------------------------------------

Piece* Board::pieceUnderCursor()
{
	QPoint pos = mapCursorPosition();
	Piece* piece;
	for (int i = m_pieces.count() - 1; i >= 0; --i) {
		piece = m_pieces.at(i);
		if (piece->contains(pos)) {
			return piece;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------

int Board::pieceCount()
{
	return m_pieces.count() + m_active_pieces.count() + m_selected_pieces.count();
}

//-----------------------------------------------------------------------------

void Board::finishGame()
{
	m_finished = true;

	// Drop remaining piece
	if (!m_active_pieces.isEmpty()) {
		m_pieces.append(m_active_pieces.first());
	}
	m_active_pieces.clear();

	// Rotate completed board to face up
	Piece* piece = m_pieces.first();
	if (piece->rotation() > 0) {
		for (int i = piece->rotation(); i < 4; ++i) {
			piece->rotate();
		}
	}
	piece->setSelected(false);

	// Hide scene rectangle
	m_scene = piece->boundingRect().adjusted(1,1,-2,-2);
	updateArray(m_scene_array, m_scene, 0);

	m_overview->hide();
	unsetCursor();
	zoomFit();
	Q_EMIT retrievePiecesAvailable(false);

	QFile::remove(Path::save(m_id));
	QSettings().remove("OpenGame");
	m_id = 0;

	Q_EMIT finished();

	m_message->setText(tr("Success"));
	m_message->setVisible(true, false);
}

//-----------------------------------------------------------------------------

void Board::cleanup()
{
	delete m_image;
	m_image = nullptr;

	Q_EMIT clearMessage();
	m_overview->reset();
	m_message->setVisible(false);
	m_active_pieces.clear();
	m_selected_pieces.clear();
	qDeleteAll(m_pieces);
	m_pieces.clear();
	m_completed = 0;
	m_id = 0;
	m_columns = 0;
	m_rows = 0;

	m_scene = QRect(0,0,0,0);
	updateArray(m_scene_array, m_scene, 0);
	m_scrolling = false;
	m_pos = QPoint(0, 0);
	m_finished = false;

	QSettings().remove("OpenGame");
}

//-----------------------------------------------------------------------------
