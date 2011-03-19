/***********************************************************************
 *
 * Copyright (C) 2011 Graeme Gott <graeme@gottcode.org>
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

#ifndef GRAPHICS_LAYER_H
#define GRAPHICS_LAYER_H

#include <qgl.h>
#include <GL/glext.h>

struct Vertex
{
	GLfloat x;
	GLfloat y;
	GLfloat z;

	GLfloat s;
	GLfloat t;

	GLfloat s2;
	GLfloat t2;

	unsigned char pad[4];

	Vertex(GLfloat x_ = 0, GLfloat y_ = 0, GLfloat z_ = 0, GLfloat s_ = 0, GLfloat t_ = 0, GLfloat s2_ = 0, GLfloat t2_ = 0)
		: x(x_), y(y_), z(z_), s(s_), t(t_), s2(s2_), t2(t2_)
	{
	}
};


struct VertexArray
{
	int start;
	int end;

	VertexArray()
	:	start(0),
		end(0)
	{
	}

	int length() const
	{
		return end - start;
	}

	bool merge(const VertexArray& other)
	{
		if (start == other.end) {
			start = other.start;
			return true;
		} else if (end == other.start) {
			end = other.end;
			return true;
		} else {
			return false;
		}
	}
};


class GraphicsLayer
{
public:
	virtual ~GraphicsLayer();

	void updateArray(VertexArray& array, const QVector<Vertex>& data);
	void removeArray(VertexArray& array);

	virtual void bindTexture(GLenum unit, GLuint texture)=0;
	virtual void draw(const VertexArray& region, GLenum mode = GL_QUADS)=0;
	virtual void setBlended(bool enabled)=0;
	virtual void setColor(const QColor& color)=0;
	virtual void setTextured(bool enabled)=0;
	virtual void setMultiTextured(bool enabled)=0;
	virtual void setModelview(const QMatrix4x4& matrix)=0;
	virtual void setProjection(const QMatrix4x4& matrix)=0;
	virtual void uploadData()=0;

	static void init();

protected:
	GraphicsLayer();

protected:
	void clearChanged();
	void uploadChanged();

	const Vertex& at(int index) const
	{
		return m_data.at(index);
	}

private:
	QVector<Vertex> m_data;
	QList<VertexArray> m_free_regions;
	QList<VertexArray> m_changed_regions;
	bool m_changed;
};
extern GraphicsLayer* graphics_layer;


class GraphicsLayer11 : public GraphicsLayer
{
public:
	GraphicsLayer11();

	virtual void bindTexture(GLenum unit, GLuint texture);
	virtual void draw(const VertexArray& array, GLenum mode = GL_QUADS);
	virtual void setBlended(bool enabled);
	virtual void setColor(const QColor& color);
	virtual void setTextured(bool enabled);
	virtual void setMultiTextured(bool enabled);
	virtual void setModelview(const QMatrix4x4& matrix);
	virtual void setProjection(const QMatrix4x4& matrix);
	virtual void uploadData();
};


class GraphicsLayer13 : public GraphicsLayer11
{
public:
	GraphicsLayer13();

	virtual void bindTexture(GLenum unit, GLuint texture);
	virtual void draw(const VertexArray& array, GLenum mode = GL_QUADS);
	virtual void setMultiTextured(bool enabled);
};


class GraphicsLayer15 : public GraphicsLayer13
{
public:
	GraphicsLayer15();
	virtual ~GraphicsLayer15();

	virtual void draw(const VertexArray& array, GLenum mode = GL_QUADS);
	virtual void setTextured(bool enabled);
	virtual void setMultiTextured(bool enabled);
	virtual void uploadData();

private:
	GLuint m_id;
};

#endif
