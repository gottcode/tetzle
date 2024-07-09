/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "board.h"

#include "appearance_dialog.h"
#include "edge_scroller.h"
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
#include <QGridLayout>
#include <QImageReader>
#include <QMatrix4x4>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QTimeLine>
#include <QWheelEvent>
#include <QVector2D>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

#include <cmath>
#include <cstdlib>

//-----------------------------------------------------------------------------

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
	: QWidget(parent)
	, m_id(0)
	, m_load_bevels(true)
	, m_has_bevels(true)
	, m_has_shadows(true)
	, m_bevel_pixmap(":/bumpmap.png")
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
	setAutoFillBackground(true);

	m_message = new Message(this);

	// Animate zooming to best fit
	m_zoom_timer = new QTimeLine(250, this);
	m_zoom_timer->setEasingCurve(QEasingCurve::Linear);
	m_zoom_timer->setFrameRange(0, 10);
	m_zoom_timer->setUpdateInterval(25);
	connect(m_zoom_timer, &QTimeLine::frameChanged, this, &Board::zoom);

	// Create edge scrollers
	m_scroll_left = new EdgeScroller(1, 0, this);
	m_scroll_left->hide();
	connect(m_scroll_left, &EdgeScroller::scroll, this, &Board::edgeScroll);

	m_scroll_right = new EdgeScroller(-1, 0, this);
	m_scroll_right->hide();
	connect(m_scroll_right, &EdgeScroller::scroll, this, &Board::edgeScroll);

	m_scroll_up = new EdgeScroller(0, 1, this);
	m_scroll_up->hide();
	connect(m_scroll_up, &EdgeScroller::scroll, this, &Board::edgeScroll);

	m_scroll_down = new EdgeScroller(0, -1, this);
	m_scroll_down->hide();
	connect(m_scroll_down, &EdgeScroller::scroll, this, &Board::edgeScroll);

	QGridLayout* layout = new QGridLayout(this);

	layout->setColumnMinimumWidth(0, 16);
	layout->setColumnStretch(1, 1);
	layout->setColumnMinimumWidth(2, 16);

	layout->setRowMinimumHeight(0, 16);
	layout->setRowStretch(1, 1);
	layout->setRowMinimumHeight(2, 16);

	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	layout->addWidget(m_scroll_left, 1, 0);
	layout->addWidget(m_scroll_right, 1, 2);
	layout->addWidget(m_scroll_up, 0, 1);
	layout->addWidget(m_scroll_down, 2, 1);

	// Create overview dialog
	m_overview = new Overview(parent);
	connect(m_overview, &Overview::toggled, this, &Board::overviewToggled);

	// Load colors
	AppearanceDialog dialog;
	dialog.accept();
	setAppearance(dialog);
}

//-----------------------------------------------------------------------------

Board::~Board()
{
	cleanup();
	delete m_message;
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
	m_has_bevels = dialog.hasBevels();
	m_has_shadows = dialog.hasShadows();

	QPalette palette = dialog.colors();
	palette.setColor(backgroundRole(), palette.color(QPalette::Base).darker(150));
	setPalette(palette);
	for (Piece* piece : std::as_const(m_pieces)) {
		piece->setSelected(piece->isSelected());
	}

	const qreal pixelratio = devicePixelRatioF();
	m_shadow_pixmap = dialog.shadow(pixelratio);
	m_selected_shadow_pixmap = dialog.shadowSelected(pixelratio);
}

//-----------------------------------------------------------------------------

void Board::updateSceneRectangle(Piece* piece)
{
	int size = Tile::size / 2;
	m_scene = m_scene.united(piece->boundingRect().adjusted(-size, -size, size, size));
	updateViewport();
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
	QCoreApplication::processEvents();

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
	QCoreApplication::processEvents();

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
	}
	updateViewport();

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

	QSettings().setValue("Overview/Visible", m_overview->isVisible());
}

//-----------------------------------------------------------------------------

