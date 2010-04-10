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

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QSettings>
#include <QWheelEvent>

//-----------------------------------------------------------------------------

Overview::Overview(QWidget* parent)
	: QGraphicsView(parent),
	m_scale_start(0),
	m_scale_factor(0),
	m_scale_level(0)
{
	setWindowTitle(tr("Overview"));
	setWindowFlags(Qt::Tool);

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
	bool default_size = m_default;
	setMinimumSize(size);
	if (default_size) {
		resize(minimumSize());
	}

	// Show overview
	QGraphicsPixmapItem* pixmap = scene()->addPixmap(QPixmap::fromImage(image, Qt::AutoColor | Qt::AvoidDither));
	pixmap->setTransformationMode(Qt::SmoothTransformation);
	scene()->setSceneRect(pixmap->boundingRect());
	zoom();
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
	if (m_scale_level < 9) {
		m_scale_level++;
		zoom();
	}
}

//-----------------------------------------------------------------------------

void Overview::zoomOut()
{
	if (m_scale_level > 0) {
		m_scale_level--;
		zoom();
	}
}

//-----------------------------------------------------------------------------

void Overview::zoom()
{
	float s = m_scale_start + (m_scale_factor * m_scale_level);
	setMatrix(QMatrix().scale(s, s));
}

//-----------------------------------------------------------------------------
