/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_PIECE_H
#define TETZLE_PIECE_H

#include "fragment_list.h"
class Board;
class Tile;

#include <QList>
#include <QPoint>
#include <QRect>
#include <QRegion>
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

	FragmentList bevel() const;
	FragmentList shadow() const;
	FragmentList tiles() const;

	void attachNeighbors();
	void findNeighbors(const QList<Piece*>& pieces);
	void moveBy(const QPoint& delta);
	void pushCollidingPieces(const QPointF& inertia = QPointF());
	void rotate(int rotations);
	void rotate(const QPoint& origin = QPoint());
	void setPosition(const QPoint& pos);
	void setSelected(bool selected);

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
	FragmentList m_tiles_list;
	FragmentList m_bevel_list;
	FragmentList m_shadow_list;
	QList<Tile*> m_tiles;
	QList<Tile*> m_shadow;
	QSet<Piece*> m_neighbors;
	int m_rotation;
	bool m_selected;

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

inline FragmentList Piece::bevel() const
{
	return m_bevel_list;
}

inline FragmentList Piece::shadow() const
{
	return m_shadow_list;
}

inline FragmentList Piece::tiles() const
{
	return m_tiles_list;
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

#endif // TETZLE_PIECE_H
