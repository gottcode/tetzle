/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

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
	explicit ZoomSlider(QWidget* parent = nullptr);

	static float scaleFactor(int level);

Q_SIGNALS:
	void valueChanged(int);
	void zoomInAvailable(bool available);
	void zoomOutAvailable(bool available);

public Q_SLOTS:
	void setValue(int level, float factor);

private:
	QLabel* m_label;
	QSlider* m_slider;
};

#endif // TETZLE_ZOOM_SLIDER_H
