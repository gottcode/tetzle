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

#include "overview.h"

#include "zoom_slider.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QSettings>
#include <QWheelEvent>

//-----------------------------------------------------------------------------

Overview::Overview(QWidget* parent)
	: QGraphicsView(parent),
	m_min_scale_level(0),
	m_scale_level(0)
{
	setWindowTitle(tr("Overview"));
	setWindowFlags(Qt::Tool);

	setBackgroundRole(QPalette::Window);
	setRenderHint(QPainter::SmoothPixmapTransform, true);
	setDragMode(ScrollHandDrag);
	setFrameStyle(NoFrame);

	QGraphicsScene* scene = new QGraphicsScene;
	setScene(scene);

	// Restore geometry
	QSettings settings;
	if (settings.contains("Overview/Geometry")) {
		restoreGeometry(settings.value("Overview/Geometry").toByteArray());
	} else {
		resize(400, 400);
		setMinimumSize(size());
	}
	m_default = settings.value("Overview/Default", true).toBool();
}

//-----------------------------------------------------------------------------

void Overview::load(const QImage& image)
{
	// Remove previous overview
	scene()->clear();

	// Find minimum scale
	float scalex = 400.0f / image.width();
	float scaley = 400.0f / image.width();
	m_min_scale_level = ZoomSlider::scaleLevel(qMin(scalex, scaley));
	zoom(m_min_scale_level);

	// Resize window
	bool default_size = m_default;
	QSize size = transform().mapRect(image.rect()).size();
	setMinimumSize(size);
	if (default_size) {
		resize(minimumSize());
	}

	// Show overview
	QGraphicsPixmapItem* pixmap = scene()->addPixmap(QPixmap::fromImage(image, Qt::AutoColor | Qt::AvoidDither));
	pixmap->setTransformationMode(Qt::SmoothTransformation);
	scene()->setSceneRect(pixmap->boundingRect());
	centerOn(pixmap);
}

//-----------------------------------------------------------------------------

void Overview::moveEvent(QMoveEvent* event)
{
	QSettings().setValue("Overview/Geometry", saveGeometry());
	QGraphicsView::moveEvent(event);
}

//-----------------------------------------------------------------------------

void Overview::hideEvent(QHideEvent* event)
{
	emit toggled(false);
	QGraphicsView::hideEvent(event);
}

//-----------------------------------------------------------------------------

void Overview::resizeEvent(QResizeEvent* event)
{
	m_default = (size() == minimumSize());
	QSettings settings;
	settings.setValue("Overview/Default", m_default);
	settings.setValue("Overview/Geometry", saveGeometry());
	QGraphicsView::resizeEvent(event);
}

//-----------------------------------------------------------------------------

void Overview::showEvent(QShowEvent* event)
{
	emit toggled(true);
	QGraphicsView::showEvent(event);
}

//-----------------------------------------------------------------------------

void Overview::wheelEvent(QWheelEvent* event)
{
	if (event->delta() > 0) {
		zoomIn();
	} else {
		zoomOut();
	}
	event->accept();
}

//-----------------------------------------------------------------------------

void Overview::zoomIn()
{
	zoom(m_scale_level + 1);
}

//-----------------------------------------------------------------------------

void Overview::zoomOut()
{
	zoom(m_scale_level - 1);
}

//-----------------------------------------------------------------------------

void Overview::zoom(int level)
{
	m_scale_level = qBound(m_min_scale_level, level, 9);
	float s = ZoomSlider::scaleFactor(m_scale_level);
	resetTransform();
	scale(s, s);

	Qt::ScrollBarPolicy policy = (m_scale_level > m_min_scale_level) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff;
	setHorizontalScrollBarPolicy(policy);
	setVerticalScrollBarPolicy(policy);
}

//-----------------------------------------------------------------------------
