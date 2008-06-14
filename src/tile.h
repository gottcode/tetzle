/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QXmlStreamWriter>

class Board;

class Tile
{
public:
	Tile(int column, int row, int rotation, const QPoint& pos, Tile* parent, Board* board);
	~Tile();

	int column() const
		{ return m_column; }
	int row() const
		{ return m_row; }
	int rotation() const
		{ return m_rotation; }
	QList<Tile*> children() const
		{ return m_children; }
	Tile* parent() const
		{ return m_parent; }

	void moveBy(const QPoint& delta)
		{ m_pos += delta; }
	QPoint scenePos() const;
	QRect rect() const;
	QRect boundingRect() const
		{ return m_rect.translated(scenePos()); }

	void rotateAround(Tile* tile);
	void attach(Tile* tile);
	void attachNeighbors(int region);
	void pushNeighbors(Tile* immobile);

	void save(QXmlStreamWriter& xml) const;

private:
	int m_column;
	int m_row;
	int m_rotation;
	QPoint m_pos;
	QRect m_rect;

	Tile* m_parent;
	QList<Tile*> m_children;
	Board* m_board;
};

#endif // TILE_H
