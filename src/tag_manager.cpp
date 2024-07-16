/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "tag_manager.h"

#include "path.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QMessageBox>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

namespace
{

class TagItem : public QListWidgetItem
{
public:
	explicit TagItem(const QString& tag = QString());

	static bool compare(const QString& a, const QString& b);
	bool operator<(const QListWidgetItem& other) const override;
};

TagItem::TagItem(const QString& tag)
	: QListWidgetItem(tag)
{
	setData(Qt::UserRole, tag);
	setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
}

inline bool TagItem::compare(const QString& a, const QString& b)
{
	return a.localeAwareCompare(b) < 0;
}

inline bool TagItem::operator<(const QListWidgetItem& other) const
{
	return compare(text(), other.text());
}

}

//-----------------------------------------------------------------------------

TagManager::TagManager(QWidget* parent)
	: QComboBox(parent)
{
	connect(this, &QComboBox::currentIndexChanged, this, &TagManager::currentTagChanged);

	// Create manage dialog
	m_manage_dialog = new QDialog(this);
	QString title = tr("Manage Tags...");
	title.remove("...");
	m_manage_dialog->setWindowTitle(title);

	QPushButton* add_tag_button = new QPushButton(QIcon::fromTheme("list-add"), tr("Add Tag"), m_manage_dialog);
	connect(add_tag_button, &QPushButton::clicked, this, &TagManager::addTag);

	m_remove_tag_button = new QPushButton(QIcon::fromTheme("list-remove"), tr("Remove Tag"), m_manage_dialog);
	connect(m_remove_tag_button, &QPushButton::clicked, this, &TagManager::removeTag);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, m_manage_dialog);
	buttons->addButton(add_tag_button, QDialogButtonBox::ActionRole);
	buttons->addButton(m_remove_tag_button, QDialogButtonBox::ActionRole);
	connect(buttons, &QDialogButtonBox::rejected, m_manage_dialog, &QDialog::reject);

	// Add filter
	m_tags_list = new QListWidget(m_manage_dialog);
	m_tags_list->setMovement(QListView::Static);
	m_tags_list->setResizeMode(QListView::Adjust);
	m_tags_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_tags_list->setSelectionBehavior(QAbstractItemView::SelectItems);
	m_tags_list->setSelectionMode(QAbstractItemView::SingleSelection);
	connect(m_tags_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* item) {
		m_remove_tag_button->setEnabled(item);
	});
	connect(m_tags_list, &QListWidget::itemChanged, this, &TagManager::tagChanged);

	QVBoxLayout* manage_layout = new QVBoxLayout(m_manage_dialog);
	manage_layout->addWidget(m_tags_list);
	manage_layout->addSpacing(12);
	manage_layout->addWidget(buttons);

	// Add tags
	m_all = QDir(Path::images(), "*.*", QDir::Name | QDir::LocaleAware, QDir::Files).entryList();

	QSettings file(Path::image("tags"), QSettings::IniFormat);
	file.beginGroup("Tags");
	const QStringList tags = file.childKeys();
	QStringList images;
	const QDir folder(Path::images(), "*.*");
	for (const QString& tag : tags) {
		images = file.value(tag).toStringList();
		QMutableStringListIterator i(images);
		while (i.hasNext()) {
			i.next();
			if (!folder.exists(i.value())) {
				i.remove();
			}
		}
		m_tags[tag] = images;

		QListWidgetItem* item = new QListWidgetItem(tag);
		item->setData(Qt::UserRole, item->text());
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		m_tags_list->addItem(item);
	}
	m_tags_list->sortItems();
	m_remove_tag_button->setEnabled(!m_tags.isEmpty());

	updateFilter();
	clearFilter();
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
	std::sort(tags.begin(), tags.end(), &TagItem::compare);
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
	std::sort(tags.begin(), tags.end(), &TagItem::compare);
	return tags.join(", ");
}

//-----------------------------------------------------------------------------

void TagManager::clearFilter()
{
	setCurrentIndex(0);
}

//-----------------------------------------------------------------------------

