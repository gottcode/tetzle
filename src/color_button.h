/*
	SPDX-FileCopyrightText: 2008-2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_COLOR_BUTTON_H
#define TETZLE_COLOR_BUTTON_H

#include <QColor>
#include <QPushButton>

class ColorButton : public QPushButton
{
	Q_OBJECT

public:
	explicit ColorButton(QWidget* parent = 0);

	QColor color() const;
	QString toString() const;

public slots:
	void setColor(const QColor& color);

signals:
	void changed(const QColor& color);

private slots:
	void chooseColor();

private:
	QColor m_color;
};


inline QColor ColorButton::color() const
{
	return m_color;
}

inline QString ColorButton::toString() const
{
	return m_color.name();
}

#endif // TETZLE_COLOR_BUTTON_H
