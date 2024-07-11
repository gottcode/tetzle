/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "image_properties_dialog.h"

#include "tag_manager.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSettings>
#include <QToolButton>

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

	m_add_tag_name = new QLineEdit(this);
	m_add_tag_name->setPlaceholderText(tr("New tag name"));
	connect(m_add_tag_name, &QLineEdit::textChanged, this, [this](const QString& tag) {
		m_add_tag_button->setEnabled(!m_manager->hasTag(tag.trimmed()));
	});
	m_add_tag_name->installEventFilter(this);

	m_add_tag_button = new QToolButton(this);
	m_add_tag_button->setEnabled(false);
	m_add_tag_button->setIcon(QIcon::fromTheme("list-add"));
	m_add_tag_button->setText(TagManager::tr("Add Tag"));
	m_add_tag_button->setToolTip(TagManager::tr("Add Tag"));
	connect(m_add_tag_button, &QToolButton::clicked, this, &ImagePropertiesDialog::addTag);

	QGridLayout* tags_layout = new QGridLayout;
	tags_layout->setContentsMargins(0, 0, 0, 0);
	tags_layout->setColumnStretch(0, 1);
	tags_layout->setRowStretch(0, 1);
	tags_layout->addWidget(m_tags, 0, 0, 1, 2);
	tags_layout->addWidget(m_add_tag_name, 1, 0);
	tags_layout->addWidget(m_add_tag_button, 1, 1);

	// Add dialog buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &ImagePropertiesDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &ImagePropertiesDialog::reject);

	// Layout dialog
	QFormLayout* layout = new QFormLayout(this);
	layout->addRow(preview);
	layout->addRow(tr("Name:"), m_name);
	layout->addItem(new QSpacerItem(6, 6));
	layout->addRow(tr("Tags:"), tags_layout);
	layout->addItem(new QSpacerItem(6, 6));
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

bool ImagePropertiesDialog::eventFilter(QObject* watched, QEvent* event)
{
	if ((watched == m_add_tag_name) && (event->type() == QEvent::KeyPress)) {
		QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
		if ((key_event->key() == Qt::Key_Return) || (key_event->key() == Qt::Key_Enter)) {
			addTag();
			return true;
		}
	}

	return QDialog::eventFilter(watched, event);
}

//-----------------------------------------------------------------------------

void ImagePropertiesDialog::accept()
{
	QStringList tags;
	for (int i = 0, count = m_tags->count(); i < count; ++i) {
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

void ImagePropertiesDialog::addTag()
{
	// Fetch new tag name
	const QString tag = m_add_tag_name->text().trimmed();
	if (m_manager->hasTag(tag)) {
		return;
	}
	m_add_tag_name->clear();

	// Create new tag item
	QListWidgetItem* item = new QListWidgetItem(tag, m_tags);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
	item->setCheckState(Qt::Checked);
	m_tags->sortItems();
	m_tags->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------
