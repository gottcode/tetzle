/*
	SPDX-FileCopyrightText: 2008-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "generator.h"

#include "dancing_links.h"
#include "tile.h"

#include <algorithm>

//-----------------------------------------------------------------------------

namespace
{

struct Shape
{
	Shape(const QPoint& p1, const QPoint& p2, const QPoint& p3, const QPoint& p4);

	QPoint cells[4];
	int width;
	int height;
};

Shape::Shape(const QPoint& p1, const QPoint& p2, const QPoint& p3, const QPoint& p4)
	: width(0)
	, height(0)
{
	cells[0] = p1;
	cells[1] = p2;
	cells[2] = p3;
	cells[3] = p4;
	for (int i = 0; i < 4; ++i) {
		width = std::max(width, cells[i].x());
		height = std::max(height, cells[i].y());
	}
}

}

//-----------------------------------------------------------------------------

Generator::Generator(int columns, int rows, QRandomGenerator& random)
	: m_columns(columns)
	, m_rows(rows)
	, m_random(random)
{
	do {
		m_pieces.clear();
		solve();
	} while (m_pieces.isEmpty());
}

//-----------------------------------------------------------------------------

void Generator::solve()
{
	QList<Shape> shapes;

	// Add S
	shapes.append(Shape(QPoint(1,0), QPoint(2,0), QPoint(0,1), QPoint(1,1)));
	shapes.append(Shape(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(1,2)));
	// Add Z
	shapes.append(Shape(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(2,1)));
	shapes.append(Shape(QPoint(1,0), QPoint(0,1), QPoint(1,1), QPoint(0,2)));
	// Add O
	shapes.append(Shape(QPoint(0,0), QPoint(1,0), QPoint(0,1), QPoint(1,1)));
	// Add T
	shapes.append(Shape(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(2,0)));
	shapes.append(Shape(QPoint(1,0), QPoint(0,1), QPoint(1,1), QPoint(1,2)));
	shapes.append(Shape(QPoint(0,1), QPoint(1,1), QPoint(1,0), QPoint(2,1)));
	shapes.append(Shape(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(0,2)));
	// Add J
	shapes.append(Shape(QPoint(1,0), QPoint(1,1), QPoint(0,2), QPoint(1,2)));
	shapes.append(Shape(QPoint(0,0), QPoint(0,1), QPoint(1,1), QPoint(2,1)));
	shapes.append(Shape(QPoint(0,0), QPoint(1,0), QPoint(0,1), QPoint(0,2)));
	shapes.append(Shape(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(2,1)));
	// Add L
	shapes.append(Shape(QPoint(0,0), QPoint(0,1), QPoint(0,2), QPoint(1,2)));
	shapes.append(Shape(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(0,1)));
	shapes.append(Shape(QPoint(0,0), QPoint(1,0), QPoint(1,1), QPoint(1,2)));
	shapes.append(Shape(QPoint(0,1), QPoint(1,1), QPoint(2,0), QPoint(2,1)));
	// Add I
	shapes.append(Shape(QPoint(0,0), QPoint(1,0), QPoint(2,0), QPoint(3,0)));
	shapes.append(Shape(QPoint(0,0), QPoint(0,1), QPoint(0,2), QPoint(0,3)));

	// Create matrix
	DLX::Matrix matrix(m_columns * m_rows);

	int size = shapes.size();
	QList<int> ids;
	for (int i = 0; i < size; ++i) {
		ids.append(i);
	}

	QList<int> cells;
	for (int i = 0; i < m_columns * m_rows; ++i) {
		cells.append(i);
	}
	std::shuffle(cells.begin(), cells.end(), m_random);

	int cell, col, row;
	for (int i = 0; i < m_columns * m_rows; ++i) {
		cell = cells.at(i);
		row = cell / m_columns;
		col = cell - (row * m_columns);

		std::shuffle(ids.begin(), ids.end(), m_random);
		for (int i = 0; i < size; ++i) {
			const Shape& shape = shapes.at(ids.at(i));
			if (shape.width + col < m_columns && shape.height + row < m_rows) {
				matrix.addRow();
				for (int i = 0; i < 4; ++i) {
					matrix.addElement((shape.cells[i].y() + row) * m_columns + shape.cells[i].x() + col);
				}
			}
		}
	}

	// Generate solution
	matrix.search(1, m_columns * m_rows);

	const QList<DLX::Node*> rows = matrix.solution();
	QList<Tile*> piece;
	for (int i = 0; i < rows.count(); ++i) {
		piece.clear();
		DLX::Node* j = rows[i];
		do {
			unsigned int r = j->column->id / m_columns;
			unsigned int c = j->column->id - (r * m_columns);
			piece.append(new Tile(c, r));
			j = j->right;
		} while (j != rows[i]);
		m_pieces.append(piece);
	}
}

//-----------------------------------------------------------------------------
