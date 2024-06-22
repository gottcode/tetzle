/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_FRAGMENT_LIST_H
#define TETZLE_FRAGMENT_LIST_H

#include <QPainter>

class FragmentList
{
public:
	void append(const QPointF& pos, const QRectF& source, qreal rotation = 0)
	{
		m_list.append(QPainter::PixmapFragment::create(pos, source, 1, 1, rotation));
	}

	void append(const FragmentList& list)
	{
		m_list.append(list.m_list);
	}

	void clear()
	{
		m_list.clear();
	}

	void reserve(qsizetype size)
	{
		m_list.reserve(size);
	}

	void draw(QPainter& painter, const QPixmap& pixmap, QPainter::PixmapFragmentHints hints = QPainter::PixmapFragmentHints()) const
	{
		painter.drawPixmapFragments(m_list.constData(), m_list.size(), pixmap, hints);
	}

private:
	QList<QPainter::PixmapFragment> m_list;
};

#endif // TETZLE_FRAGMENT_LIST_H
