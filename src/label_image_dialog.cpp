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

#include "label_image_dialog.h"

#include "label_manager.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------

LabelImageDialog::LabelImageDialog(const QString& image, LabelManager* manager, QString& filter, QWidget* parent)
	: QDialog(parent),
	m_image(image),
	m_manager(manager),
	m_filter(filter)
{
	setWindowTitle(tr("Label Image"));

	// Setup labels
	m_labels = new QListWidget(this);
	connect(m_labels, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(labelChanged(QListWidgetItem*)));
	QListWidgetItem* item;
	foreach (QString label, m_manager->labels(true)) {
		if (label == tr("All")) {
			continue;
		}

		item = new QListWidgetItem(m_labels);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);

		if (m_manager->images(label).contains(image)) {
			item->setCheckState(Qt::Checked);
		} else {
			item->setCheckState(Qt::Unchecked);
		}

		item->setText(label);
		item->setData(Qt::UserRole, label);
	}
	item = m_labels->item(0);
	if (item) {
		item->setSelected(true);
		m_labels->setCurrentItem(item);
	}

	// Add dialog buttons
	QPushButton* button;
	QVBoxLayout* buttons = new QVBoxLayout;
	buttons->setMargin(0);
	buttons->setSpacing(6);

	button = new QPushButton(tr("Add"), this);
	button->setAutoDefault(false);
	connect(button, SIGNAL(clicked()), this, SLOT(addLabel()));
	buttons->addWidget(button);

	m_remove_button = new QPushButton(tr("Remove"), this);
	m_remove_button->setAutoDefault(false);
	m_remove_button->setEnabled(item != 0);
	connect(m_remove_button, SIGNAL(clicked()), this, SLOT(removeLabel()));
	buttons->addWidget(m_remove_button);

	buttons->addStretch();

	button = new QPushButton(tr("Close"), this);
	button->setDefault(true);
	connect(button, SIGNAL(clicked()), this, SLOT(reject()));
	buttons->addWidget(button);

	// Layout dialog
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setSpacing(12);
	layout->addWidget(m_labels);
	layout->addLayout(buttons);

	// Resize dialog
	resize(QSettings().value("LabelImage/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

void LabelImageDialog::hideEvent(QHideEvent* event)
{
	QSettings().setValue("LabelImage/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void LabelImageDialog::addLabel()
{
	m_remove_button->setEnabled(true);

	// Find first value larger than current untitled labels
	int max_untitled_label = 0;
	QRegExp untitled_label(tr("Untitled Label %1").arg("(\\d+)"));
	QStringList labels = m_manager->labels(true);
	foreach (QString label, labels) {
		if (untitled_label.exactMatch(label)) {
			int id = untitled_label.cap(1).toInt();
			if (id > max_untitled_label) {
				max_untitled_label = id;
			}
		}
	}
	max_untitled_label++;

	// Add new label
	QListWidgetItem* item = new QListWidgetItem;
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
	item->setCheckState(Qt::Checked);
	item->setText(tr("Untitled Label %1").arg(max_untitled_label));
	item->setData(Qt::UserRole, item->text());
	item->setSelected(true);
	m_labels->addItem(item);
	m_labels->setCurrentItem(item);
	m_labels->editItem(item);
	m_manager->addLabel(item->text());
}

//-----------------------------------------------------------------------------

void LabelImageDialog::removeLabel()
{
	QListWidgetItem* item = m_labels->currentItem();
	if (!item) {
		return;
	}

	QString label = item->text();
	if (m_filter == label) {
		m_filter = tr("All");
	}
	m_manager->removeLabel(label);
	delete item;

	m_remove_button->setEnabled(m_labels->count() > 0);
}

//-----------------------------------------------------------------------------

void LabelImageDialog::labelChanged(QListWidgetItem* label)
{
	QString name = label->text();
	QString old_name = label->data(Qt::UserRole).toString();
	if (m_manager->renameLabel(name, old_name)) {
		if (m_filter == old_name) {
			m_filter = name;
		}
		label->setData(Qt::UserRole, name);
	}

	if (label->checkState() == Qt::Checked) {
		m_manager->addImage(m_image, name);
	} else {
		m_manager->removeImage(m_image, name);
	}
}

//-----------------------------------------------------------------------------
