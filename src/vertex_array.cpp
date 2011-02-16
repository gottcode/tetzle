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

QVector<VertexArray::Vertex> VertexArray::m_shared_data;
QList<VertexArray::Region> VertexArray::m_free_regions;

//-----------------------------------------------------------------------------

VertexArray::VertexArray()
{
}

//-----------------------------------------------------------------------------

VertexArray::~VertexArray()
{
	freeRegion();
}

//-----------------------------------------------------------------------------

void VertexArray::beginUpdate()
{
	m_indices.clear();
}

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

void VertexArray::endUpdate()
{
	// Find free region
	int length = m_vertices.count();
	if (m_region.length() != length) {
		freeRegion();

		m_region.start = -1;
		for (int i = 0; i < m_free_regions.count(); ++i) {
			Region& free_region = m_free_regions[i];
			if (free_region.length() == length) {
				m_region.start = free_region.start;
				m_free_regions.removeAt(i);
				break;
			} else if (free_region.length() > length) {
				m_region.start = free_region.start;
				free_region.start += length;
				break;
			}
		}
	}

	// Move data into shared array
	if (m_region.start != -1) {
		qCopy(m_vertices.begin(), m_vertices.end(), &m_shared_data[m_region.start]);
	} else {
		m_region.start = m_shared_data.count();
		m_shared_data += m_vertices;
	}
	m_vertices.clear();
	m_region.end = m_region.start + length;

	for (int i = 0; i < m_indices.count(); ++i) {
		m_indices[i] += m_region.start;
	}
}

//-----------------------------------------------------------------------------

void VertexArray::draw() const
{
	glDrawRangeElements(GL_QUADS, m_region.start, m_region.end, m_indices.count(), GL_UNSIGNED_SHORT, m_indices.constData());
}

//-----------------------------------------------------------------------------

void VertexArray::uploadData()
{
	glVertexPointer(2, GL_INT, sizeof(Vertex), &m_shared_data.at(0).x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &m_shared_data.at(0).s);
}

//-----------------------------------------------------------------------------

void VertexArray::freeRegion()
{
	bool merged = false;
	for (int i = 0; i < m_free_regions.count(); ++i) {
		if (m_free_regions[i].merge(m_region)) {
			merged = true;
			break;
		}
	}
	if (!merged) {
		m_free_regions.append(m_region);
	}
	m_region.start = m_region.end = -1;
}

//-----------------------------------------------------------------------------
