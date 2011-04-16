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
#include "toolbar_list.h"

#include <QAction>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

TagManager::TagManager(QWidget* parent)
	: QWidget(parent)
{
	// Add filter
	m_filter = new ToolBarList(this);
	m_filter->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_filter->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(m_filter, SIGNAL(currentRowChanged(int)), this, SLOT(currentTagChanged(int)));
	connect(m_filter, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(tagChanged(QListWidgetItem*)));

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(m_filter);

	// Add filter actions
	QAction* add_action = new QAction(QIcon::fromTheme("list-add", QPixmap(":/tango/list-add.png")), tr("Add Tag"), this);
	m_filter->addToolBarAction(add_action);
	connect(add_action, SIGNAL(triggered()), this, SLOT(addTag()));

	m_remove_action = new QAction(QIcon::fromTheme("list-remove", QPixmap(":/tango/list-remove.png")), tr("Remove Tag"), this);
	m_filter->addToolBarAction(m_remove_action);
	connect(m_remove_action, SIGNAL(triggered()), this, SLOT(removeTag()));

	// Add tags
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

		QListWidgetItem* item = new QListWidgetItem(tag, m_filter);
		item->setData(Qt::UserRole, item->text());
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
	}
	m_filter->sortItems();

	QListWidgetItem* item = new QListWidgetItem(tr("All Images"));
	item->setData(Qt::UserRole, item->text());
	m_filter->insertItem(0, item);
	m_filter->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------

QStringList TagManager::images(const QString& tag) const
{
	return m_tags.value(tag);
}

//-----------------------------------------------------------------------------

QStringList TagManager::tags() const
{
	QStringList tags = m_tags.keys();
	tags.sort();
	return tags;
}

//-----------------------------------------------------------------------------

void TagManager::clearFilter()
{
	m_filter->setCurrentRow(0, QItemSelectionModel::ClearAndSelect);
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
		updateFilter();
	}
}

//-----------------------------------------------------------------------------

void TagManager::addTag()
{
	m_remove_action->setEnabled(true);

	// Add tag
	static int new_tags = 0;
	new_tags++;
	QString tag = tr("Untitled %1").arg(new_tags);
	m_tags.insert(tag, QStringList());
	storeTags();

	// Add tag item
	QListWidgetItem* item = new QListWidgetItem(tag, m_filter);
	item->setData(Qt::UserRole, item->text());
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
	m_filter->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
	m_filter->editItem(item);
}

//-----------------------------------------------------------------------------

void TagManager::removeTag()
{
	// Find current item
	QListWidgetItem* item = m_filter->currentItem();
	if (!item || m_filter->row(item) == 0) {
		return;
	}

	// Ask before removing
	if (QMessageBox::question(window(), tr("Question"), tr("Remove selected tag?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
		return;
	}

	// Remove item
	QString tag = item->text();
	delete item;
	item = 0;
	m_remove_action->setEnabled(m_filter->count() > 1);

	// Remove tag
	m_tags.remove(tag);
	storeTags();
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::currentTagChanged(int current)
{
	m_remove_action->setEnabled(current > 0);
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::tagChanged(QListWidgetItem* item)
{
	if (item == 0) {
		return;
	}

	// Rename tag
	m_filter->blockSignals(true);
	QString tag = item->text();
	QString old_tag = item->data(Qt::UserRole).toString();
	if (tag != old_tag) {
		if (tag.isEmpty()) {
			item->setText(old_tag);
		} else if (m_tags.contains(tag)) {
			item->setText(old_tag);
			QMessageBox::warning(window(), tr("Sorry"), tr("A tag with that name already exists."));
		} else {
			item->setData(Qt::UserRole, tag);

			m_tags.insert(tag, m_tags.take(old_tag));
			storeTags();

			QListWidgetItem* all_images = m_filter->takeItem(0);
			m_filter->sortItems();
			m_filter->insertItem(0, all_images);
		}
	}
	m_filter->blockSignals(false);

	// Update list of images
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::updateFilter()
{
	QStringList filter;
	if (m_filter->currentRow() > 0) {
		filter = images(m_filter->currentItem()->text());
	} else {
		filter = QDir(Path::images(), "*.*", QDir::Name | QDir::LocaleAware, QDir::Files).entryList();
	}
	emit filterChanged(filter);
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
