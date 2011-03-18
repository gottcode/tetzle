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

#include "opengl.h"

#include "vertex_array.h"

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

namespace GL
{
	PFNGLACTIVETEXTUREPROC activeTexture = 0;
	PFNGLCLIENTACTIVETEXTUREPROC clientActiveTexture = 0;

	PFNGLBINDBUFFERPROC bindBuffer = 0;
	PFNGLBUFFERDATAPROC bufferData = 0;
	PFNGLBUFFERSUBDATAPROC bufferSubData = 0;
	PFNGLDELETEBUFFERSPROC deleteBuffers = 0;
	PFNGLGENBUFFERSPROC genBuffers = 0;

	void init()
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

		// Create shared vertex array
		switch (state) {
		case 0x03:
			vertex_array = new VertexArray15;
			break;
		case 0x01:
			vertex_array = new VertexArray13;
			break;
		default:
			vertex_array = new VertexArray11;
			break;
		}
	}
}
