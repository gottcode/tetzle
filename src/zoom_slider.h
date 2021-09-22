/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_ZOOM_SLIDER_H
#define TETZLE_ZOOM_SLIDER_H

#include <QWidget>
class QLabel;
class QSlider;

class ZoomSlider : public QWidget
{
	Q_OBJECT
public:
	ZoomSlider(QWidget* parent = 0);

	static float scaleFactor(int level);

signals:
	void valueChanged(int);

public slots:
	void setValue(int level, float factor);

private:
	QLabel* m_label;
	QSlider* m_slider;
};

#endif // TETZLE_ZOOM_SLIDER_H
