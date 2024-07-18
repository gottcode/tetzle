/*
	SPDX-FileCopyrightText: 2010-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_MESSAGE_H
#define TETZLE_MESSAGE_H

#include <QColor>
#include <QObject>
class QPainter;
class QTimeLine;
class QTimer;
class QWidget;

class Message : public QObject
{
	Q_OBJECT

public:
	explicit Message(QWidget* parent);

	void draw(QPainter& painter) const;
	void setText(const QString& text);
	void setVisible(bool visible, bool stay = true);

public Q_SLOTS:
	void hide();
	void show();

private Q_SLOTS:
	void fade(int frame);

private:
	QWidget* m_parent;
	QTimer* m_hide_timer;
	QTimeLine* m_fade_timer;

	QString m_text;
	bool m_visible;

	qreal m_opacity;
};

#endif // TETZLE_MESSAGE_H