void Board::retrievePieces()
{
	// Inform user this will take awhile
	updateStatusMessage(tr("Retrieving pieces..."));
	QApplication::setOverrideCursor(Qt::WaitCursor);
	QCoreApplication::processEvents();

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
		updateViewport();
		update();
		return;
	}

	// Animate zoom
	const int count = abs(level - m_scale_level);
	if (level > m_scale_level) {
		m_zoom_timer->setDirection(QTimeLine::Forward);
		m_zoom_timer->setFrameRange(m_scale_level, level);
	} else {
		m_zoom_timer->setDirection(QTimeLine::Backward);
		m_zoom_timer->setFrameRange(level, m_scale_level);
	}
	m_zoom_timer->setDuration(count * m_zoom_timer->updateInterval());
	m_zoom_timer->start();
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
	updateViewport();
	update();
	Q_EMIT zoomChanged(m_scale_level, m_scale);
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
bool Board::event(QEvent* event)
{
	if (event->type() == QEvent::DevicePixelRatioChange) {
		loadImage();
		for (Piece* piece : std::as_const(m_pieces)) {
			piece->setPosition(piece->scenePos());
		}
	}
	return QWidget::event(event);
}
#endif
//-----------------------------------------------------------------------------

void Board::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// Transform viewport
	painter.save();
	painter.setTransform(m_viewport_transform);

	// Draw scene rectangle
	QColor fill = palette().color(QPalette::Base);
	QColor border = fill.lighter(125);
	if (m_scene.isValid()) {
		drawRect(painter, m_scene, fill, border);
	}

	// Draw pieces
	if (!m_pixmap.isNull()) {
		FragmentList bevel;
		FragmentList shadow;
		FragmentList tiles;
		const int total_tiles = m_columns * m_rows;
		bevel.reserve(total_tiles);
		shadow.reserve(total_tiles);
		tiles.reserve(total_tiles);
		for (Piece* piece : std::as_const(m_pieces)) {
			const QRect r = m_viewport_transform.mapRect(piece->boundingRect());
			if (m_viewport.intersects(r)) {
				if (m_has_shadows) {
					shadow.append(piece->shadow());
				}
				tiles.append(piece->tiles());
				if (m_has_bevels) {
					bevel.append(piece->bevel());
				}
			}
		}
		if (m_has_shadows) {
			shadow.draw(painter, m_shadow_pixmap);
		}
		tiles.draw(painter, m_pixmap, QPainter::OpaqueHint);
		if (m_has_bevels) {
			bevel.draw(painter, m_bevel_pixmap);
		}

		for (Piece* piece : std::as_const(m_selected_pieces)) {
			if (m_has_shadows) {
				piece->shadow().draw(painter, m_selected_shadow_pixmap);
			}
			piece->tiles().draw(painter, m_pixmap, QPainter::OpaqueHint);
			if (m_has_bevels) {
				piece->bevel().draw(painter, m_bevel_pixmap);
			}
		}

		for (Piece* piece : m_active_pieces) {
			if (m_has_shadows) {
				piece->shadow().draw(painter, m_selected_shadow_pixmap);
			}
			piece->tiles().draw(painter, m_pixmap, QPainter::OpaqueHint);
			if (m_has_bevels) {
				piece->bevel().draw(painter, m_bevel_pixmap);
			}
		}
	}

	// Untransform viewport
	painter.restore();

	// Draw selection rectangle
	if (m_selecting) {
		fill = border = palette().color(QPalette::Highlight);
		fill.setAlpha(48);
		drawRect(painter, m_selection, fill, border);
	}

	// Draw message
	m_message->draw(painter);
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
	QWidget::keyPressEvent(event);
}

//-----------------------------------------------------------------------------

void Board::keyReleaseEvent(QKeyEvent* event)
{
	if (!event->isAutoRepeat()) {
		m_action_key = 0;
	}
	QWidget::keyReleaseEvent(event);
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

	QWidget::mousePressEvent(event);
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

	QWidget::mouseReleaseEvent(event);
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

		m_selection = QRect(event->pos(), m_select_pos).normalized();
	}

	update();

	m_cursor_pos = event->pos();

	if (!m_scrolling) {
		updateCursor();
	}
}

