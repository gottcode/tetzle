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

#include "new_game_tab.h"

#include "add_image.h"
#include "path.h"
#include "tag_image_dialog.h"
#include "tag_manager.h"
#include "thumbnail_delegate.h"
#include "thumbnail_loader.h"
#include "toolbar_list.h"

#include <QAction>
#include <QApplication>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QImageReader>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QSplitter>
#include <QXmlStreamReader>

#include <cmath>

//-----------------------------------------------------------------------------

namespace
{
	QString hash(const QString& path)
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			return QString();
		}
		return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha1).toHex();
	}

	enum ItemRoles
	{
		TagsRole = Qt::UserRole,
		ImageRole,
		NameRole
	};

	void updateToolTip(QListWidgetItem* item)
	{
		QString tip = item->text();
		QString tags = item->data(TagsRole).toString();
		if (!tags.isEmpty()) {
			tip += "<br><small><i>" + item->data(TagsRole).toString() + "</i></small>";
		}
		item->setToolTip(tip);
	}
}

//-----------------------------------------------------------------------------

NewGameTab::NewGameTab(const QStringList& files, QDialog* parent)
	: QWidget(parent)
{
	// Add image filter
	m_image_tags = new TagManager(this);
	connect(m_image_tags, SIGNAL(filterChanged(const QStringList&)), this, SLOT(filterImages(const QStringList&)));

	// Add image selector
	m_images = new ToolBarList(this);
	m_images->setViewMode(QListView::IconMode);
	m_images->setIconSize(QSize(74, 74));
	m_images->setMinimumSize(460 + m_images->verticalScrollBar()->sizeHint().width(), 230);
	m_images->setItemDelegate(new ThumbnailDelegate(m_images));
	connect(m_images, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(imageSelected(QListWidgetItem*)));

	// Add image actions
	QAction* add_action = new QAction(QIcon::fromTheme("list-add", QPixmap(":/tango/list-add.png")), tr("Add Image"), this);
	m_images->addToolBarAction(add_action);
	connect(add_action, SIGNAL(triggered()), this, SLOT(addImage()));

	m_remove_action = new QAction(QIcon::fromTheme("list-remove", QPixmap(":/tango/list-remove.png")), tr("Remove Image"), this);
	m_images->addToolBarAction(m_remove_action);
	connect(m_remove_action, SIGNAL(triggered()), this, SLOT(removeImage()));

	m_tag_action = new QAction(QIcon::fromTheme("image-x-generic", QPixmap(":/tango/image-x-generic.png")), tr("Tag Image"), this);
	m_images->addToolBarAction(m_tag_action);
	connect(m_tag_action, SIGNAL(triggered()), this, SLOT(changeTags()));

	// Add image splitter
	m_image_contents = new QSplitter(this);
	m_image_contents->addWidget(m_image_tags);
	m_image_contents->addWidget(m_images);
	m_image_contents->setStretchFactor(0, 0);
	m_image_contents->setStretchFactor(1, 1);
	m_image_contents->setSizes(QList<int>() << 130 << m_images->minimumWidth());

	// Add pieces slider
	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setRange(1, 1);
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(pieceCountChanged(int)));

	m_count = new QLabel(this);
	m_count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_count->setMinimumWidth(m_count->fontMetrics().width(tr("%L1 pieces").arg(9999)));

	// Add buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), parent, SLOT(reject()));

	m_accept_button = buttons->addButton(QDialogButtonBox::Ok);
	m_accept_button->setDefault(true);
	m_accept_button->setEnabled(false);

	QPushButton* cancel_button = buttons->addButton(QDialogButtonBox::Cancel);
	cancel_button->setAutoDefault(false);

	// Arrange widgets
	QGridLayout* layout = new QGridLayout(this);
	layout->setColumnStretch(1, 1);
	layout->setRowStretch(0, 1);
	layout->addWidget(m_image_contents, 0, 0, 1, 2);
	layout->setRowMinimumHeight(1, 12);
	layout->addWidget(m_count, 2, 0);
	layout->addWidget(m_slider, 2, 1);
	layout->setRowMinimumHeight(3, 12);
	layout->addWidget(buttons, 4, 0, 1, 2);

	// Load images
	QSettings details(Path::image("details"), QSettings::IniFormat);
	QListWidgetItem* item = 0;
	foreach (QString image, QDir(Path::images(), "*.*").entryList(QDir::Files, QDir::Time | QDir::Reversed)) {
		item = createItem(image, details);
	}
	m_images->sortItems();
	connect(m_images, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(imageChanged(QListWidgetItem*)));

	// Load values
	QSettings settings;
	item = m_images->item(0);
	QString image = settings.value("NewGame/Image").toString();
	if (!image.isEmpty()) {
		for (int i = m_images->count() - 1; i >= 0; --i) {
			item = m_images->item(i);
			if (item->data(ImageRole) == image) {
				break;
			}
		}
	}
	m_images->setCurrentItem(item);
	m_slider->setValue(settings.value("NewGame/Pieces", 2).toInt());
	pieceCountChanged(m_slider->value());

	// Add new images
	addImages(files);

	// Resize contents
	m_image_contents->restoreState(settings.value("NewGame/SplitterSizes").toByteArray());
}

//-----------------------------------------------------------------------------

