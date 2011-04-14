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

#include "tag_manager.h"

#include "path.h"

#include <QDir>
#include <QSettings>

//-----------------------------------------------------------------------------

TagManager::TagManager(QObject* parent)
	: QObject(parent)
{
	QSettings file(Path::image("tags"), QSettings::IniFormat);
	file.beginGroup("Tags");
	QStringList tags = file.childKeys();
	QStringList images;
	QDir folder(Path::images(), "*.*");
	foreach (QString tag, tags) {
		images = file.value(tag).toStringList();
		QMutableStringListIterator i(images);
		while (i.hasNext()) {
			i.next();
			if (!folder.exists(i.value())) {
				i.remove();
			}
		}
		m_tags[tag] = images;
	}
}

//-----------------------------------------------------------------------------

QStringList TagManager::images(const QString& tag) const
{
	if (tag != tr("All Tags")) {
		return m_tags.value(tag);
	} else {
		return QDir(Path::images(), "*.*", QDir::Name | QDir::LocaleAware, QDir::Files).entryList();
	}
}

//-----------------------------------------------------------------------------

QStringList TagManager::tags() const
{
	QStringList tags = m_tags.keys();
	tags.sort();
	tags.prepend(tr("All Tags"));
	return tags;
}

//-----------------------------------------------------------------------------

bool TagManager::addTag(const QString& tag)
{
	if (tag == tr("All Tags") || tag.isEmpty() || m_tags.constFind(tag) != m_tags.constEnd()) {
		return false;
	}

	m_tags.insert(tag, QStringList());
	storeTags();

	return true;
}

//-----------------------------------------------------------------------------

bool TagManager::renameTag(const QString& tag, const QString& old_tag)
{
	if (m_tags.constFind(tag) != m_tags.constEnd() || m_tags.constFind(old_tag) == m_tags.constEnd() || tag.isEmpty()) {
		return false;
	}

	m_tags.insert(tag, m_tags.take(old_tag));
	storeTags();

	return true;
}

//-----------------------------------------------------------------------------

bool TagManager::removeTag(const QString& tag)
{
	if (m_tags.constFind(tag) == m_tags.constEnd()) {
		return false;
	}

	m_tags.remove(tag);
	storeTags();

	return true;
}

//-----------------------------------------------------------------------------

void TagManager::setImageTags(const QString& image, const QStringList& tags)
{
	bool changed = false;
	QMutableHashIterator<QString, QStringList> i(m_tags);
	while (i.hasNext()) {
		i.next();
		bool tagged = tags.contains(i.key());
		QStringList& tag = i.value();
		if (tag.contains(image)) {
			if (!tagged) {
				changed = true;
				tag.removeAll(image);
			}
		} else {
			if (tagged) {
				changed = true;
				tag.append(image);
			}
		}
	}
	if (changed) {
		storeTags();
	}
}

//-----------------------------------------------------------------------------

void TagManager::storeTags()
{
	QSettings file(Path::image("tags"), QSettings::IniFormat);
	file.clear();
	file.beginGroup("Tags");
	QHashIterator<QString, QStringList> i(m_tags);
	while (i.hasNext()) {
		i.next();
		file.setValue(i.key(), i.value());
	}
}

//-----------------------------------------------------------------------------
