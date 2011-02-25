/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include "vertex_array.h"
class Board;
class Tile;

#include <QColor>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QXmlStreamWriter>

class Piece
{
public:
	Piece(const QPoint& pos, int rotation, const QList<Tile*>& tiles, Board* board);
	~Piece();

	bool collidesWith(const Piece* other) const;
	bool contains(const QPoint& pos) const;
	QRect boundingRect() const;
	bool isSelected() const;
	int rotation() const;
	QPoint scenePos() const;

	void attachNeighbors();
	void moveBy(const QPoint& delta);
	void pushNeighbors(const QPointF& inertia = QPointF());
	void rotate(const QPoint& origin = QPoint());
	void setSelected(bool selected);

	void draw() const;
	void save(QXmlStreamWriter& xml) const;

private:
	void attach(Piece* piece);
	bool containsTile(int column, int row);
	void updateCollisionRegions();
	void updateShadow();
	void updateTiles();
	void updateVerts();

private:
	Board* m_board;
	QPoint m_pos;
	QRect m_rect;
	QList<Tile*> m_tiles;
	QList<Tile*> m_shadow;
	int m_rotation;
	bool m_selected;

	QColor m_shadow_color;
	VertexArray m_verts;
	VertexArray m_shadow_verts;

	bool m_changed;
	QRegion m_collision_region;
	QRegion m_collision_region_expanded;
};


inline QRect Piece::boundingRect() const
{
	return m_rect.translated(m_pos);
}

inline bool Piece::contains(const QPoint& pos) const
{
	return m_collision_region.contains(pos);
}

inline bool Piece::isSelected() const
{
	return m_selected;
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
	updateVerts();
}

#endif
