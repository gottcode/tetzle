/*
	SPDX-FileCopyrightText: 2008-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_GENERATOR_H
#define TETZLE_GENERATOR_H

namespace DLX
{
	struct Node;
}
class Tile;

#include <QList>
#include <QRandomGenerator>

/**
 * Generator for a puzzle.
 */
class Generator
{
public:
	/**
	 * Construct a new puzzle.
	 *
	 * @param columns how many columns in puzzle
	 * @param rows how many rows in puzzle
	 * @param random the random number generator
	 */
	Generator(int columns, int rows, QRandomGenerator& random);

	/**
	 * Fetch the tiles which make up the new puzzle, grouped into shapes.
	 *
	 * @return the tiles which make up the new puzzle
	 */
	QList<QList<Tile*>> pieces() const
	{
		return m_pieces;
	}

private:
	/**
	 * Generate a puzzle.
	 */
	void solve();

private:
	const int m_columns; ///< how many columns in puzzle
	const int m_rows; ///< how many rows in puzzle
	QList<QList<Tile*>> m_pieces; ///< tiles that make up puzzle
	QRandomGenerator& m_random; ///< random number generator
};

#endif // TETZLE_GENERATOR_H
