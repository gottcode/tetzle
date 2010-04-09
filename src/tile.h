/***********************************************************************
 *
 * Copyright (C) 2008, 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef TILE_H
#define TILE_H

#include <QPoint>
#include <QRect>
#include <QXmlStreamWriter>
class Piece;

class Tile
{
public:
	Tile(int column, int row);

	QRect boundingRect() const;
	int column() const;
	int row() const;
	Piece* parent() const;
	QPoint pos() const;
	QPoint scenePos() const;

	void setPos(const QPoint& pos);
	void setParent(Piece* parent);

	static QRect rect()
	{
		return QRect(0, 0, 32, 32);
	}

	static int size()
	{
		return 32;
	}

	void save(QXmlStreamWriter& xml) const;

private:
	Piece* m_parent;
	int m_column;
	int m_row;
	QPoint m_pos;
};


inline QRect Tile::boundingRect() const
{
	return rect().translated(scenePos());
}

inline int Tile::column() const
{
	return m_column;
}

inline int Tile::row() const
{
	return m_row;
}

inline Piece* Tile::parent() const
{
	return m_parent;
}

inline QPoint Tile::pos() const
{
	return m_pos;
}

inline void Tile::setPos(const QPoint& pos)
{
	m_pos = pos;
}

inline void Tile::setParent(Piece* parent)
{
	m_parent = parent;
}

#endif
