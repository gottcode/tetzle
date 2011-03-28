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

#include "graphics_layer.h"

#include "appearance_dialog.h"

#include <QFile>
#include <QGLShaderProgram>

//-----------------------------------------------------------------------------

GraphicsLayer* graphics_layer = 0;

//-----------------------------------------------------------------------------

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

// Multi-texture extension
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif

typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
static PFNGLACTIVETEXTUREPROC activeTexture = 0;
typedef void (APIENTRYP PFNGLCLIENTACTIVETEXTUREPROC) (GLenum texture);
static PFNGLCLIENTACTIVETEXTUREPROC clientActiveTexture = 0;

// Vertex buffer object extension
static GLuint vbo_id = 0;

typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
static PFNGLBINDBUFFERPROC bindBuffer = 0;
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
static PFNGLBUFFERDATAPROC bufferData = 0;
typedef void (APIENTRYP PFNGLBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
static PFNGLBUFFERSUBDATAPROC bufferSubData = 0;
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
static PFNGLDELETEBUFFERSPROC deleteBuffers = 0;
typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
static PFNGLGENBUFFERSPROC genBuffers = 0;

// Vertex attribute object extension
static GLuint vao_id = 0;

typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
static PFNGLBINDVERTEXARRAYPROC bindVertexArray = 0;
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);
static PFNGLDELETEVERTEXARRAYSPROC deleteVertexArrays = 0;
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
static PFNGLGENVERTEXARRAYSPROC genVertexArrays = 0;

static void* getProcAddress(const QString& name)
{
	void* result;
	QString names[] = { name, name + "ARB", name + "EXT" };
	for (int i = 0; i < 3; ++i) {
		result = QGLContext::currentContext()->getProcAddress(names[i]);
		if (result) {
			break;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------

void GraphicsLayer::init()
{
	QStringList extensions = QString(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS))).split(' ');
	unsigned int state = 0;

	// Load multi-texture extension
	if (extensions.contains("GL_ARB_multitexture")) {
		state |= 0x01;
		activeTexture = (PFNGLACTIVETEXTUREPROC) getProcAddress("glActiveTexture");
		clientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC) getProcAddress("glClientActiveTexture");
	}

	// Load vertex buffer object extension
	if (extensions.contains("GL_ARB_vertex_buffer_object")) {
		state |= 0x02;
		bindBuffer = (PFNGLBINDBUFFERPROC) getProcAddress("glBindBuffer");
		bufferData = (PFNGLBUFFERDATAPROC) getProcAddress("glBufferData");
		bufferSubData = (PFNGLBUFFERSUBDATAPROC) getProcAddress("glBufferSubData");
		deleteBuffers = (PFNGLDELETEBUFFERSPROC) getProcAddress("glDeleteBuffers");
		genBuffers = (PFNGLGENBUFFERSPROC) getProcAddress("glGenBuffers");
	}

	// Check for minimum supported programmable pipeline
	if (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_2_1) {
		state |= 0x04;
	}

	// Load vertex array object extension
	if (extensions.contains("GL_ARB_vertex_array_object")) {
		bindVertexArray = (PFNGLBINDVERTEXARRAYPROC) getProcAddress("glBindVertexArray");
		deleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) getProcAddress("glDeleteVertexArrays");
		genVertexArrays = (PFNGLGENVERTEXARRAYSPROC) getProcAddress("glGenVertexArrays");

		genVertexArrays(1, &vao_id);
		bindVertexArray(vao_id);
	}

	// Create graphics layer instance
	switch (state) {
	case 0x07:
		graphics_layer = new GraphicsLayer21;
		break;
	case 0x03:
		graphics_layer = new GraphicsLayer15;
		break;
	case 0x01:
		graphics_layer = new GraphicsLayer13;
		break;
	default:
		graphics_layer = new GraphicsLayer11;
		break;
	}
	graphics_layer->setTextureUnits(1);
}

//-----------------------------------------------------------------------------

GraphicsLayer::GraphicsLayer()
:	m_changed(true)
{
	glDisable(GL_BLEND);

	// Enable OpenGL features
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// Set OpenGL parameters
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glFrontFace(GL_CCW);
}

//-----------------------------------------------------------------------------

