/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_TAG_ITEM_H
#define TETZLE_TAG_ITEM_H

#include <QListWidgetItem>

/**
 * Tag listwidget item.
 */
class TagItem : public QListWidgetItem
{
public:
	/**
	 * Construct a tag item.
	 *
	 * @param text the text to display
	 */
	explicit TagItem(const QString& tag = QString())
		: QListWidgetItem(tag)
	{
		setData(Qt::UserRole, tag);
		setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
	}

	/**
	 * Compare two strings using locale-aware comparison.
	 *
	 * @param a first string
	 * @param b second string
	 *
	 * @return @c true if @a is less than @b
	 */
	static bool compare(const QString& a, const QString& b)
	{
		return a.localeAwareCompare(b) < 0;
	}

	/**
	 * Compare item with another item using locale-aware comparison.
	 *
	 * @param other the item to compare
	 *
	 * @return @c true if less than other item
	 */
	bool operator<(const QListWidgetItem& other) const override
	{
		return compare(text(), other.text());
	}
};

#endif // TETZLE_TAG_ITEM_H
