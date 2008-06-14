/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#include "solver.h"

#include "dancing_links.h"

#include <algorithm>

/*****************************************************************************/

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
	:	width(0),
		height(0)
	{
		cells[0] = p1;
		cells[1] = p2;
		cells[2] = p3;
		cells[3] = p4;
		for (int i = 0; i < 4; ++i) {
			if (width < cells[i].x())
				width = cells[i].x();
			if (height < cells[i].y())
				height = cells[i].y();
		}
	}

}

/*****************************************************************************/

Solver::Solver(int columns, int rows, int total)
:	m_columns(columns),
	m_rows(rows)
{
	std::vector<Piece> p;

	// Add S
	p.push_back(Piece(QPoint(1,0), QPoint(2,0), QPoint(0,1), QPoint(1,1)));
	p.push_back(Piece(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(1,2)));
	// Add Z
	p.push_back(Piece(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(2,1)));
	p.push_back(Piece(QPoint(1,0), QPoint(0,1), QPoint(1,1), QPoint(0,2)));
	// Add O
	p.push_back(Piece(QPoint(0,0), QPoint(1,0), QPoint(0,1), QPoint(1,1)));
	// Add T
	p.push_back(Piece(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(2,0)));
	p.push_back(Piece(QPoint(1,0), QPoint(0,1), QPoint(1,1), QPoint(1,2)));
	p.push_back(Piece(QPoint(0,1), QPoint(1,1), QPoint(1,0), QPoint(2,1)));
	p.push_back(Piece(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(0,2)));
	// Add J
	p.push_back(Piece(QPoint(1,0), QPoint(1,1), QPoint(0,2), QPoint(1,2)));
	p.push_back(Piece(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(2,1)));
	p.push_back(Piece(QPoint(0,0), QPoint(1,0), QPoint(0,1), QPoint(0,2)));
	p.push_back(Piece(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(2,1)));
	// Add L
	p.push_back(Piece(QPoint(0,0), QPoint(0,1), QPoint(0,2), QPoint(1,2)));
	p.push_back(Piece(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(0,1)));
	p.push_back(Piece(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(1,2)));
	p.push_back(Piece(QPoint(0,1), QPoint(1,1), QPoint(2,0), QPoint(2,1)));
	// Add I
	p.push_back(Piece(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(3,0)));
	p.push_back(Piece(QPoint(0,0), QPoint(0,1), QPoint(0,2), QPoint(0,3)));

	// Create matrix
	DLX::Matrix matrix(columns * rows);

	int size = p.size();
	std::vector<int> ids;
	for (int i = 0; i < size; ++i)
		ids.push_back(i);

	std::vector<int> cells;
	for (int i = 0; i < columns * rows; ++i)
		cells.push_back(i);
	std::random_shuffle(cells.begin(), cells.end());

	int cell, col, row;
	for (int i = 0; i < columns * rows; ++i) {
		cell = cells.at(i);
		row = cell / columns;
		col = cell - (row * columns);

		std::random_shuffle(ids.begin(), ids.end());
		for (int i = 0; i < size; ++i) {
			const Piece& piece = p.at(ids.at(i));
			if (piece.width + col < columns && piece.height + row < rows) {
				matrix.addRow();
				for (int i = 0; i < 4; ++i) {
					matrix.addElement((piece.cells[i].y() + row) * columns + piece.cells[i].x() + col);
				}
			}
		}
	}

	// Generate solution
	matrix.search(this, &Solver::solution, total);
}

/*****************************************************************************/

void Solver::solution(const std::vector<DLX::Node*>& rows, unsigned int count)
{
	QList< QList<QPoint> > pieces;
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
		pieces.append(piece);
	}
	solutions.append(pieces);
}

/*****************************************************************************/
