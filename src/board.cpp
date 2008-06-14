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
	m_image_width(0),
	m_image_height(0),
	m_tile_size(0),
	m_image(0),
	m_bumpmap(0),
	m_image_ts(0),
	m_bumpmap_ts(0),
	m_active_tile(0),
	m_total_pieces(0),
	m_completed(0),
	m_pos(0, 0),
	m_scale_level(0),
	m_scale_level_min(0),
	m_scale_level_max(0),
	m_scale(0),
	m_scrolling(false),
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
	m_overview = new QLabel(this, Qt::Tool);
	m_overview->setWindowTitle(tr("Overview"));
	m_overview->setAlignment(Qt::AlignCenter);
	m_overview->installEventFilter(this);
	m_overview->move(QSettings().value("Overview/Position").toPoint());
}

/*****************************************************************************/

Board::~Board()
{
	cleanup();
	deleteTexture(m_success);
}

/*****************************************************************************/

void Board::reparent(Tile* tile)
{
	m_tiles.removeAll(tile);
}

/*****************************************************************************/

QList<Tile*> Board::collidingItems(Tile* tile)
{
	QList<Tile*> list;

	int region = 7.0f / m_scale;
	tile = tile->parent();
	QRect rect = tile->boundingRect().adjusted(-region, -region, region, region);

	Tile* parent;
	for (int i = m_tiles.count() - 1; i > -1; --i) {
		parent = m_tiles[i];
		Q_ASSERT(parent->parent() == parent);
		if (parent != tile && parent->boundingRect().intersects(rect)) {
			if (parent->rect().intersects(rect))
				list.append(parent);
			foreach (Tile* child, parent->children()) {
				if (child->rect().intersects(rect) && child != tile)
					list.append(child);
			}
		}
	}
	return list;
}

/*****************************************************************************/

void Board::newGame(const QString& image, int difficulty)
{
	// Remove any previous textures and tiles
	cleanup();

	// Update player about status
	emit statusMessage("");
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
	m_difficulty = difficulty;
	m_image_path = image;
	loadImage();

	// Create tiles
	int columns = m_image_width / m_tile_size / 8 * 8;
	int rows = m_image_height / m_tile_size / 8 * 8;
	QVector< QVector<Tile*> > tiles = QVector< QVector<Tile*> >(columns, QVector<Tile*>(rows));
	Tile* tile = 0;
	for (int c = 0; c < columns; ++c) {
		for (int r = 0; r < rows; ++r) {
			tile = new Tile(c, r, 0, QPoint(m_tile_size * c, m_tile_size * r), 0, this);
			tiles[c][r] = tile;
			m_tiles.append(tile);
		}
	}

	// Create pieces
	int groups_wide = columns / 8;
	int groups_tall = rows / 8;
	srand(time(0));
	Solver solver(8, 8, groups_wide * groups_tall);
	Tile* parent_tile = 0;
	int rotations = 0;
	int full_width = m_image_width * 2;
	int full_height = m_image_height * 2;
	for (int c = 0; c < groups_wide; ++c) {
		for (int r = 0; r < groups_tall; ++r) {
			foreach (const QList<QPoint>& piece, solver.solutions[r * groups_wide + c]) {
				Q_ASSERT(piece.size() == 4);
				parent_tile = tiles.at(piece.at(0).x() + (c * 8)).at(piece.at(0).y() + (r * 8));
				for (int i = 1; i < 4; ++i) {
					tile = tiles.at(piece.at(i).x() + (c * 8)).at(piece.at(i).y() + (r * 8));
					parent_tile->attach(tile);
				}

				// Rotate piece
				rotations = rand() % 4;
				for (int i = 0; i < rotations; ++i)
					parent_tile->rotateAround(parent_tile);

				// Position piece
				parent_tile->moveBy(QPoint(rand() % full_width, rand() % full_height) - parent_tile->scenePos());
			}
		}
	}

	// Don't cover other pieces
	m_scale = 1;
	foreach (Tile* tile, m_tiles) {
		tile->pushNeighbors(tile);
	}

	// Draw tiles
	zoomFit();
	updateCompleted();
}

/*****************************************************************************/

