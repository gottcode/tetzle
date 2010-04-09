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
class Board;
class Piece;

class Tile
{
public:
	Tile(int column, int row, const QPoint& pos, Board* board);

	QRect boundingRect() const;
	int column() const;
	int row() const;
	Piece* parent() const;
	QPoint pos() const;
	QRect rect() const;
	QPoint scenePos() const;

	void setPos(const QPoint& pos);
	void setParent(Piece* parent);

	void save(QXmlStreamWriter& xml, bool scene_pos = false) const;

private:
	int m_column;
	int m_row;
	QPoint m_pos;
	QRect m_rect;

	Piece* m_parent;
	Board* m_board;
};


inline QRect Tile::boundingRect() const
{
	return m_rect.translated(scenePos());
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

inline QRect Tile::rect() const
{
	return m_rect;
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
