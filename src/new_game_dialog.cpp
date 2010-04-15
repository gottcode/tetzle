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

#include "new_game_dialog.h"

#include "image_dialog.h"
#include "tag_image_dialog.h"
#include "tag_manager.h"
#include "thumbnail_list.h"

#include <QComboBox>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPixmapCache>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QToolTip>
#include <QVBoxLayout>
#include <QXmlStreamReader>

#include <cmath>

//-----------------------------------------------------------------------------

namespace {

QString hash(const QString& path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return QString();
	}
	return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha1).toHex();
}

}

//-----------------------------------------------------------------------------

NewGameDialog::NewGameDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("New Game"));

	// Add pieces slider
	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setRange(1, 1);
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(pieceCountChanged(int)));

	m_count = new QLabel(this);
	m_count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_count->setMinimumWidth(m_count->fontMetrics().width("9,999"));

	// Add image buttons
	QDialogButtonBox* image_buttons = new QDialogButtonBox(this);

	QPushButton* add_button = new QPushButton(tr("Add"), this);
	add_button->setAutoDefault(false);
	connect(add_button, SIGNAL(clicked()), this, SLOT(addImage()));
	image_buttons->addButton(add_button, QDialogButtonBox::ActionRole);

	m_remove_button = new QPushButton(tr("Remove"), this);
	m_remove_button->setAutoDefault(false);
	connect(m_remove_button, SIGNAL(clicked()), this, SLOT(removeImage()));
	image_buttons->addButton(m_remove_button, QDialogButtonBox::ActionRole);

	m_tag_button = new QPushButton(tr("Tag"), this);
	m_tag_button->setAutoDefault(false);
	connect(m_tag_button, SIGNAL(clicked()), this, SLOT(changeTags()));
	image_buttons->addButton(m_tag_button, QDialogButtonBox::ActionRole);

	// Setup thumbnail list
	m_thumbnails = new ThumbnailList(this);
	m_image_tags = new TagManager(this);

	// Add image selector
	m_images_filter = new QComboBox(this);
	m_images_filter->addItems(m_image_tags->tags());
	connect(m_images_filter, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(filterImages(const QString&)));

	m_images = new QListWidget(this);
	m_images->setViewMode(QListView::IconMode);
	m_images->setIconSize(QSize(100, 100));
	m_images->setMinimumSize(348 + m_images->verticalScrollBar()->sizeHint().width(), 224);
	m_images->setMovement(QListView::Static);
	m_images->setResizeMode(QListView::Adjust);
	m_images->setSpacing(6);
	m_images->setUniformItemSizes(true);
	connect(m_images, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(imageSelected(QListWidgetItem*)));
	QListWidgetItem* item;
	foreach (QString image, QDir("images/", "*.*").entryList(QDir::Files, QDir::Name | QDir::LocaleAware)) {
		item = new QListWidgetItem(m_images);
		item->setData(Qt::UserRole, image);
		m_thumbnails->addItem(item, "images/" + image, "images/thumbnails/" + image.section(".", 0, 0) + ".png");
	}

	// Load values
	QSettings settings;
	item = m_images->item(0);
	QString image = settings.value("NewGame/Image").toString();
	if (!image.isEmpty()) {
		for (int i = m_images->count() - 1; i >= 0; --i) {
			item = m_images->item(i);
			if (item->data(Qt::UserRole) == image) {
				break;
			}
		}
	}
	m_images->setCurrentItem(item);
	m_slider->setValue(settings.value("NewGame/Pieces", 2).toInt());
	pieceCountChanged(m_slider->value());
	int index = m_images_filter->findText(settings.value("NewGame/Filter", tr("All Tags")).toString());
	if (index == -1) {
		index = 0;
	}
	m_images_filter->setCurrentIndex(index);

	// Add dialog buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	m_accept_button = buttons->addButton(QDialogButtonBox::Ok);
	m_accept_button->setDefault(true);

	QPushButton* cancel_button = buttons->addButton(QDialogButtonBox::Cancel);
	cancel_button->setAutoDefault(false);

	// Arrange dialog
	QHBoxLayout* slider_layout = new QHBoxLayout;
	slider_layout->addWidget(m_count);
	slider_layout->addWidget(m_slider);

	QGridLayout* layout = new QGridLayout(this);
	layout->setColumnMinimumWidth(0, 6);
	layout->setRowMinimumHeight(4, 12);
	layout->setRowMinimumHeight(7, 18);
	layout->addWidget(new QLabel(tr("<b>Image</b>"), this), 0, 0, 1, 2);
	layout->addWidget(m_images_filter, 1, 1);
	layout->addWidget(m_images, 2, 1);
	layout->addWidget(image_buttons, 3, 1);
	layout->addWidget(new QLabel(tr("<b>Pieces</b>"), this), 5, 0, 1, 2);
	layout->addLayout(slider_layout, 6, 1);
	layout->addWidget(buttons, 8, 1);

	// Disable buttons if there are no images
	bool enabled = m_images->count() > 0;
	m_accept_button->setEnabled(enabled);
	m_tag_button->setEnabled(enabled);
	m_remove_button->setEnabled(enabled && QSettings().value("OpenGame/Image").toString() != image);

	// Resize dialog
	resize(QSettings().value("NewGame/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

void NewGameDialog::accept()
{
	QDialog::accept();

	QListWidgetItem* item = m_images->currentItem();
	if (!item) {
		return;
	}

	QString image = item->data(Qt::UserRole).toString();

	QSettings settings;
	settings.setValue("NewGame/Pieces", m_slider->value());
	settings.setValue("NewGame/Image", image);

	emit newGame(image, m_slider->value());
}

//-----------------------------------------------------------------------------

void NewGameDialog::hideEvent(QHideEvent* event)
{
	QSettings().setValue("NewGame/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void NewGameDialog::addImage()
{
	ImageDialog dialog(this);
	dialog.setMultipleSelections(true);
	dialog.setPath(QSettings().value("AddImage/Path").toString());
	if (dialog.exec() == QDialog::Rejected) {
		return;
	}
	QSettings().setValue("AddImage/Path", QFileInfo(dialog.selectedFile()).absolutePath());

	foreach (QString image, dialog.selectedFiles()) {
		addImage(image);
	}

	m_accept_button->setEnabled(m_images->count() > 0);
}

//-----------------------------------------------------------------------------

void NewGameDialog::removeImage()
{
	QListWidgetItem* item = m_images->currentItem();
	if (!item) {
		return;
	}
	QString current_image = item->data(Qt::UserRole).toString();

	QList<QString> games;

	QXmlStreamReader xml;
	QXmlStreamAttributes attributes;
	foreach (QString game, QDir("saves/", "*.xml").entryList(QDir::Files)) {
		QFile file("saves/" + game);
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		xml.setDevice(&file);

		// Load details
		while (!xml.isStartElement()) {
			xml.readNext();
		}
		attributes = xml.attributes();
		if (xml.name() == QLatin1String("tetzle") && attributes.value("version").toString().toUInt() == 4) {
			if (attributes.value("image").toString() == current_image) {
				games.append(game);
			}
		}
	}

	QString message;
	if (games.isEmpty()) {
		message = tr("Remove selected image?");
	} else {
		message = tr("Remove selected image?\n\nThere are saved games using this image that will also be removed.");
	}
	if (QMessageBox::question(this, tr("Remove Image"), message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		QString image_id = current_image.section(".", 0, 0);
		QFile::remove("images/" + current_image);
		QFile::remove(QString("images/thumbnails/%1.png").arg(image_id));
		foreach (QString game, games) {
			QFile::remove("saves/" + game);
		}
		delete item;
		m_image_tags->removeImage(current_image);
		m_accept_button->setEnabled(m_images->count() > 0);
		if (!m_accept_button->isEnabled()) {
			m_slider->setMaximum(-1);
			m_count->clear();
			QSettings settings;
			settings.remove("NewGame/Image");
			settings.remove("NewGame/Pieces");
		}
	}
}

//-----------------------------------------------------------------------------

void NewGameDialog::changeTags()
{
	QListWidgetItem* item = m_images->currentItem();
	if (!item) {
		return;
	}

	QString filter = m_images_filter->currentText();
	TagImageDialog dialog(item->data(Qt::UserRole).toString(), m_image_tags, filter, this);
	dialog.exec();

	m_images_filter->clear();
	m_images_filter->addItems(m_image_tags->tags());
	int index = m_images_filter->findText(filter);
	if (index == -1) {
		index = 0;
	}
	m_images_filter->setCurrentIndex(index);
}

//-----------------------------------------------------------------------------

void NewGameDialog::imageSelected(QListWidgetItem* item)
{
	bool enabled = item != 0;
	if (!enabled) {
		return;
	}

	QString image = item->data(Qt::UserRole).toString();
	m_tag_button->setEnabled(enabled);
	m_remove_button->setEnabled(enabled && QSettings().value("OpenGame/Image").toString() != image);

	m_image_size = QImageReader("images/" + image).size();
	if (m_image_size.width() > m_image_size.height()) {
		m_ratio = static_cast<float>(m_image_size.height()) / static_cast<float>(m_image_size.width());
	} else {
		m_ratio = static_cast<float>(m_image_size.width()) / static_cast<float>(m_image_size.height());
	}

	int max = qRound(std::sqrt(250.0f / m_ratio));
	int min = qRound(std::sqrt(2.5f / m_ratio));
	int value = min;
	if (m_images->count() > 1) {
		value = qRound(static_cast<float>(max * m_slider->value()) / static_cast<float>(m_slider->maximum()));
	}
	m_slider->setRange(min, max);
	m_slider->setValue(value);
	pieceCountChanged(m_slider->value());
}

//-----------------------------------------------------------------------------

void NewGameDialog::pieceCountChanged(int value)
{
	if (m_image_size.isValid()) {
		int side1 = 4 * value;
		int side2 = qMax(qRound(side1 * m_ratio), 1);
		m_count->setText(QString("%L1").arg(side1 * side2 / 4));
	}
}

//-----------------------------------------------------------------------------

void NewGameDialog::filterImages(const QString& filter)
{
	QSettings().setValue("NewGame/Filter", filter);

	QStringList images = m_image_tags->images(filter);

	QListWidgetItem* item;
	int count = m_images->count();
	for (int i = 0; i < count; ++i) {
		item = m_images->item(i);
		item->setHidden(!images.contains(item->data(Qt::UserRole).toString()));
	}

	item = m_images->currentItem();
	if (!item || item->isHidden()) {
		int count = m_images->count();
		for (int i = 0; i < count; ++i) {
			item = m_images->item(i);
			if (!item->isHidden()) {
				item->setSelected(true);
				m_images->setCurrentItem(item);
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------

void NewGameDialog::addImage(const QString& image)
{
	// Check for duplicates
	QString image_hash = hash(image);
	foreach (QString file, QDir("images/", "*.*").entryList(QDir::Files)) {
		QString file_hash = hash("images/" + file);
		if (image_hash == file_hash) {
			QMessageBox::warning(this, tr("Error"), tr("The image '%1' is already in the program.").arg(image));
			return;
		}
	}

	// Store image
	int image_id = 0;
	int id = 0;
	foreach (QString file, QDir("images/", "*.*").entryList(QDir::Files)) {
		id = file.section(".", 0, 0).toInt();
		if (id > image_id) {
			image_id = id;
		}
	}
	image_id++;
	QString image_file = QString("%1.%2").arg(image_id).arg(QFileInfo(image).suffix().toLower());
	QFile file(image);
	file.copy("images/" + image_file);

	// Show in list of images
	QListWidgetItem* item = new QListWidgetItem(m_images);
	item->setData(Qt::UserRole, image_file);
	m_thumbnails->addItem(item, "images/" + image_file, QString("images/thumbnails/%1.png").arg(image_id));
	m_images->setCurrentItem(item);
	m_images->scrollToItem(item, QAbstractItemView::PositionAtTop);
}

//-----------------------------------------------------------------------------
