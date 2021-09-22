/*
	SPDX-FileCopyrightText: 2008-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "overview.h"

#include "zoom_slider.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QIcon>
#include <QSettings>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------

Overview::Overview(QWidget* parent)
	: QGraphicsView(parent),
	m_min_scale_level(0),
	m_scale_level(0)
{
	setWindowTitle(tr("Overview"));
	setWindowFlags(Qt::Tool);

	setBackgroundBrush(Qt::darkGray);
	setBackgroundRole(QPalette::Window);
	setRenderHint(QPainter::SmoothPixmapTransform, true);
	setDragMode(ScrollHandDrag);
	setFrameStyle(NoFrame);

	// Create scene
	QGraphicsScene* scene = new QGraphicsScene(this);
	setScene(scene);
	m_pixmap = new QGraphicsPixmapItem;
	m_pixmap->setTransformationMode(Qt::SmoothTransformation);
	scene->addItem(m_pixmap);
	reset();

	// Restore geometry
	QSettings settings;
	if (settings.contains("Overview/Geometry")) {
		restoreGeometry(settings.value("Overview/Geometry").toByteArray());
		setMinimumSize(size());
	} else {
		resize(400, 400);
		setMinimumSize(size());
	}
	m_default = settings.value("Overview/Default", true).toBool();
}

//-----------------------------------------------------------------------------

void Overview::load(const QImage& image, qreal pixelratio)
{
	// Find minimum scale
	m_min_scale_level = 9;
	int side_max = std::max(image.width(), image.height()) * 0.9;
	int side = 400;
	if (side_max > side) {
		for (int i = 9; i >= 0; --i) {
			int side_test = std::floor(400.0 / ZoomSlider::scaleFactor(i));
			if (side_test > side_max) {
				break;
			}
			m_min_scale_level = i;
			side = side_test;
		}
	} else {
		side = side_max;
	}
	QPixmap pixmap = QPixmap::fromImage(image.scaled(side * pixelratio, side * pixelratio, Qt::KeepAspectRatio, Qt::SmoothTransformation), Qt::AutoColor | Qt::AvoidDither);
	pixmap.setDevicePixelRatio(pixelratio);
	zoom(m_min_scale_level);

	// Resize window
	bool default_size = m_default;
	QSize size = transform().mapRect(QRect(QPoint(0,0), pixmap.size() / pixelratio)).size();
	setMinimumSize(size);
	if (default_size) {
		resize(minimumSize());
	}

	// Show overview
	setPixmap(pixmap);
}

//-----------------------------------------------------------------------------

void Overview::reset()
{
	// Prevent zooming
	m_min_scale_level = 9;
	zoom(m_min_scale_level);

	// Show loading icon
	setPixmap(QIcon::fromTheme("image-loading", QIcon(":/tango/image-loading.png")).pixmap(128,128));
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
	if (event->angleDelta().y() > 0) {
		zoomIn();
	} else {
		zoomOut();
	}
	event->accept();
}

//-----------------------------------------------------------------------------

void Overview::setPixmap(const QPixmap& pixmap)
{
	m_pixmap->setPixmap(pixmap);
	scene()->setSceneRect(m_pixmap->boundingRect());
	centerOn(m_pixmap);
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
