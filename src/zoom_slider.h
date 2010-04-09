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

#ifndef ZOOM_SLIDER_H
#define ZOOM_SLIDER_H

#include <QWidget>
class QLabel;
class QSlider;

class ZoomSlider : public QWidget
{
	Q_OBJECT
public:
	ZoomSlider(QWidget* parent = 0);

signals:
	void valueChanged(int);

public slots:
	void setValue(int level, float factor);

private:
	QLabel* m_label;
	QSlider* m_slider;
};

#endif
