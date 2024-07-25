/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "zoom_slider.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QSlider>
#include <QToolButton>

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------

namespace
{

constexpr float scale_levels[10] = { 0.0625, 0.09375, 0.125, 0.171875, 0.234375, 0.3125, 0.40625, 0.546875, 0.75, 1.0 };

QString windowString(const char* text)
{
	QString result = QCoreApplication::translate("Window", text);
	result.remove('&');
	return result;
}

}

//-----------------------------------------------------------------------------

ZoomSlider::ZoomSlider(QWidget* parent)
	: QWidget(parent)
{
	QToolButton* zoom_fit = new QToolButton(this);
	zoom_fit->setAutoRaise(true);
	zoom_fit->setIcon(QIcon::fromTheme("zoom-fit-best"));
	zoom_fit->setToolTip(windowString("Best &Fit"));
	connect(zoom_fit, &QToolButton::clicked, this, &ZoomSlider::zoomFit);

	m_zoom_out = new QToolButton(this);
	m_zoom_out->setAutoRaise(true);
	m_zoom_out->setIcon(QIcon::fromTheme("zoom-out"));
	m_zoom_out->setToolTip(windowString("Zoom &Out"));
	m_zoom_out->setEnabled(false);
	connect(m_zoom_out, &QToolButton::clicked, this, &ZoomSlider::zoomOut);
	connect(this, &ZoomSlider::zoomOutAvailable, m_zoom_out, &QToolButton::setEnabled);

	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setRange(0, maxScaleLevel());
	m_slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
	connect(m_slider, &QSlider::valueChanged, this, &ZoomSlider::valueChanged);

	m_zoom_in = new QToolButton(this);
	m_zoom_in->setAutoRaise(true);
	m_zoom_in->setIcon(QIcon::fromTheme("zoom-in"));
	m_zoom_in->setToolTip(windowString("Zoom &In"));
	m_zoom_in->setEnabled(false);
	connect(m_zoom_in, &QToolButton::clicked, this, &ZoomSlider::zoomIn);
	connect(this, &ZoomSlider::zoomInAvailable, m_zoom_in, &QToolButton::setEnabled);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(zoom_fit);
	layout->addWidget(m_zoom_out);
	layout->addWidget(m_slider);
	layout->addWidget(m_zoom_in);
}

//-----------------------------------------------------------------------------

float ZoomSlider::scaleFactor(int level)
{
	return scale_levels[std::clamp(level, 0, maxScaleLevel())];
}

//-----------------------------------------------------------------------------

void ZoomSlider::setValue(int level)
{
	m_slider->setValue(level);
	m_slider->setToolTip(tr("Zoom: %1%").arg(std::lround(scaleFactor(level) * 100)));

	Q_EMIT zoomOutAvailable(level > 0);
	Q_EMIT zoomInAvailable(level < maxScaleLevel());
}

//-----------------------------------------------------------------------------
