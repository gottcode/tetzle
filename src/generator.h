/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011, 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef GENERATOR_H
#define GENERATOR_H

#include <QList>
#include <QPoint>
#include <QVector>
namespace DLX
{
	struct Node;
}
class Tile;

class Generator
{
public:
	Generator(int columns, int rows);

	QList< QList<Tile*> > pieces() const;

private:
	void solve();
	void solution(const QVector<DLX::Node*>& rows);

private:
	int m_columns;
	int m_rows;
	QList< QList<Tile*> > m_pieces;
};

inline QList< QList<Tile*> > Generator::pieces() const
{
	return m_pieces;
}

#endif
