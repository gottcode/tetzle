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

#include "opengl.h"

//-----------------------------------------------------------------------------

VertexArray* vertex_array = 0;

//-----------------------------------------------------------------------------

VertexArray::VertexArray()
:	m_changed(false)
{
}

//-----------------------------------------------------------------------------

VertexArray::~VertexArray()
{
}

//-----------------------------------------------------------------------------

void VertexArray::insert(Region& region, const QVector<Vertex>& data)
{
	int length = data.count();
	if (region.length() != length) {
		if (region.end > 0) {
			release(region);
		}
		region.start = -1;
		for (int i = 0; i < m_free_regions.count(); ++i) {
			Region& free_region = m_free_regions[i];
			if (free_region.length() == length) {
				region.start = free_region.start;
				m_free_regions.removeAt(i);
				break;
			} else if (free_region.length() > length) {
				region.start = free_region.start;
				free_region.start += length;
				break;
			}
		}
	}

	if (region.start != -1) {
		qCopy(data.begin(), data.end(), &m_data[region.start]);
		if (!m_changed) {
			m_changed_regions.append(region);
		}
	} else {
		region.start = m_data.count();
		m_data += data;
		m_changed = true;
		m_changed_regions.clear();
	}
	region.end = region.start + length;
}

//-----------------------------------------------------------------------------

void VertexArray::release(Region& region)
{
	bool merged = false;
	int count = m_free_regions.count();
	for (int i = 0; i < count; ++i) {
		if (m_free_regions[i].merge(region)) {
			merged = true;
			break;
		}
	}
	if (!merged) {
		m_free_regions.append(region);
	}
	region.start = region.end = 0;
}

//-----------------------------------------------------------------------------

void VertexArray::clearChanged()
{
	m_changed_regions.clear();
	m_changed = false;
}

//-----------------------------------------------------------------------------

void VertexArray::uploadChanged()
{
	if (!m_changed_regions.isEmpty()) {
		foreach (const Region& region, m_changed_regions) {
			GL::bufferSubData(GL_ARRAY_BUFFER, region.start * sizeof(Vertex), region.length() * sizeof(Vertex), &m_data.at(region.start));
		}
		m_changed_regions.clear();
	} else if (m_changed) {
		GLsizeiptr size = m_data.count() * sizeof(Vertex);
		GL::bufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
		GL::bufferSubData(GL_ARRAY_BUFFER, 0, size, m_data.constData());
		m_changed = false;
	}
}

//-----------------------------------------------------------------------------

void VertexArray11::draw(const Region& region, GLenum mode)
{
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(region.start).s);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &at(region.start).x);
	glDrawArrays(mode, 0, region.length());
}

//-----------------------------------------------------------------------------

void VertexArray11::setMultiTextured(bool enabled)
{
	Q_UNUSED(enabled);
}

//-----------------------------------------------------------------------------

void VertexArray11::uploadData()
{
	clearChanged();
}

//-----------------------------------------------------------------------------

void VertexArray13::draw(const Region& region, GLenum mode)
{
	GL::clientActiveTexture(GL_TEXTURE1);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(region.start).s2);
	GL::clientActiveTexture(GL_TEXTURE0);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &at(region.start).s);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &at(region.start).x);
	glDrawArrays(mode, 0, region.length());
}

//-----------------------------------------------------------------------------

void VertexArray13::setMultiTextured(bool enabled)
{
	GL::clientActiveTexture(GL_TEXTURE1);
	if (enabled) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	GL::clientActiveTexture(GL_TEXTURE0);
}

//-----------------------------------------------------------------------------

VertexArray15::VertexArray15()
:	m_id(0)
{
	GL::genBuffers(1, &m_id);
	GL::bindBuffer(GL_ARRAY_BUFFER, m_id);
}

//-----------------------------------------------------------------------------

VertexArray15::~VertexArray15()
{
	GL::bindBuffer(GL_ARRAY_BUFFER, 0);
	GL::deleteBuffers(1, &m_id);
}

//-----------------------------------------------------------------------------

void VertexArray15::draw(const Region& region, GLenum mode)
{
	glDrawArrays(mode, region.start, region.length());
}

//-----------------------------------------------------------------------------

void VertexArray15::setMultiTextured(bool enabled)
{
	GL::clientActiveTexture(GL_TEXTURE1);
	if (enabled) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 5));
	} else {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	GL::clientActiveTexture(GL_TEXTURE0);

	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);
}

//-----------------------------------------------------------------------------

void VertexArray15::uploadData()
{
	uploadChanged();
}

//-----------------------------------------------------------------------------
