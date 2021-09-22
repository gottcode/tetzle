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
#include <QVector>

#include <random>

class Generator
{
public:
	Generator(int columns, int rows, std::mt19937& random);

	QList<QList<Tile*>> pieces() const;

private:
	void solve();
	void solution(const QVector<DLX::Node*>& rows);

private:
	int m_columns;
	int m_rows;
	QList<QList<Tile*>> m_pieces;
	std::mt19937& m_random;
};

inline QList<QList<Tile*>> Generator::pieces() const
{
	return m_pieces;
}

#endif // TETZLE_GENERATOR_H
