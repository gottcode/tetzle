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

#ifndef TAG_MANAGER_H
#define TAG_MANAGER_H

#include <QMap>
#include <QSet>
#include <QStringList>

class TagManager : public QObject
{
public:
	TagManager(QObject* parent = 0);

	QStringList tags(bool list_empty = false) const;
	QStringList images(const QString& tag) const;
	bool isTagEmpty(const QString& tag) const;

	bool addTag(const QString& tag);
	bool renameTag(const QString& tag, const QString& old_tag);
	bool removeTag(const QString& tag);
	void addImage(const QString& image, const QString& tag);
	void removeImage(const QString& image, const QString& tag);
	void removeImage(const QString& image);

private:
	void storeTags();

private:
	QMap<QString, QStringList> m_tags;
};

#endif
