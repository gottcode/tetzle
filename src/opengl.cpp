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

static void* getProcAddress(const QString& name)
{
	void* result;
	QString names[] = { name, name + "ARB", name + "EXT" };
	for (int i = 0; i < 4; ++i) {
		result = QGLContext::currentContext()->getProcAddress(names[i]);
		if (result) {
			break;
		}
	}
	return result;
}

static void fallbackClientActiveTexture(GLenum)
{
}

namespace GL
{
	PFNGLACTIVETEXTUREPROC activeTexture = 0;
	PFNGLCLIENTACTIVETEXTUREPROC clientActiveTexture = &fallbackClientActiveTexture;

	void init()
	{
		QStringList extensions = QString(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS))).split(' ');

		// Load multi-texture extension
		if (extensions.contains("GL_ARB_multitexture")) {
			activeTexture = (PFNGLACTIVETEXTUREPROC) getProcAddress("glActiveTexture");
			clientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC) getProcAddress("glClientActiveTexture");
		}
	}
}