void Board::openGame(int id)
{
	// Remove any previous textures and tiles
	cleanup();

	// Update player about status
	emit statusMessage("");
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
		m_difficulty = attributes.value("difficulty").toString().toInt();
		board_zoom = attributes.value("zoom").toString().toInt();
		m_pos.setX(attributes.value("x").toString().toInt());
		m_pos.setY(attributes.value("y").toString().toInt());
		loadImage();
		if (version < 3) {
			float old_scale = (16 * m_difficulty * pow(1.25, board_zoom * 0.5)) / qMax(m_image_width, m_image_height);
			board_zoom = log(old_scale * (m_tile_size * 0.25)) / log(1.25) * 2;
		}
	} else {
		xml.raiseError(QString("Unknown data format"));
	}

	// Load tiles
	Tile* parent = 0;
	Tile* tile = 0;
	int rotation = -1;
	while (!xml.atEnd()) {
		xml.readNext();
		if (!xml.isStartElement())
			continue;
		if (xml.name() == QLatin1String("tile")) {
			attributes = xml.attributes();
			tile = new Tile(
				attributes.value("column").toString().toInt(),
				attributes.value("row").toString().toInt(),
				rotation != -1 ? rotation : attributes.value("rotation").toString().toInt(),
				QPoint(attributes.value("x").toString().toInt(), attributes.value("y").toString().toInt()),
				parent,
				this);
			if (!parent) {
				parent = tile;
				m_tiles.append(tile);
			}
		} else if (xml.name() == QLatin1String("group")) {
			parent = 0;
			QStringRef r = xml.attributes().value("rotation");
			rotation = !r.isEmpty() ? r.toString().toInt() : -1;
		} else if (xml.name() != QLatin1String("overview")) {
			xml.raiseError(QString("Unknown element '%1'").arg(xml.name().toString()));
		}
	}
	if (xml.hasError()) {
		QMessageBox::warning(0, tr("Error"), tr("Error parsing XML file.\n\n%1").arg(xml.errorString()));
		cleanup();
		return;
	}

	// Draw tiles
	zoom(board_zoom);
	updateCompleted();
}

/*****************************************************************************/

void Board::saveGame()
{
	if (m_tiles.count() <= 1)
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

	foreach (Tile* tile, m_tiles)
		tile->save(xml);

	xml.writeEndElement();

	xml.writeEndDocument();
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
	foreach (Tile* tile, m_tiles) {
		bounds = bounds.united(tile->boundingRect());
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
	int bumpmap_size = m_tile_size * m_scale;
	int bumpmap_texture_size = powerOfTwo(bumpmap_size);
	m_bumpmap_ts = static_cast<float>(bumpmap_size) / static_cast<float>(bumpmap_texture_size);
	QImage bumpmap(bumpmap_texture_size, bumpmap_texture_size, QImage::Format_RGB32);
	{
		QPainter painter(&bumpmap);
		painter.fillRect(bumpmap.rect(), QColor(128,128,128));

		painter.setPen(QColor(224,224,224));
		painter.drawLine(0, 0, bumpmap_size - 1, 0);
		painter.drawLine(0, 1, 0, bumpmap_size - 2);
		painter.setPen(QColor(32,32,32));
		painter.drawLine(0, bumpmap_size - 1, bumpmap_size - 1, bumpmap_size - 1);
		painter.drawLine(bumpmap_size - 1, 1, bumpmap_size - 1, bumpmap_size - 2);

		painter.setPen(QColor(160,160,160));
		painter.drawLine(1, 1, bumpmap_size - 2, 1);
		painter.drawLine(1, 2, 1, bumpmap_size - 3);
		painter.setPen(QColor(96,96,96));
		painter.drawLine(1, bumpmap_size - 2, bumpmap_size - 2, bumpmap_size - 2);
		painter.drawLine(bumpmap_size - 2, 2, bumpmap_size - 2, bumpmap_size - 3);
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
	if (m_active_tile)
		m_active_tile->parent()->moveBy(new_pos - old_pos);
	updateCursor();

	// Update scene
	updateGL();
	emit zoomChanged(m_scale_level);
	emit zoomOutAvailable(m_scale_level > m_scale_level_min);
	emit zoomInAvailable(m_scale_level < m_scale_level_max);
}

/*****************************************************************************/

void Board::showOverview()
{
	m_overview->show();
	QSettings().setValue("Overview/Visible", true);
}

/*****************************************************************************/

void Board::hideOverview()
{
	m_overview->hide();
	QSettings().setValue("Overview/Visible", false);
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

	glScalef(m_scale, m_scale, 1);
	float inverse_scale = 1.0f / (m_scale * 2);
	glTranslatef(width() * inverse_scale - m_pos.x(), height() * inverse_scale - m_pos.y(), 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_image);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED);
	glBindTexture(GL_TEXTURE_2D, m_bumpmap);

	float depth = 0;
	float depth_diff = 1.0f / m_tiles.count();
	glBegin(GL_QUADS);
		foreach (Tile* parent, m_tiles) {
			draw(parent, parent->scenePos(), depth);
			foreach (Tile* tile, parent->children()) {
				draw(tile, tile->scenePos(), depth);
			}
			depth += depth_diff;
		}
	glEnd();

	glPopMatrix();

	if (m_finished) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_success);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);

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

