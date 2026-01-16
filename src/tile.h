/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_TILE_H
#define TETZLE_TILE_H

class Piece;

#include <QPoint>
#include <QXmlStreamWriter>

/**
 * A tile on the board.
 */
class Tile
{
public:
	/**
	 * Construct a tile.
	 *
	 * @param column the horizontal position of the tile
	 * @param row the vertical position of the tile
	 */
	Tile(int column, int row);

	/**
	 * Fetch the location of the bevel in the bevel image.
	 *
	 * @return the location of the bevel in the bevel image
	 */
	QPointF bevel() const
	{
		return m_bevel_coords;
	}

	/**
	 * Fetch the horizontal position of the tile.
	 *
	 * @return the horizontal position of the tile
	 */
	int column() const
	{
		return m_column;
	}

	/**
	 * Fetch the vertical position of the tile.
	 *
	 * @return the vertical position of the tile
	 */
	int row() const
	{
		return m_row;
	}

	/**
	 * Fetch the location of the tile in the scene.
	 *
	 * @return the location of the tile in the scene
	 */
	QPoint scenePos() const;

	/**
	 * Fetch the location of the tile in the solution.
	 *
	 * @return the location of the tile in the solution
	 */
	QPoint solutionPos() const
	{
		return QPoint(m_column * size, m_row * size);
	}

	/**
	 * Rotate the tile.
	 */
	void rotate();

	/**
	 * Set the bevel of the tile.
	 *
	 * @param bevel the bevel to use
	 */
	void setBevel(int bevel);

	/**
	 * Set the position of the tile relative to parent piece.
	 *
	 * @param pos the position of the tile
	 */
	void setPos(const QPoint& pos)
	{
		m_pos = pos;
	}

	/**
	 * Set the piece containing the tile.
	 *
	 * @param parent the piece containing the tile
	 */
	void setParent(const Piece* parent)
	{
		m_parent = parent;
	}

	/**
	 * Save the tile details.
	 *
	 * @param xml where to save the details
	 */
	void save(QXmlStreamWriter& xml) const;

	static constexpr int size = 64; ///< width and height of tile in pixels

private:
	const Piece* m_parent; ///< piece containing tile
	const int m_column; ///< horizontal position in solution
	const int m_row; ///< vertical position in solution
	QPoint m_pos; ///< location in scene relative to piece
	int m_bevel; ///< which bevel to use
	QPointF m_bevel_coords; ///< location in bevel image
};

#endif // TETZLE_TILE_H
