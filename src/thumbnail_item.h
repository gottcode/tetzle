/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_THUMBNAIL_ITEM_H
#define TETZLE_THUMBNAIL_ITEM_H

#include <QListWidgetItem>

/**
 * Thumbnail listwidget item.
 */
class ThumbnailItem : public QListWidgetItem
{
public:
	/**
	 * Construct a thumbnail item.
	 *
	 * @param text the text of the item
	 */
	explicit ThumbnailItem(const QString& text = QString())
		: QListWidgetItem(QIcon::fromTheme("image-loading"), text)
	{
	}

	/**
	 * Compare text and image identifier to another thumbnail item using locale-aware comparison.
	 *
	 * @param other thumbnail item to compare with
	 */
	bool operator<(const QListWidgetItem& other) const override
	{
		int compare = text().localeAwareCompare(other.text());
		if (compare == 0) {
			compare = data(ImageRole).toString().localeAwareCompare(other.data(ImageRole).toString());
		}
		return compare < 0;
	}

	/**
	 * Roles for the thumbnail item data.
	 */
	enum ItemRoles
	{
		ImageRole = Qt::UserRole, ///< image identifier

		SmallDisplayRole, ///< secondary text
		DetailsRole = SmallDisplayRole, ///< details of the game; piece count and completion
		TagsRole = SmallDisplayRole, ///< list of tags

		GameRole, ///< game identifier
		NameRole ///< image name
	};
};

#endif // TETZLE_THUMBNAIL_ITEM_H
