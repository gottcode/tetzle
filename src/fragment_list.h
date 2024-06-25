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
	void append(const QPointF& pos, const QPointF& source_pos, qreal size, qreal rotation = 0)
	{
		m_list.append(QPainter::PixmapFragment::create(pos,
				QRectF(source_pos * m_pixelratio, QSizeF(size, size) * m_pixelratio),
				m_scale, m_scale,
				rotation));
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

	static void setDevicePixelRatio(qreal ratio)
	{
		m_pixelratio = ratio;
		m_scale = 1.0 / ratio;
	}

private:
	QList<QPainter::PixmapFragment> m_list;
	static qreal m_pixelratio;
	static qreal m_scale;
};

#endif // TETZLE_FRAGMENT_LIST_H
