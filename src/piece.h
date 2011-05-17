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

#include "graphics_layer.h"
class Board;
class Tile;

#include <QColor>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QSet>
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
	QPoint randomPoint() const;
	int rotation() const;
	QPoint scenePos() const;

	void attachNeighbors();
	void findNeighbors(const QList<Piece*>& pieces);
	void moveBy(const QPoint& delta);
	void pushNeighbors(const QPointF& inertia = QPointF());
	void rotate(int rotations);
	void rotate(const QPoint& origin = QPoint());
	void setDepth(int depth);
	void setPosition(const QPoint& pos);
	void setSelected(bool selected);

	void drawTiles() const;
	void drawShadow() const;
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
	QSet<Piece*> m_neighbors;
	int m_rotation;
	int m_depth;
	bool m_selected;

	VertexArray m_tile_array;
	VertexArray m_shadow_array;

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

inline void Piece::setPosition(const QPoint& pos)
{
	m_pos = pos;
	updateVerts();
}

inline void Piece::drawTiles() const
{
	graphics_layer->draw(m_tile_array);
}

inline void Piece::drawShadow() const
{
	graphics_layer->draw(m_shadow_array);
}

#endif
