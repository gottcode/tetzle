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

#include "tag_image_dialog.h"

#include "tag_manager.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

TagImageDialog::TagImageDialog(const QString& image, TagManager* manager, QString& filter, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	m_image(image),
	m_manager(manager),
	m_filter(filter)
{
	setWindowTitle(tr("Tag Image"));

	// Setup tags
	m_tags = new QListWidget(this);
	connect(m_tags, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(tagChanged(QListWidgetItem*)));
	QListWidgetItem* item;
	foreach (QString tag, m_manager->tags(true)) {
		if (tag == tr("All Tags")) {
			continue;
		}

		item = new QListWidgetItem(m_tags);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);

		if (m_manager->images(tag).contains(image)) {
			item->setCheckState(Qt::Checked);
		} else {
			item->setCheckState(Qt::Unchecked);
		}

		item->setText(tag);
		item->setData(Qt::UserRole, tag);
	}
	item = m_tags->item(0);
	if (item) {
		item->setSelected(true);
		m_tags->setCurrentItem(item);
	}

	// Add dialog buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(Qt::Horizontal, this);
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QPushButton* button = buttons->addButton(tr("New"), QDialogButtonBox::ActionRole);
	button->setAutoDefault(false);
	connect(button, SIGNAL(clicked()), this, SLOT(addTag()));

	m_remove_button = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	m_remove_button->setAutoDefault(false);
	m_remove_button->setEnabled(item != 0);
	connect(m_remove_button, SIGNAL(clicked()), this, SLOT(removeTag()));

	button = buttons->addButton(QDialogButtonBox::Close);
	button->setDefault(true);

	// Layout dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_tags);
	layout->addWidget(buttons);

	// Resize dialog
	resize(QSettings().value("TagImage/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

void TagImageDialog::hideEvent(QHideEvent* event)
{
	QSettings().setValue("TagImage/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void TagImageDialog::addTag()
{
	m_remove_button->setEnabled(true);

	// Find first value larger than current untitled tags
	int max_untitled_tag = 0;
	QRegExp untitled_tag(tr("Untitled Tag %1").arg("(\\d+)"));
	QStringList tags = m_manager->tags(true);
	foreach (QString tag, tags) {
		if (untitled_tag.exactMatch(tag)) {
			max_untitled_tag = qMax(max_untitled_tag, untitled_tag.cap(1).toInt());
		}
	}
	max_untitled_tag++;

	// Add new tag
	QListWidgetItem* item = new QListWidgetItem;
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
	item->setCheckState(Qt::Checked);
	item->setText(tr("Untitled Tag %1").arg(max_untitled_tag));
	item->setData(Qt::UserRole, item->text());
	item->setSelected(true);
	m_tags->addItem(item);
	m_tags->setCurrentItem(item);
	m_tags->editItem(item);
	m_manager->addTag(item->text());
}

//-----------------------------------------------------------------------------

void TagImageDialog::removeTag()
{
	QListWidgetItem* item = m_tags->currentItem();
	if (!item) {
		return;
	}

	if (QMessageBox::question(this, tr("Question"), tr("Remove selected tag?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
		return;
	}

	QString tag = item->text();
	if (m_filter == tag) {
		m_filter = tr("All Tags");
	}
	m_manager->removeTag(tag);
	delete item;

	m_remove_button->setEnabled(m_tags->count() > 0);
}

//-----------------------------------------------------------------------------

void TagImageDialog::tagChanged(QListWidgetItem* tag)
{
	QString name = tag->text();
	QString old_name = tag->data(Qt::UserRole).toString();
	if (m_manager->renameTag(name, old_name)) {
		if (m_filter == old_name) {
			m_filter = name;
		}
		tag->setData(Qt::UserRole, name);
	}

	if (tag->checkState() == Qt::Checked) {
		m_manager->addImage(m_image, name);
	} else {
		m_manager->removeImage(m_image, name);
	}
}

//-----------------------------------------------------------------------------
