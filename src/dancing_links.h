/***********************************************************************
 *
 * Copyright (C) 2008, 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef DANCING_LINKS_H
#define DANCING_LINKS_H

#include <QLinkedList>
#include <QVector>

namespace DLX
{

struct HeaderNode;
struct Node
{
	Node()
		: left(0), right(0), up(0), down(0), column(0)
	{
	}

	Node* left;
	Node* right;
	Node* up;
	Node* down;
	HeaderNode* column;
};

struct HeaderNode : public Node
{
	HeaderNode()
		: size(0), id(0)
	{
	}

	unsigned int size;
	unsigned int id;
};

class Matrix
{
	class Callback
	{
	public:
		virtual ~Callback()
		{
		}

		virtual void operator()(const QVector<Node*>&, unsigned int)
		{
		}
	};

	class GlobalCallback : public Callback
	{
		typedef void(*function)(const QVector<Node*>&, unsigned int);
	public:
		GlobalCallback(function f)
			: m_function(f)
		{
		}

		virtual void operator()(const QVector<Node*>& rows, unsigned int count)
		{
			(*m_function)(rows, count);
		}

	private:
		function m_function;
	};

	template <typename T>
	class MemberCallback : public Callback {
		typedef void(T::*function)(const QVector<Node*>& rows, unsigned int count);
	public:
		MemberCallback(T* object, function f)
			: m_object(object), m_function(f)
		{
		}

		virtual void operator()(const QVector<Node*>& rows, unsigned int count)
		{
			(*m_object.*m_function)(rows, count);
		}

	private:
		T* m_object;
		function m_function;
	};

public:
	Matrix(unsigned int max_columns);
	~Matrix();

	void addRow();
	void addElement(unsigned int column);

	unsigned int search(unsigned int max_solutions = 0xFFFFFFFF)
	{
		Callback solution;
		return search(&solution, max_solutions);
	}

	unsigned int search(void(*function)(const QVector<Node*>& rows, unsigned int count), unsigned int max_solutions = 0xFFFFFFFF)
	{
		GlobalCallback solution(function);
		return search(&solution, max_solutions);
	}

	template <typename C>
	unsigned int search(C* object, void(C::*function)(const QVector<Node*>& rows, unsigned int count), unsigned int max_solutions = 0xFFFFFFFF)
	{
		MemberCallback<C> solution(object, function);
		return search(&solution, max_solutions);
	}

private:
	unsigned int search(Callback* solution, unsigned int max_solutions);
	void solve(unsigned int k);
	void cover(HeaderNode* column);
	void uncover(HeaderNode* column);

private:
	unsigned int m_max_columns;

	HeaderNode* m_header;
	QVector<HeaderNode> m_columns;
	QLinkedList<HeaderNode> m_rows;
	QLinkedList<Node> m_nodes;
	QVector<Node*> m_output;

	Callback* m_solution;
	unsigned int m_solutions;
	unsigned int m_max_solutions;
};

}

#endif
