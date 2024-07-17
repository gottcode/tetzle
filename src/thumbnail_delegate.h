/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_THUMBNAIL_DELEGATE_H
#define TETZLE_THUMBNAIL_DELEGATE_H

#include <QFontMetrics>
#include <QStyledItemDelegate>
class QListWidget;

/**
 * Custom delegate for showing thumbnails
 */
class ThumbnailDelegate : public QStyledItemDelegate
{
public:
	/**
	 * Construct a delegate.
	 *
	 * @param list the listwidget the delegate is for, used for fetching fonts.
	 */
	explicit ThumbnailDelegate(QListWidget* list = nullptr);

	/**
	 * Draw the delegate.
	 *
	 * @param painter the painter to use for drawing
	 * @param option the options to use for drawing
	 * @param index the item to draw
	 */
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	/**
	 * Calculate the minimum size of an item.
	 *
	 * @param option the options to use for drawing
	 * @param index the item to find size for
	 *
	 * @return minimum size of an item
	 */
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected:
	/**
	 * Handle font change.
	 */
	bool eventFilter(QObject* object, QEvent* event) override;

private:
	/**
	 * Find small version of font.
	 *
	 * @param font the font to start from
	 */
	void setFont(const QFont& font);

private:
	QListWidget* m_list; ///< listwidget for delegate
	QFont m_small_font; ///< font for secondary text
	QFontMetrics m_small_font_metrics; ///< font metrics for secondary text
};

#endif // TETZLE_THUMBNAIL_DELEGATE_H
