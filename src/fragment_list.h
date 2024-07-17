/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_FRAGMENT_LIST_H
#define TETZLE_FRAGMENT_LIST_H

#include <QPainter>

/**
 * List of pixmap fragments for drawing.
 */
class FragmentList
{
public:
	/**
	 * Add a fragment to list.
	 *
	 * @param pos location of fragment in the scene
	 * @param source_pos location of fragment in the pixmap
	 * @param size size of fragment
	 * @param rotation rotation of fragment
	 */
	void append(const QPointF& pos, const QPointF& source_pos, qreal size, qreal rotation = 0)
	{
		m_list.append(QPainter::PixmapFragment::create(pos,
				QRectF(source_pos * m_pixelratio, QSizeF(size, size) * m_pixelratio),
				m_scale, m_scale,
				rotation));
	}

	/**
	 * Add fragments to list.
	 *
	 * @param list a list of fragments to append
	 */
	void append(const FragmentList& list)
	{
		m_list.append(list.m_list);
	}

	/**
	 * Remove all fragments from list.
	 */
	void clear()
	{
		m_list.clear();
	}

	/**
	 * Reserve memory to prevent resizing list as items are added.
	 *
	 * @param size how many fragments to allocate
	 */
	void reserve(qsizetype size)
	{
		m_list.reserve(size);
	}

	/**
	 * Draw fragments.
	 *
	 * @param painter the painter to use for drawing
	 * @param pixmap the pixmap to draw
	 * @param hints the hints to use for drawing
	 */
	void draw(QPainter& painter, const QPixmap& pixmap, QPainter::PixmapFragmentHints hints = QPainter::PixmapFragmentHints()) const
	{
		painter.drawPixmapFragments(m_list.constData(), m_list.size(), pixmap, hints);
	}

	/**
	 * Set the pixel ratio for rendering with high DPI. Default is 1.
	 *
	 * @param ratio the pixel ratio to render at
	 */
	static void setDevicePixelRatio(qreal ratio)
	{
		m_pixelratio = ratio;
		m_scale = 1.0 / ratio;
	}

private:
	QList<QPainter::PixmapFragment> m_list; ///< list of pixmap fragments
	static qreal m_pixelratio; ///< render at pixel ratio
	static qreal m_scale; ///< inverse of pixel ratio to keep coordinates at 1x
};

#endif // TETZLE_FRAGMENT_LIST_H
