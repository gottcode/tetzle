/*
	SPDX-FileCopyrightText: 2008-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "color_button.h"

#include <QColorDialog>
#include <QPainter>
#include <QPixmap>

//-----------------------------------------------------------------------------

ColorButton::ColorButton(QWidget* parent)
	: QPushButton(parent)
{
	setAutoDefault(false);
	connect(this, &QPushButton::clicked, this, &ColorButton::chooseColor);
}

//-----------------------------------------------------------------------------

void ColorButton::setColor(const QColor& color)
{
	if (m_color == color) {
		return;
	}
	m_color = color;

	QPixmap swatch(75, fontMetrics().height());
	swatch.fill(m_color);
	{
		QPainter painter(&swatch);
		painter.setPen(m_color.darker());
		painter.drawRect(0, 0, swatch.width() - 1, swatch.height() - 1);
		painter.setPen(m_color.lighter());
		painter.drawRect(1, 1, swatch.width() - 3, swatch.height() - 3);
	}
	setIconSize(swatch.size());
	setIcon(swatch);

	Q_EMIT changed(m_color);
}

//-----------------------------------------------------------------------------

void ColorButton::chooseColor()
{
	QColor color = QColorDialog::getColor(m_color, this);
	if (color.isValid()) {
		setColor(color);
	}
}

//-----------------------------------------------------------------------------
