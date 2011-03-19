/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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
#include <QPainter>

//-----------------------------------------------------------------------------

int powerOfTwo(int value);

//-----------------------------------------------------------------------------

Message::Message(QGLWidget* parent)
	: m_parent(parent),
	m_image(0),
	m_visible(false)
{
}

//-----------------------------------------------------------------------------

Message::~Message()
{
	cleanup();
	vertex_array->release(m_region);
}

//-----------------------------------------------------------------------------

void Message::draw() const
{
	if (m_visible) {
		glBindTexture(GL_TEXTURE_2D, m_image);
		vertex_array->draw(m_region);
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
	m_image = m_parent->bindTexture(image.mirrored(false, true));

	updateVerts();
}

//-----------------------------------------------------------------------------

void Message::setViewport(const QSize& size)
{
	m_viewport = size;
	updateVerts();
}

//-----------------------------------------------------------------------------

void Message::setVisible(bool visible)
{
	m_visible = visible;
	if (m_visible) {
		m_parent->updateGL();
		QApplication::processEvents();
	}
}

//-----------------------------------------------------------------------------

void Message::cleanup()
{
	if (m_image) {
		m_parent->deleteTexture(m_image);
	}
}

//-----------------------------------------------------------------------------

void Message::updateVerts()
{
	int x1 = (m_viewport.width() - m_size.width()) / 2;
	int y1 = (m_viewport.height() - m_size.height()) / 2;
	int x2 = x1 + m_size.width();
	int y2 = y1 + m_size.height();
	int z = 3990;

	QVector<Vertex> verts;
	verts.append( Vertex(x1,y1,z, 0,0) );
	verts.append( Vertex(x1,y2,z, 0,1) );
	verts.append( Vertex(x2,y2,z, 1,1) );
	verts.append( Vertex(x2,y1,z, 1,0) );
	vertex_array->insert(m_region, verts);
}

//-----------------------------------------------------------------------------
