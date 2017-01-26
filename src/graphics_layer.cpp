/***********************************************************************
 *
 * Copyright (C) 2011, 2012, 2014, 2016, 2017 Graeme Gott <graeme@gottcode.org>
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
#if (QT_VERSION < QT_VERSION_CHECK(5,4,0))
#include <QGLFormat>
#endif
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

//-----------------------------------------------------------------------------

GraphicsLayer* graphics_layer = 0;

//-----------------------------------------------------------------------------

static QString glsl_version;
static QString shader_version;

template <typename T>
static inline void convertMatrix(const T* in, GLfloat* out)
{
	std::copy(in, in + 16, out);
}

//-----------------------------------------------------------------------------

void GraphicsLayer::init()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5,4,0))
	const auto requested = QSurfaceFormat::defaultFormat();
#else
	const auto requested = QGLFormat::toSurfaceFormat(QGLFormat::defaultFormat());
#endif
	const auto context = QOpenGLContext::currentContext()->format();
	const auto version = std::min((context.profile() == QSurfaceFormat::CoreProfile) ? qMakePair(4,5) : requested.version(), context.version());

	if (version >= qMakePair(3,0)) {
		if (version >= qMakePair(3,3)) {
			glsl_version = QByteArray::number((version.first * 100) + (version.second * 10));
			shader_version = "330";
		} else if (version == qMakePair(3,2)) {
			glsl_version = "150";
			shader_version = "130";
		} else if (version == qMakePair(3,1)) {
			glsl_version = "140";
			shader_version = "130";
		} else {
			glsl_version = shader_version = "130";
		}
		auto vertex_array = new QOpenGLVertexArrayObject;
		vertex_array->create();
		vertex_array->bind();
		graphics_layer = new GraphicsLayer21(vertex_array);
	} else if (version >= qMakePair(2,1)) {
		glsl_version = shader_version = "120";
		graphics_layer = new GraphicsLayer21;
	} else if (version >= qMakePair(1,5)) {
		graphics_layer = new GraphicsLayer15;
	} else if (version >= qMakePair(1,3)) {
		graphics_layer = new GraphicsLayer13;
	} else {
		graphics_layer = new GraphicsLayer11;
	}

	graphics_layer->setTextureUnits(1);
}

//-----------------------------------------------------------------------------

void GraphicsLayer::setVersion(int version)
{
	QSurfaceFormat f;
	switch (version) {
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
	default:
		f.setVersion(4,5);
		f.setProfile(QSurfaceFormat::CoreProfile);
		break;
	}
#if (QT_VERSION >= QT_VERSION_CHECK(5,4,0))
	QSurfaceFormat::setDefaultFormat(f);
#else
	QGLFormat::setDefaultFormat(QGLFormat::fromSurfaceFormat(f));
#endif
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
	if (!m_changed_regions.isEmpty()) {
		for (const VertexArray& region : m_changed_regions) {
			vertex_buffer->write(region.start * sizeof(Vertex), m_data.constBegin() + region.start, region.length() * sizeof(Vertex));
		}
		m_changed_regions.clear();
	} else if (m_changed) {
		GLsizeiptr size = m_data.count() * sizeof(Vertex);
		vertex_buffer->allocate(size);
		vertex_buffer->write(0, m_data.constData(), size);
		m_changed = false;
	}
}

//-----------------------------------------------------------------------------

GraphicsLayer21::GraphicsLayer21(QOpenGLVertexArrayObject* vertex_array) :
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
	QOpenGLShaderProgram* program = loadProgram(0);
	program->setAttributeBuffer(Position, GL_FLOAT, offsetof(Vertex, x), 3, sizeof(Vertex));
	program->enableAttributeArray(Position);

	program = loadProgram(1);
	program->setAttributeBuffer(TexCoord0, GL_FLOAT, offsetof(Vertex, s), 2, sizeof(Vertex));
	program->setAttributeBuffer(Position, GL_FLOAT, offsetof(Vertex, x), 3, sizeof(Vertex));
	program->enableAttributeArray(Position);
	program->setUniformValue("texture0", GLuint(0));

	program = loadProgram(2);
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

QOpenGLShaderProgram* GraphicsLayer21::loadProgram(unsigned int index)
{
	// Load vertex shader code
	QString vertex;
	QFile file(QString(":/shaders/%1/textures%2.vert").arg(shader_version).arg(index));
	if (file.open(QFile::ReadOnly)) {
		vertex = file.readAll();
		file.close();
	}

	// Load fragment shader code
	QString frag;
	file.setFileName(QString(":/shaders/%1/textures%2.frag").arg(shader_version).arg(index));
	if (file.open(QFile::ReadOnly)) {
		frag = file.readAll();
		file.close();
	}

	// Update GLSL version
	if (glsl_version > shader_version) {
		vertex.replace("#version " + shader_version + "\n", "#version " + glsl_version + "\n");
		frag.replace("#version " + shader_version + "\n", "#version " + glsl_version + "\n");
	}

	// Create program
	m_programs[index] = new QOpenGLShaderProgram;
	m_programs[index]->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex);
	m_programs[index]->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);

	// Set attribute locations
	if (shader_version < "330") {
		m_programs[index]->bindAttributeLocation("position", Position);
		if (index > 0) {
			m_programs[index]->bindAttributeLocation("texcoord0", TexCoord0);
		}
		if (index > 1) {
			m_programs[index]->bindAttributeLocation("texcoord1", TexCoord1);
		}
	}

	// Link and bind program
	m_programs[index]->link();
	m_programs[index]->bind();
	return m_programs[index];
}

//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
