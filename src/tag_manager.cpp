/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "tag_manager.h"

#include "path.h"
#include "toolbar_list.h"

#include <QAction>
#include <QDir>
#include <QEvent>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

TagManager::TagManager(QWidget* parent)
	: QWidget(parent)
	, m_all_images_item(0)
{
	// Add filter
	m_filter = new ToolBarList(this);
	m_filter->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_filter->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(m_filter, &ToolBarList::currentItemChanged, this, &TagManager::currentTagChanged);
	connect(m_filter, &ToolBarList::itemChanged, this, &TagManager::tagChanged);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_filter);

	// Add filter actions
	QAction* add_action = new QAction(m_filter->fetchIcon("list-add"), tr("Add Tag"), this);
	m_filter->addToolBarAction(add_action);
	connect(add_action, &QAction::triggered, this, &TagManager::addTag);

	m_remove_action = new QAction(m_filter->fetchIcon("list-remove"), tr("Remove Tag"), this);
	m_filter->addToolBarAction(m_remove_action);
	connect(m_remove_action, &QAction::triggered, this, &TagManager::removeTag);

	// Add tags
	m_untagged_item = new QListWidgetItem(tr("Untagged"));
	m_untagged_item->setData(Qt::UserRole, m_untagged_item->text());
	m_filter->addItem(m_untagged_item);
	m_untagged = QDir(Path::images(), "*.*", QDir::Name | QDir::LocaleAware, QDir::Files).entryList();

	QSettings file(Path::image("tags"), QSettings::IniFormat);
	file.beginGroup("Tags");
	const QStringList tags = file.childKeys();
	QStringList images;
	QDir folder(Path::images(), "*.*");
	for (const QString& tag : tags) {
		images = file.value(tag).toStringList();
		QMutableStringListIterator i(images);
		while (i.hasNext()) {
			i.next();
			if (!folder.exists(i.value())) {
				i.remove();
			}
			m_untagged.removeAll(i.value());
		}
		m_tags[tag] = images;

		QListWidgetItem* item = new QListWidgetItem(tag);
		item->setData(Qt::UserRole, item->text());
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		m_filter->addItem(item);
	}
	m_filter->sortItems();

	m_all_images_item = new QListWidgetItem(tr("All Images"));
	m_all_images_item->setData(Qt::UserRole, m_all_images_item->text());
	QFont font = m_filter->font();
	font.setBold(true);
	m_all_images_item->setFont(font);
	m_filter->insertItem(0, m_all_images_item);
	m_filter->setCurrentItem(m_all_images_item, QItemSelectionModel::ClearAndSelect);
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

QString TagManager::tags(const QString& image) const
{
	QStringList tags;
	QHashIterator<QString, QStringList> i(m_tags);
	while (i.hasNext()) {
		i.next();
		if (i.value().contains(image)) {
			tags.append(i.key());
		}
	}
	tags.sort();
	return tags.join(", ");
}

//-----------------------------------------------------------------------------

void TagManager::clearFilter()
{
	m_filter->setCurrentItem(m_all_images_item, QItemSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------

void TagManager::addImage(const QString& image)
{
	m_untagged.append(image);
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::removeImage(const QString& image)
{
	setImageTags(image, QStringList());
	m_untagged.removeAll(image);
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::setImageTags(const QString& image, const QStringList& tags)
{
	if (!tags.isEmpty()) {
		m_untagged.removeAll(image);
	} else {
		m_untagged.append(image);
	}

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

void TagManager::changeEvent(QEvent* event)
{
	QWidget::changeEvent(event);
	if (event->type() == QEvent::FontChange) {
		QFont font = m_filter->font();
		font.setBold(true);
		m_all_images_item->setFont(font);
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
	QListWidgetItem* item = new QListWidgetItem(tag);
	item->setData(Qt::UserRole, item->text());
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
	m_filter->addItem(item);
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
	item = nullptr;
	m_remove_action->setEnabled(m_filter->count() > 1);

	// Remove tag
	m_tags.remove(tag);
	storeTags();
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::currentTagChanged(QListWidgetItem* item)
{
	m_remove_action->setEnabled((item != m_all_images_item) && (item != m_untagged_item));
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::tagChanged(QListWidgetItem* item)
{
	if (!item) {
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

			m_all_images_item = m_filter->takeItem(0);
			m_filter->sortItems();
			m_filter->insertItem(0, m_all_images_item);

			emit tagsChanged();
		}
	}
	m_filter->blockSignals(false);

	// Update list of images
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::updateFilter()
{
	QListWidgetItem* item = m_filter->currentItem();
	QStringList filter;
	if (item == m_all_images_item) {
		filter = m_untagged;
		QHashIterator<QString, QStringList> i(m_tags);
		while (i.hasNext()) {
			i.next();
			filter += i.value();
		}
		filter.removeDuplicates();
	} else if (item == m_untagged_item) {
		filter = m_untagged;
	} else if (item) {
		filter = images(item->text());
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
