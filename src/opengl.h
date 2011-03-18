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

#ifndef OPENGL_H
#define OPENGL_H

#include <qgl.h>
#include <GL/glext.h>

namespace GL
{
	void init();

	// Multi-texture extension
	extern PFNGLACTIVETEXTUREPROC activeTexture;
	extern PFNGLCLIENTACTIVETEXTUREPROC clientActiveTexture;

	// Vertex buffer object extension
	extern PFNGLBINDBUFFERPROC bindBuffer;
	extern PFNGLBUFFERDATAPROC bufferData;
	extern PFNGLBUFFERSUBDATAPROC bufferSubData;
	extern PFNGLDELETEBUFFERSPROC deleteBuffers;
	extern PFNGLGENBUFFERSPROC genBuffers;
}

#endif
