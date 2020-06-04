/***********************************************************************
 *
 * Copyright (C) 2008, 2009, 2013 Graeme Gott <graeme@gottcode.org>
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

#ifndef TETZLE_DANCING_LINKS_H
#define TETZLE_DANCING_LINKS_H

#include <QVector>

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
	Node() :
		left(0),
		right(0),
		up(0),
		down(0),
		column(0)
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
	HeaderNode() :
		size(0),
		id(0)
	{
	}

	unsigned int size; /**< how many nodes with value of 1 are in column */
	unsigned int id; /**< unique identifier */
};

/** Sparse matrix class. */
class Matrix
{
	/** Abstract base class for solution callback. */
	class Callback
	{
	public:
		/** Destroy collback. */
		virtual ~Callback()
		{
		}

		/** Empty function to allow for ignored callbacks. */
		virtual void operator()(const QVector<Node*>&, unsigned int)
		{
		}
	};

	/** Callback using non-member function. */
	class GlobalCallback : public Callback
	{
	public:
		typedef void(*function)(const QVector<Node*>&);

		/**
		 * Constructs callback.
		 *
		 * @param f non-member function to use as callback
		 */
		GlobalCallback(function f) :
			m_function(f)
		{
		}

		/** Perform callback using non-member function. */
		void operator()(const QVector<Node*>& rows, unsigned int count)
		{
			(*m_function)(rows.mid(0, count));
		}

	private:
		function m_function; /**< non-member function to use as callback */
	};

	/** Callback using member function */
	template <typename T>
	class MemberCallback : public Callback
	{
	public:
		typedef void(T::*function)(const QVector<Node*>& rows);

		/**
		 * Constructs callback.
		 *
		 * @param object pointer to object of callback
		 * @param f member function of @p object to use as callback
		 */
		MemberCallback(T* object, function f) :
			m_object(object),
			m_function(f)
		{
		}

		/** Perform callback using member function. */
		void operator()(const QVector<Node*>& rows, unsigned int count)
		{
			(*m_object.*m_function)(rows.mid(0, count));
		}

	private:
		T* m_object; /**< pointer to object of callback */
		function m_function; /**< member function of @p object to use as callback */
	};

public:
	/** Constructs a matrix with @p max_columns number of columns. */
	Matrix(unsigned int max_columns);

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
	 * @param max_solutions maximum allowed solutions
	 * @param max_tries maximum allowed attempts before stopping search
	 * @return total count of solutions
	 */
	unsigned int search(unsigned int max_solutions = 0xFFFFFFFF, unsigned int max_tries = 0xFFFFFFFF)
	{
		Callback solution;
		return search(&solution, max_solutions, max_tries);
	}

	/**
	 * Search for solutions.
	 *
	 * @param function non-member function called with each solution
	 * @param max_solutions maximum allowed solutions before stopping search
	 * @param max_tries maximum allowed attempts before stopping search
	 * @return total count of solutions
	 */
	unsigned int search(void(*function)(const QVector<Node*>& rows), unsigned int max_solutions = 0xFFFFFFFF, unsigned int max_tries = 0xFFFFFFFF)
	{
		GlobalCallback solution(function);
		return search(&solution, max_solutions, max_tries);
	}

	/**
	 * Search for solutions.
	 *
	 * @param object pointer to object of callback
	 * @param function member function of @p object called with each solution
	 * @param max_solutions maximum allowed solutions before stopping search
	 * @param max_tries maximum allowed attempts before stopping search
	 * @return total count of solutions
	 */
	template <typename T>
	unsigned int search(T* object, void(T::*function)(const QVector<Node*>& rows), unsigned int max_solutions = 0xFFFFFFFF, unsigned int max_tries = 0xFFFFFFFF)
	{
		MemberCallback<T> solution(object, function);
		return search(&solution, max_solutions, max_tries);
	}

private:
	/**
	 * Performs the search for solutions.
	 *
	 * @param solution function called with each solution
	 * @param max_solutions maximum allowed solutions before stopping search
	 * @param max_tries maximum allowed attempts before stopping search
	 * @return total count of solutions
	 */
	unsigned int search(Callback* solution, unsigned int max_solutions, unsigned int max_tries);

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
	QVector<HeaderNode> m_columns; /**< constraints */
	std::list<HeaderNode> m_rows; /**< rows */
	std::list<Node> m_nodes; /**< row values */
	QVector<Node*> m_output; /**< rows where columns do not conflict */

	Callback* m_solution; /**< function to call when a solution is found */
	unsigned int m_solutions; /**< how many solutions have been found so far */
	unsigned int m_max_solutions; /**< maximum allowed solutions */
	unsigned int m_tries; /**< how many attempts have been made so far */
	unsigned int m_max_tries; /**< maximum allowed attempts */
};

}

#endif // TETZLE_DANCING_LINKS_H
