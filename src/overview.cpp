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

#include "overview.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QSettings>
#include <QWheelEvent>

/*****************************************************************************/

Overview::Overview(QWidget* parent)
:	QGraphicsView(parent),
	m_scale_start(0),
	m_scale_factor(0),
	m_scale_level(0),
	m_pixmap(0)
{
	setWindowTitle(tr("Overview"));
	setWindowFlags(Qt::Tool);

	setRenderHint(QPainter::SmoothPixmapTransform, true);
	setDragMode(ScrollHandDrag);
	setFrameStyle(NoFrame);

	QGraphicsScene* scene = new QGraphicsScene;
	setScene(scene);

	// Restore size and position
	resize(QSettings().value("Overview/Size", QSize(401,401)).toSize());
	move(QSettings().value("Overview/Position", QPoint(0,0)).toPoint());
}

/*****************************************************************************/

void Overview::load(const QImage& image)
{
	// Remove previous overview
	delete m_pixmap;
	m_pixmap = 0;
	resetMatrix();

	// Set scale
	QSize size = image.size();
	if (size.width() > 400 || size.height() > 400) {
		size.scale(400, 400, Qt::KeepAspectRatio);
		float max_image = qMax(image.width(), image.height());
		float min_image = qMax(size.width(), size.height());
		m_scale_start = min_image / max_image;
		m_scale_factor = 0.1f - (m_scale_start / 9.f);
	} else {
		m_scale_start = 1.f;
		m_scale_factor = 0.f;
	}
	m_scale_level = 0;

	// Resize window
	QSize scene_size = sceneRect().toRect().size();
	scene_size.scale(400, 400, Qt::KeepAspectRatio);
	bool default_size = (width() == scene_size.width() + 1) && (height() == scene_size.height() + 1);
	setMinimumSize(size.width() + 1, size.height() + 1);
	if (default_size) {
		resize(minimumSize());
	}

	// Show overview
	m_pixmap = scene()->addPixmap(QPixmap::fromImage(image, Qt::AutoColor | Qt::AvoidDither));
	m_pixmap->setTransformationMode(Qt::SmoothTransformation);
	scene()->setSceneRect(m_pixmap->boundingRect());
	zoom();
}

/*****************************************************************************/

void Overview::hideEvent(QHideEvent* event)
{
	emit toggled(false);
	QGraphicsView::hideEvent(event);
}

/*****************************************************************************/

void Overview::moveEvent(QMoveEvent* event)
{
	QSettings().setValue("Overview/Position", pos());
	QGraphicsView::moveEvent(event);
}

/*****************************************************************************/

void Overview::resizeEvent(QResizeEvent* event)
{
	QSettings().setValue("Overview/Size", size());
	QGraphicsView::resizeEvent(event);
}

/*****************************************************************************/

void Overview::showEvent(QShowEvent* event)
{
	emit toggled(true);
	QGraphicsView::showEvent(event);
}

/*****************************************************************************/

void Overview::wheelEvent(QWheelEvent* event)
{
	if (event->delta() > 0) {
		zoomIn();
	} else {
		zoomOut();
	}
	event->accept();
}

/*****************************************************************************/

void Overview::zoomIn()
{
	if (m_scale_level < 9) {
		m_scale_level++;
		zoom();
	}
}

/*****************************************************************************/

void Overview::zoomOut()
{
	if (m_scale_level > 0) {
		m_scale_level--;
		zoom();
	}
}

/*****************************************************************************/

void Overview::zoom()
{
	float s = m_scale_start + (m_scale_factor * m_scale_level);
	setMatrix(QMatrix().scale(s, s));
}

/*****************************************************************************/
