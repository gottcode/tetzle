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
	Piece(const QPoint& pos, int rotation, const QList<Tile*>& tiles, Board* board);
	~Piece();

	bool collidesWith(const Piece * other) const;
	QRect boundingRect() const;
	QList<Tile*> children() const;
	QRect marginRect() const;
	int rotation() const;
	QPoint scenePos() const;

	void attach(Piece* piece);
	void attachNeighbors();
	void moveBy(const QPoint& delta);
	void moveTo(const QPoint& pos);
	void moveTo(int x, int y);
	void pushNeighbors(const QPointF& inertia = QPointF());
	void rotateAround(Tile* tile);

	void draw() const;
	void save(QXmlStreamWriter& xml) const;

private:
	void updateShadow();
	bool containsTile(int column, int row);

private:
	Board* m_board;
	int m_rotation;
	QPoint m_pos;
	QRect m_rect;
	QList<Tile*> m_children;
	QList<Tile*> m_shadow;
};


inline QRect Piece::boundingRect() const
{
	return m_rect.translated(m_pos);
}

inline QList<Tile*> Piece::children() const
{
	return m_children;
}

inline int Piece::rotation() const
{
	return m_rotation;
}

inline QPoint Piece::scenePos() const
{
	return m_pos;
}

inline void Piece::moveBy(const QPoint& delta)
{
	m_pos += delta;
}

inline void Piece::moveTo(const QPoint& pos)
{
	m_pos = pos;
}

inline void Piece::moveTo(int x, int y)
{
	m_pos.setX(x);
	m_pos.setY(y);
}

#endif
