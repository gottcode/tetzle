/***********************************************************************
 *
 * Copyright (C) 2011, 2012, 2016, 2018 Graeme Gott <graeme@gottcode.org>
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

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_1_1>
#include <QOpenGLFunctions_1_3>
class QOpenGLBuffer;
class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;

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

	static Vertex init(GLfloat x_ = 0, GLfloat y_ = 0, GLfloat z_ = 0, GLfloat s_ = 0, GLfloat t_ = 0, GLfloat s2_ = 0, GLfloat t2_ = 0)
	{
		Vertex result = { x_, y_, z_, s_, t_, s2_, t2_, {0,0,0,0} };
		return result;
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
};


class GraphicsLayer
{
public:
	virtual ~GraphicsLayer();

	static void init();
	static void setVersion(int version);

	void updateArray(VertexArray& array, const QVector<Vertex>& data);
	void removeArray(VertexArray& array);

	virtual void bindTexture(unsigned int unit, GLuint texture)=0;
	virtual void clear()=0;
	virtual void draw(const VertexArray& region, GLenum mode = GL_TRIANGLES)=0;
	virtual GLint getMaxTextureSize()=0;
	virtual void setBlended(bool enabled)=0;
	virtual void setClearColor(const QColor& color)=0;
	virtual void setColor(const QColor& color)=0;
	virtual void setModelview(const QMatrix4x4& matrix)=0;
	virtual void setProjection(const QMatrix4x4& matrix)=0;
	virtual void setTextureUnits(unsigned int units)=0;
	virtual void setViewport(GLint x, GLint y, GLsizei width, GLsizei height)=0;
	virtual void uploadData()=0;

protected:
	GraphicsLayer();

protected:
	void clearChanged();
	void uploadChanged(QOpenGLBuffer* vertex_buffer);

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


// Programmable pipeline
class GraphicsLayer21 : public GraphicsLayer, protected QOpenGLFunctions
{
public:
	GraphicsLayer21(QOpenGLVertexArrayObject* vertex_array, const QByteArray& glsl, const QString& shader);
	~GraphicsLayer21();

	virtual void bindTexture(unsigned int unit, GLuint texture);
	virtual void clear();
	virtual void draw(const VertexArray& array, GLenum mode = GL_TRIANGLES);
	virtual GLint getMaxTextureSize();
	virtual void setBlended(bool enabled);
	virtual void setClearColor(const QColor& color);
	virtual void setColor(const QColor& color);
	virtual void setModelview(const QMatrix4x4& matrix);
	virtual void setProjection(const QMatrix4x4& matrix);
	virtual void setTextureUnits(unsigned int units);
	virtual void setViewport(GLint x, GLint y, GLsizei width, GLsizei height);
	virtual void uploadData();

private:
	QOpenGLShaderProgram* loadProgram(unsigned int index, const QByteArray& glsl, const QString& shader);

private:
	enum Attribute
	{
		Position = 0,
		TexCoord0,
		TexCoord1
	};

	QMatrix4x4 m_modelview;
	QMatrix4x4 m_projection;
	GLfloat m_matrix[4][4];

	QOpenGLShaderProgram* m_program;
	QOpenGLShaderProgram* m_programs[3];
	int m_color_location;
	int m_matrix_location;

	QOpenGLVertexArrayObject* m_vertex_array;
	QOpenGLBuffer* m_vertex_buffer;
};


#ifndef QT_OPENGL_ES_2
// Fixed function pipeline
class GraphicsLayer11 : public GraphicsLayer, protected QOpenGLFunctions_1_1
{
public:
	GraphicsLayer11();

	virtual void bindTexture(unsigned int unit, GLuint texture);
	virtual void clear();
	virtual void draw(const VertexArray& array, GLenum mode = GL_TRIANGLES);
	virtual GLint getMaxTextureSize();
	virtual void setBlended(bool enabled);
	virtual void setClearColor(const QColor& color);
	virtual void setColor(const QColor& color);
	virtual void setModelview(const QMatrix4x4& matrix);
	virtual void setProjection(const QMatrix4x4& matrix);
	virtual void setTextureUnits(unsigned int units);
	virtual void setViewport(GLint x, GLint y, GLsizei width, GLsizei height);
	virtual void uploadData();
};


class GraphicsLayer13 : public GraphicsLayer, protected QOpenGLFunctions_1_3
{
public:
	GraphicsLayer13();

	virtual void bindTexture(unsigned int unit, GLuint texture);
	virtual void clear();
	virtual void draw(const VertexArray& array, GLenum mode = GL_TRIANGLES);
	virtual GLint getMaxTextureSize();
	virtual void setBlended(bool enabled);
	virtual void setClearColor(const QColor& color);
	virtual void setColor(const QColor& color);
	virtual void setModelview(const QMatrix4x4& matrix);
	virtual void setProjection(const QMatrix4x4& matrix);
	virtual void setTextureUnits(unsigned int units);
	virtual void setViewport(GLint x, GLint y, GLsizei width, GLsizei height);
	virtual void uploadData();
};


class GraphicsLayer15 : public GraphicsLayer13
{
public:
	GraphicsLayer15();
	~GraphicsLayer15();

	virtual void draw(const VertexArray& array, GLenum mode = GL_TRIANGLES);
	virtual void setTextureUnits(unsigned int units);
	virtual void uploadData();

private:
	QOpenGLBuffer* m_vertex_buffer;
};
#endif

#endif
