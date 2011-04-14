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

#include "tag_image_dialog.h"

#include "tag_manager.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

TagImageDialog::TagImageDialog(const QString& image, TagManager* manager, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	m_image(image),
	m_manager(manager)
{
	setWindowTitle(tr("Tag Image"));

	// Add tags
	m_tags = new QListWidget(this);
	m_tags->setSortingEnabled(true);
	QStringList tags = m_manager->tags(true);
	foreach (const QString& tag, tags) {
		if (tag == tr("All Tags")) {
			continue;
		}
		QListWidgetItem* item = new QListWidgetItem(tag, m_tags);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
		item->setCheckState(m_manager->images(tag).contains(image) ? Qt::Checked : Qt::Unchecked);
	}
	if (m_tags->count() > 0) {
		QListWidgetItem* item = m_tags->item(0);
		item->setSelected(true);
		m_tags->setCurrentItem(item);
	}

	// Add dialog buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	// Layout dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_tags);
	layout->addWidget(buttons);

	// Resize dialog
	resize(QSettings().value("TagImage/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

void TagImageDialog::accept()
{
	QStringList tags;
	int count = m_tags->count();
	for (int i = 0; i < count; ++i) {
		if (m_tags->item(i)->checkState() == Qt::Checked) {
			tags.append(m_tags->item(i)->text());
		}
	}
	m_manager->setImageTags(m_image, tags);
	QDialog::accept();
}

//-----------------------------------------------------------------------------

void TagImageDialog::hideEvent(QHideEvent* event)
{
	QSettings().setValue("TagImage/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------
