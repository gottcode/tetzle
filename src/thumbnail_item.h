/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_THUMBNAIL_ITEM_H
#define TETZLE_THUMBNAIL_ITEM_H

#include <QListWidgetItem>

class ThumbnailItem : public QListWidgetItem
{
public:
	explicit ThumbnailItem(const QString& text = QString())
		: QListWidgetItem(QIcon::fromTheme("image-loading"), text)
	{
	}

	bool operator<(const QListWidgetItem& other) const override
	{
		int compare = text().localeAwareCompare(other.text());
		if (compare == 0) {
			compare = data(ImageRole).toString().localeAwareCompare(other.data(ImageRole).toString());
		}
		return compare < 0;
	}

	enum ItemRoles
	{
		ImageRole = Qt::UserRole,

		SmallDisplayRole,
		DetailsRole = SmallDisplayRole,
		TagsRole = SmallDisplayRole,

		GameRole,
		NameRole
	};
};

#endif // TETZLE_THUMBNAIL_ITEM_H
