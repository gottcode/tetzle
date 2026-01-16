/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

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

/**
 * A piece on the board.
 */
class Piece
{
public:
	/**
	 * Construct a piece.
	 *
	 * @param pos location in scene
	 * @param rotation how many times it is rotated 90 degrees
	 * @param tiles the list of tiles that make up the piece
	 * @param board the game board
	 */
	Piece(const QPoint& pos, int rotation, const QList<Tile*>& tiles, Board* board);

	/**
	 * Clean up piece.
	 */
	~Piece();

	/**
	 * Check if piece collides with another piece.
	 *
	 * @param other the piece to check for collision
	 *
	 * @return @c true if it collides
	 */
	bool collidesWith(const Piece* other) const;

	/**
	 * Check if piece contains a position.
	 *
	 * @param pos the position to check
	 *
	 * @return @c true if it contains position
	 */
	bool contains(const QPoint& pos) const
	{
		return m_collision_region.contains(pos);
	}

	/**
	 * Fetch bounding rectangle of the piece.
	 *
	 * @return bounding rectangle of the piece
	 */
	QRect boundingRect() const
	{
		return m_rect.translated(m_pos);
	}

	/**
	 * Fetch if piece is selected.
	 *
	 * @return @c true if piece is selected
	 */
	bool isSelected() const
	{
		return m_selected;
	}

	/**
	 * Fetch random position inside piece.
	 *
	 * @return random position inside piece
	 */
	QPoint randomPoint() const;

	/**
	 * Fetch how many times piece is rotated 90 degrees.
	 *
	 * @return how many times piece is rotated 90 degrees
	 */
	int rotation() const
	{
		return m_rotation;
	}

	/**
	 * Fetch location of piece in scene.
	 *
	 * @return location of piece in scene
	 */
	QPoint scenePos() const
	{
		return m_pos;
	}

	/**
	 * Fetch pixmap fragments for tile bevels.
	 *
	 * @return list of pixmap fragments for bevels
	 */
	FragmentList bevel() const
	{
		return m_bevel_list;
	}

	/**
	 * Fetch pixmap fragments for border shadow.
	 *
	 * @return list of pixmap fragments for shadow
	 */
	FragmentList shadow() const
	{
		return m_shadow_list;
	}

	/**
	 * Fetch pixmap fragments for tiles.
	 *
	 * @return list of pixmap fragments for tiles
	 */
	FragmentList tiles() const
	{
		return m_tiles_list;
	}

	/**
	 * Attempt to attach neighboring pieces in solution.
	 */
	void attachSolutionNeighbors();

	/**
	 * Find neighbors in solution.
	 *
	 * @param pieces list of pieces to check for neighbors
	 */
	void findSolutionNeighbors(const QList<Piece*>& pieces);

	/**
	 * Move piece.
	 *
	 * @param delta how far to move piece
	 */
	void moveBy(const QPoint& delta)
	{
		m_pos += delta;
		updateFragmentLists();
	}

	/**
	 * Push colliding pieces.
	 *
	 * @param inertia the inertia from a previous push
	 */
	void pushCollidingPieces(const QPointF& inertia = QPointF());

	/**
	 * Rotate piece.
	 *
	 * @param rotations how many times to rotate piece 90 degrees
	 */
	void rotate(int rotations);

	/**
	 * Rotate piece 90 degrees.
	 *
	 * @param origin center to rotate around
	 */
	void rotate(const QPoint& origin = QPoint());

	/**
	 * Set position of piece in scene.
	 *
	 * @param pos scene location of piece
	 */
	void setPosition(const QPoint& pos)
	{
		m_pos = pos;
		updateFragmentLists();
	}

	/**
	 * Select piece.
	 *
	 * @param selected @c true if piece is selected
	 */
	void setSelected(bool selected);

	/**
	 * Save piece details.
	 *
	 * @param xml where to save details
	 */
	void save(QXmlStreamWriter& xml) const;

private:
	/**
	 * Merge another piece into this piece. The other piece will be destroyed.
	 *
	 * @param piece the piece to attach
	 */
	void attach(Piece* piece);

	/**
	 * Check if piece contains a tile.
	 *
	 * @param column the column of the tile
	 * @param row the row of the tile
	 *
	 * @return @c true if piece contains tile
	 */
	bool containsTile(int column, int row) const;

	/**
	 * Expand a rectangle by the margin for attaching.
	 *
	 * @param rect the rectangle to expand
	 *
	 * @return rectangle expanded by attach margin
	 */
	QRect marginRect(const QRect& rect) const
	{
		return rect.adjusted(-m_attach_margin, -m_attach_margin, m_attach_margin, m_attach_margin);
	}

	/**
	 * Update collision regions to contain all tiles in piece.
	 */
	void updateCollisionRegions();

	/**
	 * Update pixmap fragment lists for drawing.
	 */
	void updateFragmentLists();

	/**
	 * Update shadow to be only under edge tiles of piece.
	 */
	void updateShadow();

	/**
	 * Update tiles to belong to piece.
	 */
	void updateTiles();

private:
	Board* m_board; ///< game board
	QPoint m_pos; ///< location in scene
	QRect m_rect; ///< bounding rectangle
	FragmentList m_tiles_list; ///< pixmap fragments of tiles
	FragmentList m_bevel_list; ///< pixmap fragments of bevels
	FragmentList m_shadow_list; ///< pixmap fragments of shadow
	QList<Tile*> m_tiles; ///< tiles of piece
	QList<Tile*> m_shadow; ///< tiles of piece with shadow
	QSet<Piece*> m_neighbors; ///< neighboring pieces in solution
	int m_rotation; ///< how many times is the piece rotated 90 degrees
	bool m_selected; ///< is the piece selected

	bool m_changed; ///< are collision regions out of date
	QRegion m_collision_region; ///< collision region
	QRegion m_collision_region_expanded; ///< collision region expanded by attach margin

	static constexpr int m_attach_margin = 16; ///< maximum distance between pieces before they attach
};

#endif // TETZLE_PIECE_H
