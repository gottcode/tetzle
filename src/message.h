/*
	SPDX-FileCopyrightText: 2010-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_MESSAGE_H
#define TETZLE_MESSAGE_H

#include "graphics_layer.h"

#include <QColor>
class QOpenGLTexture;
class QTimeLine;
class QTimer;
class QWidget;

class Message : public QObject
{
	Q_OBJECT

public:
	explicit Message(QWidget* parent);
	~Message();

	void draw(QPainter& painter) const;
	void setText(const QString& text);
	void setViewport(const QSize& size);
	void setVisible(bool visible, bool stay = true);

public Q_SLOTS:
	void hide();
	void show();

private:
	void cleanup();
	void updateVerts();

private Q_SLOTS:
	void fade(int frame);

private:
	QWidget* m_parent;
	QTimer* m_hide_timer;
	QTimeLine* m_fade_timer;
	QOpenGLTexture* m_image;
	VertexArray m_array;

	QString m_text;
	QSize m_size;
	QSize m_viewport;
	bool m_visible;

	QColor m_color;
};

#endif // TETZLE_MESSAGE_H