//-----------------------------------------------------------------------------

void Board::resizeEvent(QResizeEvent* event)
{
	updateViewport();
	QWidget::resizeEvent(event);
}

//-----------------------------------------------------------------------------

void Board::wheelEvent(QWheelEvent* event)
{
	if (event->angleDelta().y() > 0) {
		zoomIn();
	} else {
		zoomOut();
	}

	QWidget::wheelEvent(event);
}

//-----------------------------------------------------------------------------

void Board::edgeScroll(int horizontal, int vertical)
{
	scroll(QPoint((5 * horizontal) / m_scale, (5 * vertical) / m_scale));
	update();
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
	updateViewport();
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
	QCoreApplication::processEvents();

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
	int count = m_selected_pieces.count();
	for (int i = 0; i < count; ++i) {
		Piece* piece = m_selected_pieces.at(i);
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

void Board::drawRect(QPainter& painter, const QRect& rect, const QColor& fill, const QColor& border)
{
	painter.save();

	painter.setPen(QPen(border, 0));
	painter.setBrush(fill);
	painter.drawRect(rect);

	painter.restore();
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

	// Set pixmap fragments to match device pixel ratio for high DPI rendering
	const qreal pixelratio = devicePixelRatioF();
	FragmentList::setDevicePixelRatio(pixelratio);

	// Create bevel texture
	m_bevel_pixmap = QPixmap(":/bumpmap.png");
	m_bevel_pixmap = m_bevel_pixmap.scaled(m_bevel_pixmap.size() * pixelratio, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	m_bevel_pixmap.setDevicePixelRatio(pixelratio);

	// Create shadow textures
	m_shadow_pixmap = AppearanceDialog::shadow(pixelratio);
	m_selected_shadow_pixmap = AppearanceDialog::shadowSelected(pixelratio);

	// Create puzzle texture
	const QSize size(m_columns * Tile::size * pixelratio, m_rows * Tile::size * pixelratio);
	QSize scaled_size = source.size();
	scaled_size.scale(size, Qt::KeepAspectRatioByExpanding);
	source.setScaledSize(scaled_size);
	source.setScaledClipRect(QRect((scaled_size.width() - size.width()) / 2, (scaled_size.height() - size.height()) / 2, size.width(), size.height()));
	const QImage image = source.read();

	m_pixmap = QPixmap(image.width(), image.height());
	m_pixmap.fill(Qt::darkGray);
	{
		QPainter painter(&m_pixmap);
		painter.drawImage(0, 0, image, 0, 0, image.width(), image.height(), Qt::AutoColor | Qt::AvoidDither);
	}
	m_pixmap.setDevicePixelRatio(pixelratio);

	// Create overview
	m_overview->load(image, pixelratio);
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
	QCoreApplication::processEvents();
}

//-----------------------------------------------------------------------------

void Board::updateViewport()
{
	m_viewport = rect();

	m_viewport_transform.reset();
	m_viewport_transform.scale(m_scale, m_scale);
	m_viewport_transform.translate(std::lround((width() / (2 * m_scale)) - m_pos.x()), std::lround((height() / (2 * m_scale)) - m_pos.y()));

	const QRect scene = m_scene.width() ? m_viewport_transform.mapRect(m_scene) : m_viewport.adjusted(2, 2, -4, -4);
	m_scroll_left->setVisible((scene.left() - 2) < m_viewport.left());
	m_scroll_right->setVisible((scene.right() + 2) > m_viewport.right());
	m_scroll_up->setVisible((scene.top() - 2) < m_viewport.top());
	m_scroll_down->setVisible((scene.bottom() + 2) > m_viewport.bottom());
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
	m_scrolling = false;
	m_pos = QPoint(0, 0);
	m_finished = false;

	QSettings().remove("OpenGame");

	m_scroll_left->hide();
	m_scroll_right->hide();
	m_scroll_up->hide();
	m_scroll_down->hide();
}

//-----------------------------------------------------------------------------
