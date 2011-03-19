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

#include <QMatrix4x4>

//-----------------------------------------------------------------------------

GraphicsLayer* graphics_layer = 0;

//-----------------------------------------------------------------------------

// Multi-texture extension
static PFNGLACTIVETEXTUREPROC activeTexture = 0;
static PFNGLCLIENTACTIVETEXTUREPROC clientActiveTexture = 0;

// Vertex buffer object extension
static PFNGLBINDBUFFERPROC bindBuffer = 0;
static PFNGLBUFFERDATAPROC bufferData = 0;
static PFNGLBUFFERSUBDATAPROC bufferSubData = 0;
static PFNGLDELETEBUFFERSPROC deleteBuffers = 0;
static PFNGLGENBUFFERSPROC genBuffers = 0;

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

	// Create graphics layer instance
	switch (state) {
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
}

//-----------------------------------------------------------------------------

GraphicsLayer::GraphicsLayer()
:	m_changed(false)
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
				m_free_regions.removeAt(i);
				break;
			} else if (free_region.length() > length) {
				array.start = free_region.start;
				free_region.start += length;
				break;
			}
		}
	}

	if (array.start != -1) {
		qCopy(data.begin(), data.end(), &m_data[array.start]);
		if (!m_changed) {
			m_changed_regions.append(array);
		}
	} else {
		array.start = m_data.count();
		m_data += data;
		m_changed = true;
		m_changed_regions.clear();
	}
	array.end = array.start + length;
}

//-----------------------------------------------------------------------------

void GraphicsLayer::removeArray(VertexArray& array)
{
	bool merged = false;
	int count = m_free_regions.count();
	for (int i = 0; i < count; ++i) {
		if (m_free_regions[i].merge(array)) {
			merged = true;
			break;
		}
	}
	if (!merged) {
		m_free_regions.append(array);
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

GraphicsLayer11::GraphicsLayer11()
{
	AppearanceDialog::setBevelsEnabled(false);

	// Disable unused OpenGL features
	glDisable(GL_LIGHTING);

	// Enable OpenGL features
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	// Set OpenGL parameters
	setColor(Qt::white);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::bindTexture(GLenum unit, GLuint texture)
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

void GraphicsLayer11::setTextured(bool enabled)
{
	if (enabled) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setMultiTextured(bool enabled)
{
	Q_UNUSED(enabled);
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setModelview(const QMatrix4x4& matrix)
{
	glLoadIdentity();
	glMultMatrixd(matrix.constData());
}

//-----------------------------------------------------------------------------

void GraphicsLayer11::setProjection(const QMatrix4x4& matrix)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixd(matrix.constData());
	glMatrixMode(GL_MODELVIEW);
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

void GraphicsLayer13::bindTexture(GLenum unit, GLuint texture)
{
	activeTexture(unit);
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

void GraphicsLayer13::setMultiTextured(bool enabled)
{
	activeTexture(GL_TEXTURE1);
	clientActiveTexture(GL_TEXTURE1);
	if (enabled) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}
	clientActiveTexture(GL_TEXTURE0);
	activeTexture(GL_TEXTURE0);
}

//-----------------------------------------------------------------------------

GraphicsLayer15::GraphicsLayer15()
:	m_id(0)
{
	genBuffers(1, &m_id);
	bindBuffer(GL_ARRAY_BUFFER, m_id);
}

//-----------------------------------------------------------------------------

GraphicsLayer15::~GraphicsLayer15()
{
	bindBuffer(GL_ARRAY_BUFFER, 0);
	deleteBuffers(1, &m_id);
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::draw(const VertexArray& array, GLenum mode)
{
	glDrawArrays(mode, array.start, array.length());
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::setTextured(bool enabled)
{
	if (enabled) {
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

void GraphicsLayer15::setMultiTextured(bool enabled)
{
	activeTexture(GL_TEXTURE1);
	clientActiveTexture(GL_TEXTURE1);
	if (enabled) {
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 5));
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
	}
	clientActiveTexture(GL_TEXTURE0);
	activeTexture(GL_TEXTURE0);

	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
}

//-----------------------------------------------------------------------------

void GraphicsLayer15::uploadData()
{
	uploadChanged();
}

//-----------------------------------------------------------------------------
