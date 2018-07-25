/***********************************************************************
 *
 * Copyright (C) 2011, 2012, 2014, 2016, 2017, 2018 Graeme Gott <graeme@gottcode.org>
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

#include <QCoreApplication>
#include <QFile>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

//-----------------------------------------------------------------------------

GraphicsLayer* graphics_layer = 0;

//-----------------------------------------------------------------------------

template <typename T>
static inline void convertMatrix(const T* in, GLfloat* out)
{
	std::copy(in, in + 16, out);
}

//-----------------------------------------------------------------------------

void GraphicsLayer::init()
{
	const auto requested = QSurfaceFormat::defaultFormat();
	const auto context = QOpenGLContext::currentContext()->format();

	if (!QOpenGLContext::currentContext()->isOpenGLES()) {
		const auto version = std::min((context.profile() == QSurfaceFormat::CoreProfile) ? qMakePair(4,6) : requested.version(), context.version());

		if (version >= qMakePair(3,0)) {
			const QString shader = "gl3";
			QByteArray glsl = "130";
			if (version >= qMakePair(3,3)) {
				glsl = QByteArray::number((version.first * 100) + (version.second * 10));
			} else if (version == qMakePair(3,2)) {
				glsl = "150";
			} else if (version == qMakePair(3,1)) {
				glsl = "140";
			}

			auto vertex_array = new QOpenGLVertexArrayObject;
			vertex_array->create();
			vertex_array->bind();

			graphics_layer = new GraphicsLayer21(vertex_array, glsl, shader);
		} else if (version >= qMakePair(2,1)) {
			const QString shader = "gl2";
			const QByteArray glsl = "120";

			graphics_layer = new GraphicsLayer21(nullptr, glsl, shader);
#ifndef QT_OPENGL_ES_2
		} else if (version >= qMakePair(1,5)) {
			graphics_layer = new GraphicsLayer15;
		} else if (version >= qMakePair(1,3)) {
			graphics_layer = new GraphicsLayer13;
		} else {
			graphics_layer = new GraphicsLayer11;
#endif
		}
	} else {
		const auto version = std::min(requested.version(), context.version());

		if (version >= qMakePair(3,0)) {
			const QString shader = "gl3";
			const QByteArray glsl = "300 es";

			auto vertex_array = new QOpenGLVertexArrayObject;
			vertex_array->create();
			vertex_array->bind();

			graphics_layer = new GraphicsLayer21(vertex_array, glsl, shader);
		} else {
			const QString shader = "gl2";

			graphics_layer = new GraphicsLayer21(nullptr, QByteArray(), shader);
		}
	}

	graphics_layer->setTextureUnits(1);
}

//-----------------------------------------------------------------------------

void GraphicsLayer::setVersion(int version)
{
	QSurfaceFormat f;
	if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
		switch (version) {
#ifndef QT_OPENGL_ES_2
		case 11:
		case 12:
			f.setVersion(1,1);
			break;
		case 13:
		case 14:
			f.setVersion(1,3);
			break;
		case 15:
		case 20:
			f.setVersion(1,5);
			break;
#endif
		case 21:
			f.setVersion(2,1);
			break;
		case 30:
		case 31:
		case 32:
		case 33:
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		default:
			f.setVersion(4,6);
			f.setProfile(QSurfaceFormat::CoreProfile);
			break;
		}
	} else {
		switch (version) {
		case 30:
		case 31:
		case 32:
			f.setVersion(3,0);
			break;
		case 20:
		default:
			f.setVersion(2,0);
			break;
		}
	}
	QSurfaceFormat::setDefaultFormat(f);
}

//-----------------------------------------------------------------------------

GraphicsLayer::GraphicsLayer() :
	m_changed(true)
{
	// Start with a 1MB vertex buffer
	m_data.resize(0x100000 / sizeof(Vertex));
	VertexArray free_region;
	free_region.end = m_data.count();
	m_free_regions.append(free_region);
}

//-----------------------------------------------------------------------------

GraphicsLayer::~GraphicsLayer()
{
}

//-----------------------------------------------------------------------------

void GraphicsLayer::updateArray(VertexArray& array, const QVector<Vertex>& data)
{
	int length = data.count();
	if (array.length() != length) {
		removeArray(array);
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
		std::copy(data.begin(), data.end(), m_data.begin() + array.start);
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
	if (array.end == 0) {
		return;
	}

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

void GraphicsLayer::uploadChanged(QOpenGLBuffer* vertex_buffer)
{
	const int vertex_size = sizeof(Vertex);
	if (!m_changed_regions.isEmpty()) {
		for (const VertexArray& region : m_changed_regions) {
			vertex_buffer->write(region.start * vertex_size, m_data.constBegin() + region.start, region.length() * vertex_size);
		}
		m_changed_regions.clear();
	} else if (m_changed) {
		const int size = m_data.count() * vertex_size;
		vertex_buffer->allocate(size);
		vertex_buffer->write(0, m_data.constData(), size);
		m_changed = false;
	}
}

//-----------------------------------------------------------------------------

GraphicsLayer21::GraphicsLayer21(QOpenGLVertexArrayObject* vertex_array, const QByteArray& glsl, const QString& shader) :
	m_program(nullptr),
	m_vertex_array(vertex_array)
{
	initializeOpenGLFunctions();

	AppearanceDialog::setBevelsEnabled(true);

	glDisable(GL_BLEND);

	// Enable OpenGL features
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// Set OpenGL parameters
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glFrontFace(GL_CCW);

	// Create vertex buffer object
	m_vertex_buffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	m_vertex_buffer->setUsagePattern(QOpenGLBuffer::DynamicDraw);
	m_vertex_buffer->create();
	m_vertex_buffer->bind();

	// Load shaders
	QOpenGLShaderProgram* program = loadProgram(0, glsl, shader);
	program->setAttributeBuffer(Position, GL_FLOAT, offsetof(Vertex, x), 3, sizeof(Vertex));
	program->enableAttributeArray(Position);

	program = loadProgram(1, glsl, shader);
	program->setAttributeBuffer(TexCoord0, GL_FLOAT, offsetof(Vertex, s), 2, sizeof(Vertex));
	program->setAttributeBuffer(Position, GL_FLOAT, offsetof(Vertex, x), 3, sizeof(Vertex));
	program->enableAttributeArray(Position);
	program->setUniformValue("texture0", GLuint(0));

	program = loadProgram(2, glsl, shader);
	program->setAttributeBuffer(TexCoord1, GL_FLOAT, offsetof(Vertex, s2), 2, sizeof(Vertex));
	program->setAttributeBuffer(TexCoord0, GL_FLOAT, offsetof(Vertex, s), 2, sizeof(Vertex));
	program->setAttributeBuffer(Position, GL_FLOAT, offsetof(Vertex, x), 3, sizeof(Vertex));
	program->enableAttributeArray(Position);
	program->setUniformValue("texture0", GLuint(0));
	program->setUniformValue("texture1", GLuint(1));
}

//-----------------------------------------------------------------------------

GraphicsLayer21::~GraphicsLayer21()
{
	// Unload shaders
	for (int i = 0; i < 3; ++i) {
		delete m_programs[i];
	}

	// Delete vertex buffer object
	delete m_vertex_buffer;

	// Delete vertex array object
	delete m_vertex_array;
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::bindTexture(unsigned int unit, GLuint texture)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE0);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::draw(const VertexArray& array, GLenum mode)
{
	glDrawArrays(mode, array.start, array.length());
}

//-----------------------------------------------------------------------------

GLint GraphicsLayer21::getMaxTextureSize()
{
	GLint max_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	return max_size;
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

void GraphicsLayer21::setClearColor(const QColor& color)
{
	glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setColor(const QColor& color)
{
	m_program->setUniformValue(m_color_location, color);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setModelview(const QMatrix4x4& matrix)
{
	m_modelview = matrix;
	convertMatrix((m_projection * m_modelview).constData(), m_matrix[0]);
	m_program->setUniformValue(m_matrix_location, m_matrix);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setProjection(const QMatrix4x4& matrix)
{
	m_projection = matrix;
	convertMatrix((m_projection * m_modelview).constData(), m_matrix[0]);
	m_program->setUniformValue(m_matrix_location, m_matrix);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::setTextureUnits(unsigned int units)
{
	Q_ASSERT(units < 3);
	QOpenGLShaderProgram* program = m_programs[units];
	if (m_program == program) {
		return;
	}

	m_program = program;
	m_program->bind();

	m_color_location = m_program->uniformLocation("color");
	m_matrix_location = m_program->uniformLocation("matrix");
	m_program->setUniformValue(m_color_location, Qt::white);
	m_program->setUniformValue(m_matrix_location, m_matrix);

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

void GraphicsLayer21::setViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glViewport(x, y, width, height);
}

//-----------------------------------------------------------------------------

void GraphicsLayer21::uploadData()
{
	uploadChanged(m_vertex_buffer);
}

//-----------------------------------------------------------------------------

QOpenGLShaderProgram* GraphicsLayer21::loadProgram(unsigned int index, const QByteArray& glsl, const QString& shader)
{
	// Load vertex shader code
	QByteArray vertex;
	QFile file(QString(":/shaders/%1/textures%2.vert").arg(shader).arg(index));
	if (file.open(QFile::ReadOnly)) {
		vertex = file.readAll();
		file.close();
	}

	// Load fragment shader code
	QByteArray frag;
	file.setFileName(QString(":/shaders/%1/textures%2.frag").arg(shader).arg(index));
	if (file.open(QFile::ReadOnly)) {
		frag = file.readAll();
		file.close();
	}

	// Add GLSL version
	if (!glsl.isEmpty()) {
		vertex.prepend("#version " + glsl + "\n");
		frag.prepend("#version " + glsl + "\n");
	}

	// Create program
	m_programs[index] = new QOpenGLShaderProgram;
	m_programs[index]->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex);
	m_programs[index]->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);

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

#ifndef QT_OPENGL_ES_2

GraphicsLayer11::GraphicsLayer11()
{
	initializeOpenGLFunctions();

	AppearanceDialog::setBevelsEnabled(false);

	// Disable unused OpenGL features
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);

	// Enable OpenGL features
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);

	// Set OpenGL parameters
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glFrontFace(GL_CCW);
	setColor(Qt::white);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::bindTexture(unsigned int unit, GLuint texture)
{
	Q_UNUSED(unit);
	glBindTexture(GL_TEXTURE_2D, texture);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::draw(const VertexArray& array, GLenum mode)
{
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(array.start).s);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &at(array.start).x);
	glDrawArrays(mode, 0, array.length());
}

//-----------------------------------------------------------------------------

GLint GraphicsLayer11::getMaxTextureSize()
{
	GLint max_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	return max_size;
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

void GraphicsLayer11::setClearColor(const QColor& color)
{
	glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setColor(const QColor& color)
{
	glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setModelview(const QMatrix4x4& matrix)
{
	GLfloat modelview[16];
	convertMatrix(matrix.constData(), modelview);
	glLoadMatrixf(modelview);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setProjection(const QMatrix4x4& matrix)
{
	GLfloat projection[16];
	convertMatrix(matrix.constData(), projection);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection);
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

void GraphicsLayer11::setViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glViewport(x, y, width, height);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::uploadData()
{
	clearChanged();
}

//-----------------------------------------------------------------------------

GraphicsLayer13::GraphicsLayer13()
{
	initializeOpenGLFunctions();

	AppearanceDialog::setBevelsEnabled(true);

	// Disable unused OpenGL features
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);

	// Enable OpenGL features
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);

	// Set OpenGL parameters
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LEQUAL);
	glFrontFace(GL_CCW);
	setColor(Qt::white);

	glActiveTexture(GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED);
	glActiveTexture(GL_TEXTURE0);
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::bindTexture(unsigned int unit, GLuint texture)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE0);
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::draw(const VertexArray& array, GLenum mode)
{
	glClientActiveTexture(GL_TEXTURE1);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(array.start).s2);
	glClientActiveTexture(GL_TEXTURE0);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(array.start).s);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &at(array.start).x);
	glDrawArrays(mode, 0, array.length());
}

//-----------------------------------------------------------------------------

GLint GraphicsLayer13::getMaxTextureSize()
{
	GLint max_size;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	return max_size;
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::setBlended(bool enabled)
{
	if (enabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::setClearColor(const QColor& color)
{
	glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::setColor(const QColor& color)
{
	glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::setModelview(const QMatrix4x4& matrix)
{
	GLfloat modelview[16];
	convertMatrix(matrix.constData(), modelview);
	glLoadMatrixf(modelview);
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::setProjection(const QMatrix4x4& matrix)
{
	GLfloat projection[16];
	convertMatrix(matrix.constData(), projection);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection);
	glMatrixMode(GL_MODELVIEW);
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::setTextureUnits(unsigned int units)
{
	glActiveTexture(GL_TEXTURE1);
	glClientActiveTexture(GL_TEXTURE1);
	if (units > 1) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	if (units > 0) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::setViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	glViewport(x, y, width, height);
}

//-----------------------------------------------------------------------------

void GraphicsLayer13::uploadData()
{
	clearChanged();
}

//-----------------------------------------------------------------------------

GraphicsLayer15::GraphicsLayer15()
{
	m_vertex_buffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	m_vertex_buffer->setUsagePattern(QOpenGLBuffer::DynamicDraw);
	m_vertex_buffer->create();
	m_vertex_buffer->bind();
}

//-----------------------------------------------------------------------------

GraphicsLayer15::~GraphicsLayer15()
{
	// Delete vertex buffer object
	delete m_vertex_buffer;
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::draw(const VertexArray& array, GLenum mode)
{
	glDrawArrays(mode, array.start, array.length());
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::setTextureUnits(unsigned int units)
{
	glActiveTexture(GL_TEXTURE1);
	glClientActiveTexture(GL_TEXTURE1);
	if (units > 1) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, s2)));
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	if (units > 0) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, s)));
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, x)));
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::uploadData()
{
	uploadChanged(m_vertex_buffer);
}

#endif

//-----------------------------------------------------------------------------
