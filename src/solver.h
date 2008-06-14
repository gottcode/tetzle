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

#ifndef SOLVER_H
#define SOLVER_H

#include <QList>
#include <QPoint>

#include <vector>
namespace DLX
{
	struct Node;
}

class Solver
{
public:
	Solver(int columns, int rows, int total = 1);

	QList< QList< QList<QPoint> > > solutions;

private:
	void solution(const std::vector<DLX::Node*>& rows, unsigned int count);

	int m_columns;
	int m_rows;
};

#endif // SOLVER_H
