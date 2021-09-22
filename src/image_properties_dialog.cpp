/*
	SPDX-FileCopyrightText: 2008-2014 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "image_properties_dialog.h"

#include "tag_manager.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>

//-----------------------------------------------------------------------------

ImagePropertiesDialog::ImagePropertiesDialog(const QIcon& icon, const QString& name, TagManager* manager, const QString& image, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
	, m_image(image)
	, m_manager(manager)
{
	setWindowTitle(tr("Image Properties"));

	QLabel* preview = new QLabel(this);
	preview->setAlignment(Qt::AlignCenter);
	preview->setPixmap(icon.pixmap(74,74));
	preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// Add name
	m_name = new QLineEdit(name, this);

	// Add tags
	m_tags = new QListWidget(this);
	m_tags->setSortingEnabled(true);
	const QStringList tags = m_manager->tags();
	for (const QString& tag : tags) {
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
	connect(buttons, &QDialogButtonBox::accepted, this, &ImagePropertiesDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &ImagePropertiesDialog::reject);

	// Layout dialog
	QFormLayout* layout = new QFormLayout(this);
	layout->addRow(preview);
	layout->addRow(tr("Name:"), m_name);
	layout->addRow(tr("Tags:"), m_tags);
	layout->addRow(buttons);

	// Resize dialog
	resize(QSettings().value("ImageProperties/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

QString ImagePropertiesDialog::name() const
{
	return m_name->text();
}

//-----------------------------------------------------------------------------

void ImagePropertiesDialog::accept()
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

void ImagePropertiesDialog::hideEvent(QHideEvent* event)
{
	QSettings().setValue("ImageProperties/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------
