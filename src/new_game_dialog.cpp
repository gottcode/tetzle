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

#include "add_image.h"
#include "path.h"
#include "tag_image_dialog.h"
#include "tag_manager.h"
#include "thumbnail_list.h"

#include <QApplication>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
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
	return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5).toHex();
}

}

//-----------------------------------------------------------------------------

NewGameDialog::NewGameDialog(const QStringList& files, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setWindowTitle(tr("New Game"));
	setAcceptDrops(true);

	// Create image widgets
	QGroupBox* image_box = new QGroupBox(tr("Image"), this);

	// Set up thumbnail list
	m_image_tags = new TagManager(this);

	// Add image selector
	m_images = new ThumbnailList(image_box);
	m_images->setViewMode(QListView::IconMode);
	m_images->setGridSize(QSize(112, 112));
	m_images->setMinimumSize(460 + m_images->verticalScrollBar()->sizeHint().width(), 230);
	m_images->setMovement(QListView::Static);
	m_images->setResizeMode(QListView::Adjust);
	m_images->setUniformItemSizes(true);
	connect(m_images, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(imageSelected(QListWidgetItem*)));

	// Add image filter
	m_images_filter = new QComboBox(image_box);
	m_images_filter->addItems(m_image_tags->tags());
	connect(m_images_filter, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(filterImages(const QString&)));

	// Add image buttons
	QPushButton* add_button = new QPushButton(tr("Add"), image_box);
	add_button->setAutoDefault(false);
	connect(add_button, SIGNAL(clicked()), this, SLOT(addImage()));

	m_remove_button = new QPushButton(tr("Remove"), image_box);
	m_remove_button->setAutoDefault(false);
	m_remove_button->setEnabled(false);
	connect(m_remove_button, SIGNAL(clicked()), this, SLOT(removeImage()));

	m_tag_button = new QPushButton(tr("Tag"), image_box);
	m_tag_button->setAutoDefault(false);
	m_tag_button->setEnabled(false);
	connect(m_tag_button, SIGNAL(clicked()), this, SLOT(changeTags()));

	// Create pieces widgets
	QGroupBox* pieces_box = new QGroupBox(tr("Pieces"), this);

	// Add pieces slider
	m_slider = new QSlider(Qt::Horizontal, pieces_box);
	m_slider->setRange(1, 1);
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(pieceCountChanged(int)));

	m_count = new QLabel(pieces_box);
	m_count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_count->setMinimumWidth(m_count->fontMetrics().width("9,999"));

	// Add dialog buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	m_accept_button = buttons->addButton(QDialogButtonBox::Ok);
	m_accept_button->setDefault(true);
	m_accept_button->setEnabled(false);

	QPushButton* cancel_button = buttons->addButton(QDialogButtonBox::Cancel);
	cancel_button->setAutoDefault(false);

	// Arrange dialog
	QHBoxLayout* image_actions = new QHBoxLayout;
	image_actions->setMargin(0);
	image_actions->addWidget(m_images_filter);
	image_actions->addStretch();
	image_actions->addWidget(add_button);
	image_actions->addWidget(m_remove_button);
	image_actions->addWidget(m_tag_button);

	QVBoxLayout* image_layout = new QVBoxLayout(image_box);
	image_layout->addWidget(m_images);
	image_layout->addLayout(image_actions);

	QHBoxLayout* pieces_layout = new QHBoxLayout(pieces_box);
	pieces_layout->addWidget(m_count);
	pieces_layout->addWidget(m_slider);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(image_box, 1);
	layout->addWidget(pieces_box);
	layout->addWidget(buttons);

	// Load images
	QListWidgetItem* item;
	foreach (QString image, QDir(Path::images(), "*.*").entryList(QDir::Files, QDir::Time | QDir::Reversed)) {
		item = m_images->addImage(Path::image(image));
		item->setData(Qt::UserRole, image);
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

	// Add new images
	addImages(files);

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

void NewGameDialog::dragEnterEvent(QDragEnterEvent* event)
{
	AddImage::dragEnterEvent(event);
}

//-----------------------------------------------------------------------------

void NewGameDialog::dropEvent(QDropEvent* event)
{
	addImages(AddImage::dropEvent(event));
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
	addImages(AddImage::getOpenFileNames(this));
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
	foreach (QString game, QDir(Path::saves(), "*.xml").entryList(QDir::Files)) {
		QFile file(Path::save(game));
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		xml.setDevice(&file);

		// Load details
		while (!xml.isStartElement()) {
			xml.readNext();
		}
		attributes = xml.attributes();
		if (xml.name() == QLatin1String("tetzle") && attributes.value("version").toString().toUInt() == 5) {
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
		QFile::remove(Path::image(current_image));
		QFile::remove(Path::thumbnail(image_id));
		foreach (QString game, games) {
			QFile::remove(Path::save(game));
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
	m_accept_button->setEnabled(enabled);
	m_tag_button->setEnabled(enabled);
	m_remove_button->setEnabled(enabled);
	if (!enabled) {
		return;
	}

	QString image = item->data(Qt::UserRole).toString();
	m_remove_button->setEnabled(QSettings().value("OpenGame/Image").toString() != image);

	m_image_size = QImageReader(Path::image(image)).size();
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
	QString filename = hash(image) + "." + QFileInfo(image).suffix().toLower();

	QListWidgetItem* item = 0;
	if (!QDir(Path::images()).exists(filename)) {
		// Copy and rotate image
		QFile::copy(image, Path::image(filename));
		QProcess rotate;
		rotate.start(QString("jhead -autorot \"%1\"").arg(Path::image(filename)));
		rotate.waitForFinished(-1);

		// Add to list of images
		item = m_images->addImage(Path::image(filename));
		item->setData(Qt::UserRole, filename);
	} else {
		// Find in list of images
		for (int i = 0; i < m_images->count(); ++i) {
			if (m_images->item(i)->data(Qt::UserRole).toString() == filename) {
				item = m_images->item(i);
				break;
			}
		}
	}

	// Select in list of images
	m_images->setCurrentItem(item);
	m_images->scrollToItem(item, QAbstractItemView::PositionAtTop);
}

//-----------------------------------------------------------------------------

void NewGameDialog::addImages(const QStringList& images)
{
	int count = images.count();
	if (count == 0) {
		return;
	}

	QProgressDialog progress(tr("Copying images..."), tr("Cancel"), 0, count, this);
	progress.setMinimumDuration(500);
	progress.setWindowModality(Qt::WindowModal);

	QApplication::setOverrideCursor(Qt::WaitCursor);

	m_images_filter->setCurrentIndex(0);
	for (int i = 0; i < count; i++) {
		progress.setValue(i);
		if (progress.wasCanceled()) {
			break;
		}

		QString image = images.at(i);
		if (QDir::match(AddImage::supportedFormats(), image)) {
			addImage(image);
		}

		QApplication::processEvents();
	}
	progress.setValue(count);

	QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