void NewGameTab::addImages(const QStringList& images)
{
	int count = images.count();
	if (count == 0) {
		return;
	}

	QProgressDialog progress(tr("Copying images..."), tr("Cancel"), 0, count, this);
	progress.setMinimumDuration(500);
	progress.setWindowModality(Qt::WindowModal);

	QApplication::setOverrideCursor(Qt::WaitCursor);

	m_image_tags->clearFilter();
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

void NewGameTab::hideEvent(QHideEvent* event)
{
	QSettings().setValue("NewGame/SplitterSizes", m_image_contents->saveState());
	QWidget::hideEvent(event);
}

//-----------------------------------------------------------------------------

void NewGameTab::accept()
{
	QListWidgetItem* item = m_images->currentItem();
	if (!item) {
		return;
	}

	QString image = item->data(ImageRole).toString();

	QSettings settings;
	settings.setValue("NewGame/Pieces", m_slider->value());
	settings.setValue("NewGame/Image", image);

	emit newGame(image, m_slider->value());
}

//-----------------------------------------------------------------------------

void NewGameTab::addImage()
{
	addImages(AddImage::getOpenFileNames(this));
}

//-----------------------------------------------------------------------------

void NewGameTab::removeImage()
{
	QListWidgetItem* item = m_images->currentItem();
	if (!item) {
		return;
	}
	QString current_image = item->data(ImageRole).toString();

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
		if (xml.name() == QLatin1String("tetzle") && attributes.value("version").toString().toUInt() <= 5) {
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
		m_image_tags->setImageTags(current_image, QStringList());
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

void NewGameTab::changeTags()
{
	QListWidgetItem* item = m_images->currentItem();
	if (item && !item->isHidden()) {
		TagImageDialog dialog(item->data(ImageRole).toString(), m_image_tags, this);
		if (dialog.exec() == QDialog::Accepted) {
			item->setData(TagsRole, m_image_tags->tags(item->data(ImageRole).toString()));
			updateToolTip(item);
		}
	}
}

//-----------------------------------------------------------------------------

void NewGameTab::imageChanged(QListWidgetItem* item)
{
	if (item && item->text() != item->data(NameRole).toString()) {
		updateToolTip(item);

		// Update name
		QString filename = item->data(ImageRole).toString();
		QSettings details(Path::image("details"), QSettings::IniFormat);
		details.setValue(filename + "/Name", item->text());
		emit imageRenamed(filename, item->text());

		// Show item
		m_images->sortItems();
		m_images->scrollToItem(item);
	}
}

//-----------------------------------------------------------------------------

void NewGameTab::imageSelected(QListWidgetItem* item)
{
	bool enabled = item != 0;
	m_accept_button->setEnabled(enabled);
	m_tag_action->setEnabled(enabled);
	m_remove_action->setEnabled(enabled);
	if (!enabled) {
		return;
	}

	QString image = item->data(ImageRole).toString();
	m_remove_action->setEnabled(QSettings().value("OpenGame/Image").toString() != image);

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

void NewGameTab::pieceCountChanged(int value)
{
	if (m_image_size.isValid()) {
		int side1 = 4 * value;
		int side2 = qMax(qRound(side1 * m_ratio), 1);
		m_count->setText(tr("%L1 pieces").arg(side1 * side2 / 4));
	}
}

//-----------------------------------------------------------------------------

void NewGameTab::filterImages(const QStringList& filter)
{
	// Filter items
	QListWidgetItem* item;
	int count = m_images->count();
	for (int i = 0; i < count; ++i) {
		item = m_images->item(i);
		item->setHidden(!filter.contains(item->data(ImageRole).toString()));
	}

	// Select next item if current item was hidden
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

	// Disable tag button if no images are visible
	item = m_images->currentItem();
	m_tag_action->setEnabled(item && !item->isHidden());
}

//-----------------------------------------------------------------------------

void NewGameTab::addImage(const QString& image)
{
	// Find image ID
	QString filename;
	int image_id = 0;
	QString image_hash = hash(image);

	QSettings details(Path::image("details"), QSettings::IniFormat);
	QStringList images = QDir(Path::images(), "*.*").entryList(QDir::Files);
	foreach (QString file, images) {
		image_id = qMax(image_id, file.section(".", 0, 0).toInt());

		QString key = file + "/SHA1";
		if (!details.contains(key)) {
			details.setValue(key, hash(Path::image(file)));
		}
		if (details.value(key) == image_hash) {
			filename = file;
			break;
		}
	}
	image_id++;

	QListWidgetItem* item = 0;
	if (filename.isEmpty()) {
		// Find filename
		QFileInfo info(image);
		filename = QString("%1.%2").arg(image_id).arg(info.suffix().toLower());
		details.setValue(filename + "/SHA1", image_hash);
		details.setValue(filename + "/Name", info.completeBaseName());

		// Copy and rotate image
		QFile::copy(image, Path::image(filename));
		QProcess rotate;
		rotate.start(QString("jhead -autorot \"%1\"").arg(Path::image(filename)));
		rotate.waitForFinished(-1);
	} else {
		// Find in list of images
		for (int i = 0; i < m_images->count(); ++i) {
			if (m_images->item(i)->data(ImageRole).toString() == filename) {
				item = m_images->item(i);
				break;
			}
		}
	}

	// Select item
	if (!item) {
		m_images->blockSignals(true);
		item = createItem(filename, details);
		m_images->blockSignals(false);
		m_images->editItem(item);
	}
	m_images->setCurrentItem(item);
	m_images->scrollToItem(item, QAbstractItemView::PositionAtTop);
}

//-----------------------------------------------------------------------------

QListWidgetItem* NewGameTab::createItem(const QString& image, const QSettings& details)
{
	QListWidgetItem* item = ThumbnailLoader::createItem(Path::image(image), details.value(image + "/Name", tr("Untitled")).toString(), m_images);
	item->setData(ImageRole, image);
	item->setData(NameRole, item->text());
	item->setData(TagsRole, m_image_tags->tags(image));
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	updateToolTip(item);
	return item;
}

//-----------------------------------------------------------------------------