bool Board::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == m_overview) {
		if (event->type() == QEvent::Hide) {
			emit overviewHidden();
		} else if (event->type() == QEvent::Show) {
			emit overviewShown();
		}
		return false;
	} else {
		return QGLWidget::eventFilter(watched, event);
	}
}

/*****************************************************************************/

void Board::hideEvent(QHideEvent* event)
{
	QSettings().setValue("Overview/Position", m_overview->pos());
	QGLWidget::hideEvent(event);
}

/*****************************************************************************/

void Board::keyPressEvent(QKeyEvent* event)
{
	if (!event->isAutoRepeat()) {
		m_action_key = event->key();
		performAction();
	}
	QGLWidget::keyPressEvent(event);
}

/*****************************************************************************/

void Board::keyReleaseEvent(QKeyEvent* event)
{
	if (!event->isAutoRepeat()) {
		m_action_key = 0;
		performAction();
	}
	QGLWidget::keyReleaseEvent(event);
}

/*****************************************************************************/

void Board::mousePressEvent(QMouseEvent* event)
{
	m_action_button = event->button();
	performAction();
	QGLWidget::mousePressEvent(event);
}

/*****************************************************************************/

void Board::mouseReleaseEvent(QMouseEvent* event)
{
	m_action_button = Qt::NoButton;
	performAction();
	QGLWidget::mouseReleaseEvent(event);
}

/*****************************************************************************/

