/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_TILE_H
#define TETZLE_TILE_H

class Piece;

#include <QPoint>
#include <QRect>
#include <QXmlStreamWriter>

class Tile
{
public:
	Tile(int column, int row);

	QPointF bevel() const;
	int column() const;
	int row() const;
	Piece* parent() const;
	QPoint pos() const;
	QPoint gridPos() const;
	QPoint scenePos() const;

	void rotate();
	void setBevel(int bevel);
	void setPos(const QPoint& pos);
	void setParent(Piece* parent);

	static const int size = 64;

	void save(QXmlStreamWriter& xml) const;

private:
	Piece* m_parent;
	int m_column;
	int m_row;
	QPoint m_pos;
	int m_bevel;
	QPointF m_bevel_coords;
};


inline QPointF Tile::bevel() const
{
	return m_bevel_coords;
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

inline QPoint Tile::gridPos() const
{
	return QPoint(m_column * size, m_row * size);
}

inline void Tile::setPos(const QPoint& pos)
{
	m_pos = pos;
}

inline void Tile::setParent(Piece* parent)
{
	m_parent = parent;
}

#endif // TETZLE_TILE_H
