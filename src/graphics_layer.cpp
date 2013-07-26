/***********************************************************************
 *
 * Copyright (C) 2011, 2012 Graeme Gott <graeme@gottcode.org>
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
#include <QGLShaderProgram>
#include <QSettings>

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
#ifndef GL_VERSION_1_3
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#endif

#ifndef GL_VERSION_1_3_DEPRECATED
#define GL_COMBINE     0x8570
#define GL_COMBINE_RGB 0x8571
#define GL_ADD_SIGNED  0x8574
#endif

typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
static PFNGLACTIVETEXTUREPROC activeTexture = 0;
typedef void (APIENTRYP PFNGLCLIENTACTIVETEXTUREPROC) (GLenum texture);
static PFNGLCLIENTACTIVETEXTUREPROC clientActiveTexture = 0;

// Vertex buffer object extension
#ifndef GL_VERSION_1_5
#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
#endif

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

// Shaders extension
#ifndef GL_VERSION_2_0
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif

static QString glsl_version;

// OpenGL 3 string
#ifndef GL_VERSION_3_0
#define GL_NUM_EXTENSIONS 0x821D
#endif

typedef const GLubyte* (APIENTRYP PFNGLGETSTRINGIPROC)(GLenum name, GLuint index);
static PFNGLGETSTRINGIPROC getStringi = 0;

// Vertex attribute object extension
static GLuint vao_id = 0;

typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
static PFNGLBINDVERTEXARRAYPROC bindVertexArray = 0;
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);
static PFNGLDELETEVERTEXARRAYSPROC deleteVertexArrays = 0;
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
static PFNGLGENVERTEXARRAYSPROC genVertexArrays = 0;

namespace
{
	enum StateFlags
	{
		MultiTextureFlag = 0x01,
		VertexBufferObjectFlag = 0x02,
		FragmentShadersFlag = 0x04,
		VertexArrayObjectFlag = 0x08
	};

	enum Versions
	{
		Version11 = 0,
		Version13 = MultiTextureFlag,
		Version15 = MultiTextureFlag | VertexBufferObjectFlag,
		Version21 = FragmentShadersFlag | MultiTextureFlag | VertexBufferObjectFlag,
		Version30 = VertexArrayObjectFlag | FragmentShadersFlag | MultiTextureFlag | VertexBufferObjectFlag
	};
}

#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
typedef void* QFunctionPointer;
#endif
static QFunctionPointer getProcAddress(const QString& name)
{
	QFunctionPointer result = 0;
	QString names[] = { name, name + "ARB", name + "EXT" };
	for (int i = 0; i < 3; ++i) {
		result = QGLContext::currentContext()->getProcAddress(names[i]);
		if (result) {
			break;
		}
	}
	return result;
}

static inline void convertMatrix(const float* in, GLfloat* out)
{
	std::copy(in, in + 16, out);
}

static inline void convertMatrix(const double* in, GLfloat* out)
{
	std::copy(in, in + 16, out);
}

//-----------------------------------------------------------------------------

void GraphicsLayer::init()
{
	unsigned int state = 0;
	unsigned int symbols = 0;

	// Try to load multi-texture functions
	activeTexture = (PFNGLACTIVETEXTUREPROC) getProcAddress("glActiveTexture");
	clientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC) getProcAddress("glClientActiveTexture");
	if (activeTexture && clientActiveTexture) {
		symbols |= MultiTextureFlag;
	}

	// Try to load vertex buffer object functions
	bindBuffer = (PFNGLBINDBUFFERPROC) getProcAddress("glBindBuffer");
	bufferData = (PFNGLBUFFERDATAPROC) getProcAddress("glBufferData");
	bufferSubData = (PFNGLBUFFERSUBDATAPROC) getProcAddress("glBufferSubData");
	deleteBuffers = (PFNGLDELETEBUFFERSPROC) getProcAddress("glDeleteBuffers");
	genBuffers = (PFNGLGENBUFFERSPROC) getProcAddress("glGenBuffers");
	if (bindBuffer && bufferData && bufferSubData && deleteBuffers && genBuffers) {
		symbols |= VertexBufferObjectFlag;
	}

	// Check for minimum supported programmable pipeline
	QByteArray glsl = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	int index = glsl.indexOf(' ');
	if (index != -1) {
		glsl.truncate(index);
	}
	glsl.replace('.', "");
	if (QGLShaderProgram::hasOpenGLShaderPrograms() && (glsl >= "120")) {
		state |= FragmentShadersFlag;
		symbols |= FragmentShadersFlag;
		if (activeTexture) {
			symbols |= MultiTextureFlag;
		}
	}

	// Try to load vertex array object functions
	bindVertexArray = (PFNGLBINDVERTEXARRAYPROC) getProcAddress("glBindVertexArray");
	deleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) getProcAddress("glDeleteVertexArrays");
	genVertexArrays = (PFNGLGENVERTEXARRAYSPROC) getProcAddress("glGenVertexArrays");
	if (bindVertexArray && deleteVertexArrays && genVertexArrays) {
		symbols |= VertexArrayObjectFlag;
	}

	// Try to determine OpenGL extensions
	QList<QByteArray> extensions;
	const char* ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	if (!ext) {
		getStringi = (PFNGLGETSTRINGIPROC) getProcAddress("glGetStringi");
		if (getStringi) {
			int count = 0;
			glGetIntegerv(GL_NUM_EXTENSIONS, &count);
			for (int i = 0; i < count; ++i) {
				extensions += reinterpret_cast<const char*>(getStringi(GL_EXTENSIONS, i));
			}
		}
	} else {
		extensions = QByteArray(ext).split(' ');
	}
	foreach (const QByteArray& extension, extensions) {
		if (extension == "GL_ARB_multitexture") {
			state |= MultiTextureFlag;
		} else if (extension == "GL_ARB_vertex_buffer_object") {
			state |= VertexBufferObjectFlag;
		} else if (extension == "GL_ARB_vertex_array_object") {
			state |= VertexArrayObjectFlag;
		}
	}

	// Find maximum supported state
	int detected = 11;
	switch (qMin(state, symbols)) {
	case Version30: detected = 30; break;
	case Version21: detected = 21; break;
	case Version15: detected = 15; break;
	case Version13: detected = 13; break;
	default: break;
	}

	// Check for a requested state
	unsigned int new_state = 0;
	int requested = QSettings().value("GraphicsLayer", detected).toInt();
	QStringList args = QCoreApplication::arguments();
	foreach (const QString& arg, args) {
		if (arg.startsWith("--graphics-layer=")) {
			requested = arg.mid(17).toInt();
		}
	}
	switch (requested) {
	case 30: new_state = Version30; break;
	case 21: new_state = Version21; break;
	case 15: new_state = Version15; break;
	case 13: new_state = Version13; break;
	case 11: new_state = Version11; break;
	default:
		new_state = state;
		qWarning("Requested GraphicsLayer%d is invalid; using detected GraphicsLayer%d instead.", requested, detected);
		break;
	}
	if (state >= new_state) {
		state = new_state;
	} else {
		qWarning("Unable to use requested GraphicsLayer%d; using detected GraphicsLayer%d instead.", requested, detected);
	}

	// Create graphics layer instance
	switch (state) {
	case Version30:
		glsl_version = (glsl <= "420") ? glsl : "420";
		genVertexArrays(1, &vao_id);
		bindVertexArray(vao_id);
		graphics_layer = new GraphicsLayer21;
		break;
	case Version21:
		glsl_version = "120";
		graphics_layer = new GraphicsLayer21;
		break;
	case Version15:
		graphics_layer = new GraphicsLayer15;
		break;
	case Version13:
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

	// Start with a 1MB vertex buffer
	m_data.resize(0x100000 / sizeof(Vertex));
	VertexArray free_region;
	free_region.end = m_data.count();
	m_free_regions.append(free_region);
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

void GraphicsLayer::uploadChanged()
{
	if (!m_changed_regions.isEmpty()) {
		foreach (const VertexArray& region, m_changed_regions) {
			bufferSubData(GL_ARRAY_BUFFER, region.start * sizeof(Vertex), region.length() * sizeof(Vertex), m_data.constBegin() + region.start);
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
	QGLShaderProgram* program = m_programs[units];
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
	if (glsl_version >= "130") {
		vertex.replace("#version 120\n", "#version " + glsl_version + "\n");
		vertex.replace("attribute ", "in ");
		vertex.replace("varying ", "out ");

		frag.replace("#version 120\n", "#version " + glsl_version + "\n\nout vec4 out_color;\n");
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
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(offsetof(Vertex, s2)));
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}

	activeTexture(GL_TEXTURE0);
	clientActiveTexture(GL_TEXTURE0);
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
	uploadChanged();
}

//-----------------------------------------------------------------------------
