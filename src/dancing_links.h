/*
	SPDX-FileCopyrightText: 2008-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_DANCING_LINKS_H
#define TETZLE_DANCING_LINKS_H

#include <QList>

#include <list>

/**
 * Dancing Links implementation of Algorithm X.
 *
 * Algorithm X is a recursive backtracking algorithm that finds all solutions
 * to the exact cover problem. It works on a matrix consisting of 0s an 1s.
 * The purpose is to find a combination of rows such that the digit 1 appears
 * in each column only once.
 *
 * To convert an exact cover problem into a sparse matrix solvable by
 * Algorithm X you represent each constraint by a column. Each possible value
 * is then placed into a row with 1s in the columns for the constraints it
 * matches.
 */
namespace DLX
{

struct HeaderNode;

/** %Node in matrix. */
struct Node
{
	/** Constructs a node with the value of 1. */
	explicit Node()
		: left(nullptr)
		, right(nullptr)
		, up(nullptr)
		, down(nullptr)
		, column(nullptr)
	{
	}

	Node* left; /**< node to the left with value of 1 */
	Node* right; /**< node to the right with value of 1 */
	Node* up; /**< node above with value of 1 */
	Node* down; /**< node below with value of 1 */
	HeaderNode* column; /**< column containing this node */
};

/** Head node of column or row in matrix. */
struct HeaderNode : public Node
{
	/** Constructs an empty column. */
	explicit HeaderNode()
		: size(0)
		, id(0)
	{
	}

	unsigned int size; /**< how many nodes with value of 1 are in column */
	unsigned int id; /**< unique identifier */
};

/** Sparse matrix class. */
class Matrix
{
public:
	/** Constructs a matrix with @p max_columns number of columns. */
	explicit Matrix(unsigned int max_columns);

	/** Clean up matrix. */
	~Matrix();

	/** Add row to matrix. */
	void addRow();

	/**
	 * Add element to matrix.
	 *
	 * @param column which column in current row to mark as filled
	 */
	void addElement(unsigned int column);

	/**
	 * Search for solutions.
	 *
	 * @param max_solutions maximum allowed solutions before stopping search
	 * @param max_tries maximum allowed attempts before stopping search
	 * @return total count of solutions
	 */
	unsigned int search(unsigned int max_solutions, unsigned int max_tries);

	/**
	 * Retrieve solution.
	 *
	 * @return last solution found
	 */
	QList<Node*> solution() const
	{
		return m_solution;
	}

private:
	/**
	 * Run Algorithm X at depth @p k.
	 *
	 * This is a recursive function that hides rows and columns and checks to
	 * see if a solution has been found.
	 */
	void solve(unsigned int k);

	/**
	 * Remove column or row from matrix.
	 *
	 * @param node head node of column or row to remove
	 */
	void cover(HeaderNode* node);

	/**
	 * Add column or row back to matrix.
	 *
	 * @param node head node of column or row to add
	 */
	void uncover(HeaderNode* node);

private:
	unsigned int m_max_columns; /**< amount of constraints */

	HeaderNode* m_header; /**< root element */
	QList<HeaderNode> m_columns; /**< constraints */
	std::list<HeaderNode> m_rows; /**< rows */
	std::list<Node> m_nodes; /**< row values */
	QList<Node*> m_output; /**< rows where columns do not conflict */
	QList<Node*> m_solution; /**< nodes of most recent solution */

	unsigned int m_solutions; /**< how many solutions have been found so far */
	unsigned int m_max_solutions; /**< maximum allowed solutions */
	unsigned int m_tries; /**< how many attempts have been made so far */
	unsigned int m_max_tries; /**< maximum allowed attempts */
};

}

#endif // TETZLE_DANCING_LINKS_H
