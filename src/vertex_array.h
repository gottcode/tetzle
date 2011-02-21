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

#ifndef VERTEX_ARRAY_H
#define VERTEX_ARRAY_H

#include <qgl.h>

class VertexArray
{
public:
	void append(GLint x, GLint y, GLfloat s, GLfloat t);
	void clear();
	void draw() const;

private:
	struct Vertex
	{
		GLint x;
		GLint y;
		GLfloat s;
		GLfloat t;

		Vertex(GLint x_ = 0, GLint y_ = 0, GLfloat s_ = 0, GLfloat t_ = 0)
			: x(x_), y(y_), s(s_), t(t_)
		{
		}

		bool operator==(const Vertex& other) const
		{
			return (x == other.x) && (y == other.y) && (s == other.s) && (t == other.t);
		}
	};

	QVector<Vertex> m_vertices;
	QVector<GLushort> m_indices;
};

#endif
