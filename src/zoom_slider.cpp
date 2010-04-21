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

#include "zoom_slider.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>

//-----------------------------------------------------------------------------

namespace
{

const float scale_levels[] = { 0.125, 0.15625, 0.1875, 0.25, 0.3125, 0.40625, 0.5, 0.625, 0.78125, 1.0 };

}

//-----------------------------------------------------------------------------

ZoomSlider::ZoomSlider(QWidget* parent)
	: QWidget(parent)
{
	m_label = new QLabel(tr("??%"), this);
	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setRange(0, 9);
	m_slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
	connect(m_slider, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(m_label);
	layout->addWidget(m_slider);
}

//-----------------------------------------------------------------------------

float ZoomSlider::scaleFactor(int level)
{
	return scale_levels[qBound(0, level, 9)];
}

//-----------------------------------------------------------------------------

int ZoomSlider::scaleLevel(float factor)
{
	factor = qBound(0.0f, factor, 1.0f);
	int level = 9;
	for (int i = 0; i < 9; ++i) {
		if ((factor - scale_levels[i]) < 0.0f) {
			level = (i - 1);
			break;
		}
	}
	return level;
}

//-----------------------------------------------------------------------------

void ZoomSlider::setValue(int level, float factor)
{
	m_slider->setValue(level);
	m_label->setText(tr("%1%").arg(qRound(factor * 100)));
}

//-----------------------------------------------------------------------------