/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include <QHash>
#include <QStringList>
#include <QWidget>
class QComboBox;

class TagManager : public QWidget
{
	Q_OBJECT

public:
	TagManager(QWidget* parent = 0);

	QStringList images(const QString& tag) const;
	QStringList tags() const;

	void clearFilter();
	void setImageTags(const QString& image, const QStringList& tags);

signals:
	void filterChanged(const QStringList& images);

private slots:
	bool addTag(const QString& tag);
	bool renameTag(const QString& tag, const QString& old_tag);
	bool removeTag(const QString& tag);
	void updateFilter();

private:
	void storeTags();

private:
	QHash<QString, QStringList> m_tags;
	QComboBox* m_filter;
};

#endif
