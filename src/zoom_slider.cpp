/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "zoom_slider.h"

#include <QHBoxLayout>
#include <QSlider>

#include <cmath>

//-----------------------------------------------------------------------------

namespace
{
	const float scale_levels[10] = { 0.0625, 0.09375, 0.125, 0.171875, 0.234375, 0.3125, 0.40625, 0.546875, 0.75, 1.0 };
}

//-----------------------------------------------------------------------------

ZoomSlider::ZoomSlider(QWidget* parent)
	: QWidget(parent)
{
	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setRange(0, 9);
	m_slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
	connect(m_slider, &QSlider::valueChanged, this, &ZoomSlider::valueChanged);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_slider);
}

//-----------------------------------------------------------------------------

float ZoomSlider::scaleFactor(int level)
{
	return scale_levels[qBound(0, level, 9)];
}

//-----------------------------------------------------------------------------

void ZoomSlider::setValue(int level, float factor)
{
	m_slider->setValue(level);
	m_slider->setToolTip(tr("Zoom: %1%").arg(std::lround(factor * 100)));

	Q_EMIT zoomOutAvailable(level > 0);
	Q_EMIT zoomInAvailable(level < 9);
}

//-----------------------------------------------------------------------------
