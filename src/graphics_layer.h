/*
	SPDX-FileCopyrightText: 2011-2018 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_GRAPHICS_LAYER_H
#define TETZLE_GRAPHICS_LAYER_H

#include <QMatrix4x4>
#include <QOpenGLFunctions>
#ifndef QT_OPENGL_ES_2
#include <QOpenGLFunctions_1_1>
#include <QOpenGLFunctions_1_3>
#endif
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
		: start(0)
		, end(0)
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

	void updateArray(VertexArray& array, const QList<Vertex>& data);
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
	explicit GraphicsLayer();

protected:
	void clearChanged();
	void uploadChanged(QOpenGLBuffer* vertex_buffer);

	const Vertex& at(int index) const
	{
		return m_data.at(index);
	}

private:
	QList<Vertex> m_data;
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

	void bindTexture(unsigned int unit, GLuint texture) override;
	void clear() override;
	void draw(const VertexArray& array, GLenum mode = GL_TRIANGLES) override;
	GLint getMaxTextureSize() override;
	void setBlended(bool enabled) override;
	void setClearColor(const QColor& color) override;
	void setColor(const QColor& color) override;
	void setModelview(const QMatrix4x4& matrix) override;
	void setProjection(const QMatrix4x4& matrix) override;
	void setTextureUnits(unsigned int units) override;
	void setViewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
	void uploadData() override;

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
	explicit GraphicsLayer11();

	void bindTexture(unsigned int unit, GLuint texture) override;
	void clear() override;
	void draw(const VertexArray& array, GLenum mode = GL_TRIANGLES) override;
	GLint getMaxTextureSize() override;
	void setBlended(bool enabled) override;
	void setClearColor(const QColor& color) override;
	void setColor(const QColor& color) override;
	void setModelview(const QMatrix4x4& matrix) override;
	void setProjection(const QMatrix4x4& matrix) override;
	void setTextureUnits(unsigned int units) override;
	void setViewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
	void uploadData() override;
};


class GraphicsLayer13 : public GraphicsLayer, protected QOpenGLFunctions_1_3
{
public:
	explicit GraphicsLayer13();

	void bindTexture(unsigned int unit, GLuint texture) override;
	void clear() override;
	void draw(const VertexArray& array, GLenum mode = GL_TRIANGLES) override;
	GLint getMaxTextureSize() override;
	void setBlended(bool enabled) override;
	void setClearColor(const QColor& color) override;
	void setColor(const QColor& color) override;
	void setModelview(const QMatrix4x4& matrix) override;
	void setProjection(const QMatrix4x4& matrix) override;
	void setTextureUnits(unsigned int units) override;
	void setViewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
	void uploadData() override;
};


class GraphicsLayer15 : public GraphicsLayer13
{
public:
	explicit GraphicsLayer15();
	~GraphicsLayer15();

	void draw(const VertexArray& array, GLenum mode = GL_TRIANGLES) override;
	void setTextureUnits(unsigned int units) override;
	void uploadData() override;

private:
	QOpenGLBuffer* m_vertex_buffer;
};
#endif

#endif // TETZLE_GRAPHICS_LAYER_H
