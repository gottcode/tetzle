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

#include "generator.h"

#include "dancing_links.h"

#include <algorithm>

//-----------------------------------------------------------------------------

namespace
{

struct Piece
{
	Piece(const QPoint& p1, const QPoint& p2, const QPoint& p3, const QPoint& p4);

	QPoint cells[4];
	int width;
	int height;
};

Piece::Piece(const QPoint& p1, const QPoint& p2, const QPoint& p3, const QPoint& p4)
	: width(0),
	height(0)
{
	cells[0] = p1;
	cells[1] = p2;
	cells[2] = p3;
	cells[3] = p4;
	for (int i = 0; i < 4; ++i) {
		width = qMax(width, cells[i].x());
		height = qMax(height, cells[i].y());
	}
}

}

//-----------------------------------------------------------------------------

Generator::Generator(int columns, int rows)
	: m_columns(columns),
	m_rows(rows)
{
	do {
		m_pieces.clear();
		solve();
	} while (m_pieces.isEmpty());
}

//-----------------------------------------------------------------------------

void Generator::solve()
{
	QList<Piece> p;

	// Add S
	p.append(Piece(QPoint(1,0), QPoint(2,0), QPoint(0,1), QPoint(1,1)));
	p.append(Piece(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(1,2)));
	// Add Z
	p.append(Piece(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(2,1)));
	p.append(Piece(QPoint(1,0), QPoint(0,1), QPoint(1,1), QPoint(0,2)));
	// Add O
	p.append(Piece(QPoint(0,0), QPoint(1,0), QPoint(0,1), QPoint(1,1)));
	// Add T
	p.append(Piece(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(2,0)));
	p.append(Piece(QPoint(1,0), QPoint(0,1), QPoint(1,1), QPoint(1,2)));
	p.append(Piece(QPoint(0,1), QPoint(1,1), QPoint(1,0), QPoint(2,1)));
	p.append(Piece(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(0,2)));
	// Add J
	p.append(Piece(QPoint(1,0), QPoint(1,1), QPoint(0,2), QPoint(1,2)));
	p.append(Piece(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(2,1)));
	p.append(Piece(QPoint(0,0), QPoint(1,0), QPoint(0,1), QPoint(0,2)));
	p.append(Piece(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(2,1)));
	// Add L
	p.append(Piece(QPoint(0,0), QPoint(0,1), QPoint(0,2), QPoint(1,2)));
	p.append(Piece(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(0,1)));
	p.append(Piece(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(1,2)));
	p.append(Piece(QPoint(0,1), QPoint(1,1), QPoint(2,0), QPoint(2,1)));
	// Add I
	p.append(Piece(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(3,0)));
	p.append(Piece(QPoint(0,0), QPoint(0,1), QPoint(0,2), QPoint(0,3)));

	// Create matrix
	DLX::Matrix matrix(m_columns * m_rows);

	int size = p.size();
	QList<int> ids;
	for (int i = 0; i < size; ++i) {
		ids.append(i);
	}

	QList<int> cells;
	for (int i = 0; i < m_columns * m_rows; ++i) {
		cells.append(i);
	}
	std::random_shuffle(cells.begin(), cells.end());

	int cell, col, row;
	for (int i = 0; i < m_columns * m_rows; ++i) {
		cell = cells.at(i);
		row = cell / m_columns;
		col = cell - (row * m_columns);

		std::random_shuffle(ids.begin(), ids.end());
		for (int i = 0; i < size; ++i) {
			const Piece& piece = p.at(ids.at(i));
			if (piece.width + col < m_columns && piece.height + row < m_rows) {
				matrix.addRow();
				for (int i = 0; i < 4; ++i) {
					matrix.addElement((piece.cells[i].y() + row) * m_columns + piece.cells[i].x() + col);
				}
			}
		}
	}

	// Generate solution
	matrix.search(this, &Generator::solution, 1, m_columns * m_rows);
}

//-----------------------------------------------------------------------------

void Generator::solution(const QVector<DLX::Node*>& rows, unsigned int count)
{
	QList<QPoint> piece;
	for (unsigned int i = 0; i < count; ++i) {
		piece.clear();
		DLX::Node* j = rows[i];
		do {
			unsigned int r = j->column->id / m_columns;
			unsigned int c = j->column->id - (r * m_columns);
			piece.append(QPoint(c, r));
			j = j->right;
		} while (j != rows[i]);
		m_pieces.append(piece);
	}
}

//-----------------------------------------------------------------------------
