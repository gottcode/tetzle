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
	Vertex vertex(x, y, s, t);
	int index = m_vertices.indexOf(vertex);
	if (index == -1) {
		index = m_vertices.count();
		m_vertices += vertex;
	}
	m_indices.append(index);
}

//-----------------------------------------------------------------------------

void VertexArray::clear()
{
	m_vertices.clear();
	m_indices.clear();
}

//-----------------------------------------------------------------------------

void VertexArray::draw() const
{
	glVertexPointer(2, GL_INT, sizeof(Vertex), &m_vertices.at(0).x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &m_vertices.at(0).s);
	glDrawElements(GL_QUADS, m_indices.count(), GL_UNSIGNED_SHORT, m_indices.constData());
}

//-----------------------------------------------------------------------------