GraphicsLayer::~GraphicsLayer()
{
	// Delete VBO
	if (vbo_id != 0) {
		bindBuffer(GL_ARRAY_BUFFER, 0);
		deleteBuffers(1, &vbo_id);
		vbo_id = 0;
	}

	// Delete VAO
	if (vao_id != 0) {
		bindVertexArray(0);
		deleteVertexArrays(1, &vao_id);
		vao_id = 0;
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer::updateArray(VertexArray& array, const QVector<Vertex>& data)
{
	int length = data.count();
	if (array.length() != length) {
		if (array.end > 0) {
			removeArray(array);
		}
		array.start = -1;
		for (int i = 0; i < m_free_regions.count(); ++i) {
			VertexArray& free_region = m_free_regions[i];
			if (free_region.length() == length) {
				array.start = free_region.start;
				array.end = array.start + length;
				m_free_regions.removeAt(i);
				break;
			} else if (free_region.length() > length) {
				array.start = free_region.start;
				array.end = array.start + length;
				free_region.start += length;
				break;
			}
		}
	}

	if (array.start != -1) {
		qCopy(data.begin(), data.end(), m_data.begin() + array.start);
		if (!m_changed) {
			m_changed_regions.append(array);
		}
	} else {
		array.start = m_data.count();
		array.end = array.start + length;
		m_data += data;
		m_changed = true;
		m_changed_regions.clear();
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer::removeArray(VertexArray& array)
{
	bool merged = false;
	int count = m_free_regions.count();
	int pos = count;
	for (int i = 0; i < count; ++i) {
		VertexArray& free_region = m_free_regions[i];
		if (free_region.end == array.start) {
			merged = true;
			free_region.end = array.end;
			if ((i+1) < count && free_region.end == m_free_regions[i+1].start) {
				free_region.end = m_free_regions[i+1].end;
				m_free_regions.removeAt(i+1);
			}
			break;
		} else if (free_region.start == array.end) {
			merged = true;
			free_region.start = array.start;
			if (i > 0 && free_region.start == m_free_regions[i-1].end) {
				free_region.start = m_free_regions[i-1].start;
				m_free_regions.removeAt(i-1);
			}
			break;
		} else if (free_region.start > array.start) {
			pos = i;
			break;
		}
	}
	if (!merged) {
		m_free_regions.insert(pos, array);
	}
	array.start = array.end = 0;
}

//-----------------------------------------------------------------------------

void GraphicsLayer::clearChanged()
{
	m_changed_regions.clear();
	m_changed = false;
}

//-----------------------------------------------------------------------------

void GraphicsLayer::uploadChanged()
{
	if (!m_changed_regions.isEmpty()) {
		foreach (const VertexArray& region, m_changed_regions) {
			bufferSubData(GL_ARRAY_BUFFER, region.start * sizeof(Vertex), region.length() * sizeof(Vertex), &m_data.at(region.start));
		}
		m_changed_regions.clear();
	} else if (m_changed) {
		GLsizeiptr size = m_data.count() * sizeof(Vertex);
		bufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
		bufferSubData(GL_ARRAY_BUFFER, 0, size, m_data.constData());
		m_changed = false;
	}
}

//-----------------------------------------------------------------------------

GraphicsLayer21::GraphicsLayer21()
:	m_program(0)
{
	AppearanceDialog::setBevelsEnabled(true);

	// Create VBO
	genBuffers(1, &vbo_id);
	bindBuffer(GL_ARRAY_BUFFER, vbo_id);

	// Load shaders
	QGLShaderProgram* program = loadProgram(0);
	program->setAttributeBuffer(TexCoord1, GL_FLOAT, sizeof(GLfloat) * 5, 2, sizeof(Vertex));
	program->setAttributeBuffer(TexCoord0, GL_FLOAT, sizeof(GLfloat) * 3, 2, sizeof(Vertex));
	program->setAttributeBuffer(Position, GL_FLOAT, 0, 3, sizeof(Vertex));
	program->enableAttributeArray(Position);

	program = loadProgram(1);
	program->setUniformValue("texture0", GLuint(0));

	program = loadProgram(2);
	program->setUniformValue("texture0", GLuint(0));
	program->setUniformValue("texture1", GLuint(1));
}

//-----------------------------------------------------------------------------

GraphicsLayer21::~GraphicsLayer21()
{
	// Unload shaders
	for (int i = 0; i < 3; ++i) {
		delete m_programs[i];
		m_programs[i] = 0;
	}
	m_program = 0;
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::bindTexture(unsigned int unit, GLuint texture)
{
	activeTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	activeTexture(GL_TEXTURE0);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::draw(const VertexArray& array, GLenum mode)
{
	glDrawArrays(mode, array.start, array.length());
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setBlended(bool enabled)
{
	if (enabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setColor(const QColor& color)
{
	m_program->setUniformValue(m_color, color);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setModelview(const QMatrix4x4& matrix)
{
	m_modelview = matrix;
	m_program->setUniformValue(m_matrix, m_projection * m_modelview);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setProjection(const QMatrix4x4& matrix)
{
	m_projection = matrix;
	m_program->setUniformValue(m_matrix, m_projection * m_modelview);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setTextureUnits(unsigned int units)
{
	Q_ASSERT(units < 3);
	QGLShaderProgram* program = m_programs[units];
	if (m_program == program) {
		return;
	}

	m_program = program;
	m_program->bind();

	m_color = m_program->uniformLocation("color");
	m_matrix = m_program->uniformLocation("matrix");
	m_program->setUniformValue(m_matrix, m_projection * m_modelview);

	if (units > 1) {
		m_program->enableAttributeArray(TexCoord1);
		m_program->enableAttributeArray(TexCoord0);
	} else if (units > 0) {
		m_program->disableAttributeArray(TexCoord1);
		m_program->enableAttributeArray(TexCoord0);
	} else {
		m_program->disableAttributeArray(TexCoord1);
		m_program->disableAttributeArray(TexCoord0);
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::uploadData()
{
	uploadChanged();
}

//-----------------------------------------------------------------------------

QGLShaderProgram* GraphicsLayer21::loadProgram(unsigned int index)
{
	// Load vertex shader code
	QString vertex;
	QFile file(QString(":/shaders/textures%1.vert").arg(index));
	if (file.open(QFile::ReadOnly)) {
		vertex = file.readAll();
		file.close();
	}

	// Load fragment shader code
	QString frag;
	file.setFileName(QString(":/shaders/textures%1.frag").arg(index));
	if (file.open(QFile::ReadOnly)) {
		frag = file.readAll();
		file.close();
	}

	// Update GLSL version
	QString version;
	version = (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_3_0) ? "130" : version;
#if QT_VERSION >= 0x040700
	version = (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_3_1) ? "140" : version;
	version = (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_3_2) ? "150" : version;
	version = (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_3_3) ? "330" : version;
	version = (QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_4_0) ? "400" : version;
#endif
	if (!version.isEmpty()) {
		vertex.replace("#version 120\n", "#version " + version + "\n");
		vertex.replace("attribute ", "in ");
		vertex.replace("varying ", "out ");

		frag.replace("#version 120\n", "#version " + version + "\n\nout vec4 out_color;\n");
		frag.replace("gl_FragColor", "out_color");
		frag.replace("varying ", "in ");
	}

	// Create program
	m_programs[index] = new QGLShaderProgram;
	m_programs[index]->addShaderFromSourceCode(QGLShader::Vertex, vertex);
	m_programs[index]->addShaderFromSourceCode(QGLShader::Fragment, frag);

	// Set attribute locations
	m_programs[index]->bindAttributeLocation("position", Position);
	if (index > 0) {
		m_programs[index]->bindAttributeLocation("texcoord0", TexCoord0);
	}
	if (index > 1) {
		m_programs[index]->bindAttributeLocation("texcoord1", TexCoord1);
	}

	// Link and bind program
	m_programs[index]->link();
	m_programs[index]->bind();
	return m_programs[index];
}

//-----------------------------------------------------------------------------

GraphicsLayer11::GraphicsLayer11()
{
	AppearanceDialog::setBevelsEnabled(false);

	// Disable unused OpenGL features
	glDisable(GL_LIGHTING);

	// Enable OpenGL features
	glEnableClientState(GL_VERTEX_ARRAY);

	// Set OpenGL parameters
	setColor(Qt::white);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::bindTexture(unsigned int unit, GLuint texture)
{
	Q_UNUSED(unit);
	glBindTexture(GL_TEXTURE_2D, texture);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::draw(const VertexArray& array, GLenum mode)
{
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(array.start).s);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &at(array.start).x);
	glDrawArrays(mode, 0, array.length());
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setBlended(bool enabled)
{
	if (enabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setColor(const QColor& color)
{
	glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setModelview(const QMatrix4x4& matrix)
{
	glLoadMatrixd(matrix.constData());
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setProjection(const QMatrix4x4& matrix)
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(matrix.constData());
	glMatrixMode(GL_MODELVIEW);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setTextureUnits(unsigned int units)
{
	if (units) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::uploadData()
{
	clearChanged();
}

//-----------------------------------------------------------------------------

GraphicsLayer13::GraphicsLayer13()
{
	AppearanceDialog::setBevelsEnabled(true);

	// Set OpenGL parameters
	activeTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED);
	activeTexture(GL_TEXTURE0);
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::bindTexture(unsigned int unit, GLuint texture)
{
	activeTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	activeTexture(GL_TEXTURE0);
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::draw(const VertexArray& array, GLenum mode)
{
	clientActiveTexture(GL_TEXTURE1);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(array.start).s2);
	clientActiveTexture(GL_TEXTURE0);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(array.start).s);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &at(array.start).x);
	glDrawArrays(mode, 0, array.length());
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::setTextureUnits(unsigned int units)
{
	activeTexture(GL_TEXTURE1);
	clientActiveTexture(GL_TEXTURE1);
	if (units > 1) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	activeTexture(GL_TEXTURE0);
	clientActiveTexture(GL_TEXTURE0);
	if (units > 0) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}
}

//-----------------------------------------------------------------------------

GraphicsLayer15::GraphicsLayer15()
{
	// Create VBO
	genBuffers(1, &vbo_id);
	bindBuffer(GL_ARRAY_BUFFER, vbo_id);
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::draw(const VertexArray& array, GLenum mode)
{
	glDrawArrays(mode, array.start, array.length());
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::setTextureUnits(unsigned int units)
{
	activeTexture(GL_TEXTURE1);
	clientActiveTexture(GL_TEXTURE1);
	if (units > 1) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 5));
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	activeTexture(GL_TEXTURE0);
	clientActiveTexture(GL_TEXTURE0);
	if (units > 0) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::uploadData()
{
	uploadChanged();
}

//-----------------------------------------------------------------------------
