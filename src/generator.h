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
#include <QPoint>
#include <QRandomGenerator>

class Generator
{
public:
	Generator(int columns, int rows, QRandomGenerator& random);

	QList<QList<Tile*>> pieces() const;

private:
	void solve();

private:
	int m_columns;
	int m_rows;
	QList<QList<Tile*>> m_pieces;
	QRandomGenerator& m_random;
};

inline QList<QList<Tile*>> Generator::pieces() const
{
	return m_pieces;
}

#endif // TETZLE_GENERATOR_H
