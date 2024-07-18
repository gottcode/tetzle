/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_TAG_ITEM_H
#define TETZLE_TAG_ITEM_H

#include <QListWidgetItem>

class TagItem : public QListWidgetItem
{
public:
	explicit TagItem(const QString& tag = QString())
		: QListWidgetItem(tag)
	{
		setData(Qt::UserRole, tag);
		setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
	}

	static bool compare(const QString& a, const QString& b)
	{
		return a.localeAwareCompare(b) < 0;
	}

	bool operator<(const QListWidgetItem& other) const override
	{
		return compare(text(), other.text());
	}
};

#endif // TETZLE_TAG_ITEM_H