void TagManager::addImage(const QString& image)
{
	m_all.append(image);
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::removeImage(const QString& image)
{
	setImageTags(image, QStringList());
	m_all.removeOne(image);
	updateFilter();
}

//-----------------------------------------------------------------------------

void TagManager::setImageTags(const QString& image, const QStringList& tags)
{
	// Add new tags
	for (const QString& tag : tags) {
		if (!m_tags.contains(tag)) {
			createTag(tag);
		}
	}

	// Add or remove image from tags
	bool changed = false;
	QMutableHashIterator<QString, QStringList> i(m_tags);
	while (i.hasNext()) {
		i.next();
		const bool tagged = tags.contains(i.key());
		QStringList& tag = i.value();
		if (tag.contains(image)) {
			if (!tagged) {
				changed = true;
				tag.removeOne(image);
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

void TagManager::addTag()
{
	// Find first unused tag
	int new_tag = 0;
	QString tag;
	do {
		tag = tr("Untitled %1").arg(++new_tag);
	} while(m_tags.contains(tag));

	// Add tag
	QListWidgetItem* item = createTag(tag);
	m_tags_list->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
	m_tags_list->editItem(item);
}

//-----------------------------------------------------------------------------

void TagManager::removeTag()
{
	// Find current item
	const QListWidgetItem* item = m_tags_list->currentItem();
	if (!item) {
		return;
	}

	// Ask before removing
	if (QMessageBox::question(window(), tr("Question"), tr("Remove selected tag?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
		return;
	}

	// Remove item
	const QString tag = item->text();
	delete item;
	item = nullptr;

	// Remove tag
	m_tags.remove(tag);
	m_remove_tag_button->setEnabled(!m_tags.isEmpty());
	storeTags();
	updateFilter();
	Q_EMIT tagsChanged();
}

//-----------------------------------------------------------------------------

void TagManager::currentTagChanged(int index)
{
	const QString action = itemData(index).toString();
	QStringList filter;
	if (action == "ALL") {
		filter = m_all;
	} else if (action == "UNTAGGED") {
		filter = m_all;
		QHashIterator<QString, QStringList> i(m_tags);
		while (i.hasNext()) {
			i.next();
			for (const QString& image : i.value()) {
				filter.removeOne(image);
			}
		}
	} else if (action == "MANAGE") {
		clearFilter();
		m_tags_list->setCurrentRow(0);
		m_tags_list->scrollToTop();
		m_manage_dialog->exec();
		return;
	} else {
		filter = images(itemText(index));
	}
	Q_EMIT filterChanged(filter);
}

//-----------------------------------------------------------------------------

void TagManager::tagChanged(QListWidgetItem* item)
{
	if (!item) {
		return;
	}

	const QString tag = item->text();
	const QString old_tag = item->data(Qt::UserRole).toString();
	if (tag != old_tag) {
		m_tags_list->blockSignals(true);
		if (tag.isEmpty()) {
			// Don't allow empty tags
			item->setText(old_tag);
		} else if (m_tags.contains(tag)) {
			// Don't allow duplicate tags
			item->setText(old_tag);
			QMessageBox::warning(window(), tr("Sorry"), tr("A tag with that name already exists."));
		} else {
			// Rename tag
			item->setData(Qt::UserRole, tag);

			m_tags.insert(tag, m_tags.take(old_tag));
			storeTags();

			m_tags_list->sortItems();
			updateFilter();

			Q_EMIT tagsChanged();
		}
		m_tags_list->blockSignals(false);
	}
}

//-----------------------------------------------------------------------------

void TagManager::updateFilter()
{
	blockSignals(true);
	clear();
	addItem(tr("All Images"), "ALL");
	for (int i = 0, count = m_tags_list->count(); i < count; ++i) {
		addItem(m_tags_list->item(i)->text());
	}
	addItem(tr("Untagged"), "UNTAGGED");
	insertSeparator(count());
	addItem(tr("Manage Tags..."), "MANAGE");
	blockSignals(false);
}

//-----------------------------------------------------------------------------

QListWidgetItem* TagManager::createTag(const QString& tag)
{
	m_remove_tag_button->setEnabled(true);

	// Add tag
	m_tags.insert(tag, QStringList());
	storeTags();

	// Add tag item
	QListWidgetItem* item = new TagItem(tag);
	m_tags_list->addItem(item);

	// Update list of tags
	m_tags_list->sortItems();
	updateFilter();

	return item;
}

//-----------------------------------------------------------------------------

void TagManager::storeTags() const
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