void Board::mouseMoveEvent(QMouseEvent* event)
{
	if (m_scrolling) {
		QPoint delta = (event->pos() - m_scroll_pos) / -m_scale;
		m_scroll_pos = event->pos();

		// Move by delta
		m_pos += delta;
		if (m_active_tile)
			m_active_tile->parent()->moveBy(delta);

		// Draw tiles
		updateGL();
	}

	if (m_active_tile) {
		m_active_tile->parent()->moveBy((event->pos() - m_active_pos) / m_scale);
		m_active_tile->parent()->attachNeighbors(7.0f / m_scale);
		m_active_pos = event->pos();
		updateGL();
		updateCompleted();

		// Handle finishing game
		if (m_tiles.count() == 1)
			finishGame();
	} else if (!m_scrolling) {
		updateCursor();
	}
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

void Board::performAction()
{
	if (m_action_button == Qt::LeftButton) {
		if (m_action_key == 0) {
			if (!m_active_tile) {
				grabTile();
			} else {
				releaseTile();
			}
#if !defined(Q_OS_MAC)
		} else if (m_action_key == Qt::Key_Control) {
#else
		} else if (m_action_key == Qt::Key_Meta) {
#endif
			rotateTile();
		} else if (m_action_key == Qt::Key_Shift) {
			startScrolling();
		}
	} else if (m_action_button == Qt::RightButton) {
		rotateTile();
	} else if (m_action_button == Qt::MidButton) {
		startScrolling();
	} else if (m_scrolling) {
		stopScrolling();
	}
}

/*****************************************************************************/

void Board::startScrolling()
{
	m_scrolling = true;
	m_scroll_pos = mapFromGlobal(QCursor::pos());
	setCursor(Qt::SizeAllCursor);
}

/*****************************************************************************/

void Board::stopScrolling()
{
	m_scrolling = false;
	updateCursor();
}

/*****************************************************************************/

void Board::grabTile()
{
	if (m_scrolling || m_finished)
		return;

	Tile* tile = tileUnderCursor();
	if (tile == 0)
		return;
	m_active_tile = tile;
	m_active_pos = mapFromGlobal(QCursor::pos());

	tile = tile->parent();
	m_tiles.removeAll(tile);
	m_tiles.append(tile);
	setCursor(Qt::ClosedHandCursor);

	updateGL();
}

/*****************************************************************************/

void Board::releaseTile()
{
	if (m_scrolling || m_finished)
		return;

	int region = 7.0f / m_scale;
	m_active_tile->parent()->attachNeighbors(region);
	m_active_tile->parent()->pushNeighbors(m_active_tile->parent());
	m_active_tile = 0;
	updateCursor();
	updateCompleted();

	if (m_tiles.count() == 1)
		finishGame();

	updateGL();
}

/*****************************************************************************/

void Board::rotateTile()
{
	if (m_scrolling || m_finished)
		return;

	Tile* child = tileUnderCursor();
	if (child == 0)
		return;
	Tile* tile = child->parent();

	int region = 7.0f / m_scale;
	tile->rotateAround(child);
	tile->attachNeighbors(region);
	if (!m_active_tile)
		tile->pushNeighbors(tile);
	updateCompleted();

	if (m_tiles.count() == 1)
		finishGame();

	updateGL();
}

/*****************************************************************************/

void Board::loadImage()
{
	// Record currently open image
	QSettings().setValue("OpenGame/Image", m_image_path);

	// Load puzzle image
	QImage source("images/" + m_image_path);

	// Create overview
	QPixmap overview = QPixmap::fromImage(source, Qt::AutoColor | Qt::AvoidDither);
	if (overview.width() > 400 || overview.height() > 400)
		overview = overview.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_overview->setPixmap(overview);
	m_overview->setMinimumSize(overview.size());
	m_overview->resize(overview.size());

	// Find image size
	GLint max_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	if (max_size < qMax(source.width(), source.height()))
		source = source.scaled(max_size, max_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	m_tile_size = qMin(source.width(), source.height()) / (m_difficulty * 8);
	int groups_wide = source.width() / m_tile_size / 8;
	int groups_tall = source.height() / m_tile_size / 8;
	m_total_pieces = groups_wide * groups_tall * 16;
	m_image_width = groups_wide * 8 * m_tile_size;
	m_image_height = groups_tall * 8 * m_tile_size;

	// Create puzzle texture
	int image_texture_size = powerOfTwo(qMax(m_image_width, m_image_height));
	m_image_ts = static_cast<float>(m_tile_size) / static_cast<float>(image_texture_size);
	QImage texture(image_texture_size, image_texture_size, QImage::Format_RGB32);
	{
		QPainter painter(&texture);
		painter.fillRect(texture.rect(), Qt::white);
		painter.drawImage(0, 0, source, (source.width() - m_image_width) >> 1, (source.height() - m_image_height) >> 1, m_image_width, m_image_height, Qt::AutoColor | Qt::AvoidDither);
	}
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_image = bindTexture(texture.mirrored(false, true));

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
	if (!m_active_tile) {
		QPoint pos = mapCursorPosition();
		if (tileAt(pos) && !m_finished) {
			setCursor(Qt::OpenHandCursor);
		} else {
			unsetCursor();
		}
	} else {
		setCursor(Qt::ClosedHandCursor);
	}
}

/*****************************************************************************/

QPoint Board::mapCursorPosition() const
{
	return ( (mapFromGlobal(QCursor::pos()) - QPoint(width() >> 1, height() >> 1)) * (1.0f / m_scale) ) + m_pos;
}

/*****************************************************************************/

void Board::draw(Tile* tile, const QPoint& pos, float depth) const
{
	int x1 = pos.x();
	int y1 = pos.y();
	int x2 = x1 + m_tile_size;
	int y2 = y1 + m_tile_size;

	const QPointF* corners = m_corners[tile->rotation()];
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
	int t = 100 * (m_tiles.count() - 1);
	int T = m_total_pieces - 1;
	m_completed = 100 - (t / T);
	emit statusMessage(tr("%1% complete").arg(m_completed));
}

/*****************************************************************************/

Tile* Board::tileAt(const QPoint& pos) const
{
	Tile* tile;
	for (int i = m_tiles.count() - 1; i > -1; --i) {
		tile = m_tiles[i];
		if (tile->boundingRect().contains(pos)) {
			if (tile->rect().contains(pos))
				return tile;
			foreach (Tile* child, tile->children()) {
				if (child->rect().contains(pos))
					return child;
			}
		}
	}
	return 0;
}

/*****************************************************************************/

Tile* Board::tileUnderCursor()
{
	if (m_active_tile) {
		return m_active_tile;
	} else {
		return tileAt(mapCursorPosition());
	}
}

/*****************************************************************************/

void Board::finishGame()
{
	m_finished = true;

	// Rotate completed board to face up
	Tile* tile = m_tiles.first();
	if (tile->rotation() > 0) {
		for (int i = tile->rotation(); i < 4; ++i) {
			tile->rotateAround(0);
		}
	}

	m_overview->hide();
	m_active_tile = 0;
	unsetCursor();
	zoomFit();

	QFile(QString("saves/%1.xml").arg(m_id)).remove();
	QSettings().remove("OpenGame/Image");

	emit finished();
}

/*****************************************************************************/

void Board::cleanup()
{
	deleteTexture(m_image);
	glDeleteTextures(1, &m_bumpmap);

	m_overview->clear();
	m_overview->hide();
	m_active_tile = 0;
	qDeleteAll(m_tiles);
	m_tiles.clear();
	m_completed = 0;

	m_scrolling = false;
	m_pos = QPoint(0, 0);
	m_finished = false;

	QSettings().remove("OpenGame/Image");
}

/*****************************************************************************/
