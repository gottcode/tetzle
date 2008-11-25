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

#ifndef PIECE_H
#define PIECE_H

#include <QList>
#include <QPoint>
#include <QRect>
#include <QXmlStreamWriter>

class Board;
class Tile;

class Piece
{
public:
	Piece(int rotation, const QPoint& pos, Board* board);
	~Piece();

	int rotation() const
		{ return m_rotation; }
	QList<Tile*> children() const
		{ return m_children; }

	void moveBy(const QPoint& delta)
		{ m_pos += delta; }
	void moveTo(const QPoint& pos)
		{ m_pos = pos; }
	void moveTo(int x, int y)
		{ m_pos.setX(x); m_pos.setY(y); }
	const QPoint& position() const
		{ return m_pos; }
	QPoint scenePos() const;
	QRect boundingRect() const
		{ return m_rect.translated(m_pos); }
	QRect marginRect() const;
	bool collidesWith(const Piece * other) const;

	void rotateAround(Tile* tile);
	void attach(Tile* tile);
	void attach(Piece* piece);
	void attachNeighbors();
	void pushNeighbors()
		{ pushNeighbors(QPointF()); }

	void save(QXmlStreamWriter& xml) const;

private:
	void pushNeighbors(const QPointF& inertia);

	int m_rotation;
	QPoint m_pos;
	QRect m_rect;

	QList<Tile*> m_children;
	Board* m_board;
};

#endif // PIECE_H
