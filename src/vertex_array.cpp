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

#include "vertex_array.h"

//-----------------------------------------------------------------------------

void VertexArray::append(GLint x, GLint y, GLfloat s, GLfloat t)
{
	int index = -1;
	int count = m_verts.count();
	for (int i = 0; i < count; ++i) {
		if ((m_verts.at(i) == x) &&
			(m_tex_coords.at(i) == s) &&
			(m_verts.at(++i) == y) &&
			(m_tex_coords.at(i) == t)) {
			index = i >> 1;
			break;
		}
	}
	if (index == -1) {
		index = count >> 1;
		m_verts.append(x);
		m_verts.append(y);
		m_tex_coords.append(s);
		m_tex_coords.append(t);
	}
	m_indices.append(index);
}

//-----------------------------------------------------------------------------

void VertexArray::clear()
{
	m_verts.clear();
	m_tex_coords.clear();
	m_indices.clear();
}

//-----------------------------------------------------------------------------

void VertexArray::draw() const
{
	glVertexPointer(2, GL_INT, 0, m_verts.constData());
	glTexCoordPointer(2, GL_FLOAT, 0, m_tex_coords.constData());
	glDrawElements(GL_QUADS, m_indices.count(), GL_UNSIGNED_SHORT, m_indices.constData());
}

//-----------------------------------------------------------------------------
