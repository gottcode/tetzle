/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011, 2012, 2015, 2016 Graeme Gott <graeme@gottcode.org>
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

#include "message.h"

#include <QApplication>
#include <QOpenGLTexture>
#include <QPainter>
#include <QTimeLine>
#include <QTimer>
#include <QWidget>

//-----------------------------------------------------------------------------

int powerOfTwo(int value);

//-----------------------------------------------------------------------------

Message::Message(QWidget* parent) :
	QObject(parent),
	m_parent(parent),
	m_image(nullptr),
	m_visible(false),
	m_color(Qt::white)
{
	m_hide_timer = new QTimer(this);
	m_hide_timer->setInterval(2000);
	m_hide_timer->setSingleShot(true);
	connect(m_hide_timer, &QTimer::timeout, this, &Message::hide);

	m_fade_timer = new QTimeLine(160, this);
	m_fade_timer->setCurveShape(QTimeLine::LinearCurve);
	m_fade_timer->setDirection(QTimeLine::Backward);
	m_fade_timer->setFrameRange(0, 10);
	m_fade_timer->setUpdateInterval(16);
	connect(m_fade_timer, &QTimeLine::frameChanged, this, &Message::fade);
}

//-----------------------------------------------------------------------------

Message::~Message()
{
	cleanup();
	graphics_layer->removeArray(m_array);
}

//-----------------------------------------------------------------------------

void Message::draw() const
{
	if (m_image && (m_visible || (m_fade_timer->state() == QTimeLine::Running))) {
		graphics_layer->setColor(m_color);
		graphics_layer->bindTexture(0, m_image->textureId());
		graphics_layer->draw(m_array);
		graphics_layer->setColor(Qt::white);
	}
}

//-----------------------------------------------------------------------------

void Message::setText(const QString& text)
{
	// Don't change if the text is the same
	if (text == m_text) {
		return;
	}

	cleanup();
	m_text = text;

	// Find texture size
	QFont font("Sans", 24);
	QFontMetrics metrics(font);
	int width = metrics.width(m_text);
	int height = metrics.height();
	QSize size(width + height, height * 2);
	m_size = QSize(powerOfTwo(size.width()), powerOfTwo(size.height()));

	// Create texture
	QImage image(m_size, QImage::Format_ARGB32);
	image.fill(Qt::transparent);
	{
		QPainter painter(&image);
		painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
		painter.translate((m_size.width() - size.width()) / 2, (m_size.height() - size.height()) / 2);

		painter.setPen(Qt::NoPen);
		painter.setBrush(QColor(0, 0, 0, 200));
		painter.drawRoundedRect(0, 0, width + height, height * 2, 10, 10);

		painter.setFont(font);
		painter.setPen(Qt::white);
		painter.drawText(height / 2, height / 2 + metrics.ascent(), m_text);
	}
	m_image = new QOpenGLTexture(image);
	m_image->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	m_image->setMagnificationFilter(QOpenGLTexture::Linear);

	updateVerts();
}

//-----------------------------------------------------------------------------

void Message::setViewport(const QSize& size)
{
	m_viewport = size;
	updateVerts();
}

//-----------------------------------------------------------------------------

void Message::setVisible(bool visible, bool stay)
{
	m_visible = visible;

	m_color.setAlpha(255);
	if (m_visible) {
		m_fade_timer->stop();
	} else {
		m_fade_timer->start();
	}

	if (m_visible && !stay) {
		m_hide_timer->start();
	} else {
		m_hide_timer->stop();
	}

	m_parent->update();
	QApplication::processEvents();
}

//-----------------------------------------------------------------------------

void Message::hide()
{
	setVisible(false);
}

//-----------------------------------------------------------------------------

void Message::show()
{
	setVisible(true);
}

//-----------------------------------------------------------------------------

void Message::cleanup()
{
	delete m_image;
	m_image = nullptr;
}

//-----------------------------------------------------------------------------

void Message::updateVerts()
{
	int x1 = (m_viewport.width() - m_size.width()) / 2;
	int y1 = (m_viewport.height() - m_size.height()) / 2;
	int x2 = x1 + m_size.width();
	int y2 = y1 + m_size.height();
	int z = 3990;

	graphics_layer->updateArray(m_array,
	{
		Vertex::init(x1,y1,z, 0,0),
		Vertex::init(x1,y2,z, 0,1),
		Vertex::init(x2,y1,z, 1,0),
		Vertex::init(x2,y1,z, 1,0),
		Vertex::init(x1,y2,z, 0,1),
		Vertex::init(x2,y2,z, 1,1)
	});
}

//-----------------------------------------------------------------------------

void Message::fade(int frame)
{
	m_color.setAlpha(frame * 25);
	m_parent->update();
	QApplication::processEvents();
}

//-----------------------------------------------------------------------------
