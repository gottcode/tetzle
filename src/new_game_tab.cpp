/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "new_game_tab.h"

#include "add_image.h"
#include "image_properties_dialog.h"
#include "path.h"
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
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSlider>
#include <QSplitter>
#include <QXmlStreamReader>

#include <algorithm>
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
	connect(m_image_tags, &TagManager::filterChanged, this, &NewGameTab::filterImages);
	connect(m_image_tags, &TagManager::tagsChanged, this, &NewGameTab::updateTagsStrings);

	// Add image selector
	m_images = new ToolBarList(this);
	m_images->setViewMode(QListView::IconMode);
	m_images->setSelectionMode(QListWidget::ExtendedSelection);
	m_images->setIconSize(QSize(74, 74));
	m_images->setMinimumSize(460 + m_images->verticalScrollBar()->sizeHint().width(), 230);
	m_images->setItemDelegate(new ThumbnailDelegate(m_images));
	connect(m_images, &ToolBarList::itemSelectionChanged, this, &NewGameTab::imageSelected);
	connect(m_images, &ToolBarList::itemActivated, this, &NewGameTab::editImageProperties);

	// Add image actions
	QAction* add_action = new QAction(QIcon::fromTheme("list-add"), tr("Add Image"), this);
	m_images->addToolBarAction(add_action);
	connect(add_action, &QAction::triggered, this, &NewGameTab::addImageClicked);

	m_remove_action = new QAction(QIcon::fromTheme("list-remove"), tr("Remove Image"), this);
	m_images->addToolBarAction(m_remove_action);
	connect(m_remove_action, &QAction::triggered, this, &NewGameTab::removeImage);

	m_tag_action = new QAction(QIcon::fromTheme("document-properties"), tr("Image Properties"), this);
	m_images->addToolBarAction(m_tag_action);
	connect(m_tag_action, &QAction::triggered, this, &NewGameTab::editImageProperties);

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
	connect(m_slider, &QSlider::valueChanged, this, &NewGameTab::pieceCountChanged);

	m_count = new QLabel(this);
	m_count->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_count->setMinimumWidth(m_count->fontMetrics().boundingRect(tr("%L1 pieces").arg(9999)).width());

	// Add buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &NewGameTab::accept);
	connect(buttons, &QDialogButtonBox::rejected, parent, &QDialog::reject);

	m_accept_button = buttons->button(QDialogButtonBox::Ok);
	m_accept_button->setEnabled(false);

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
	const QStringList images = QDir(Path::images(), "*.*").entryList(QDir::Files, QDir::Time | QDir::Reversed);
	for (const QString& image : images) {
		createItem(image, details);
	}
	m_images->sortItems();

	// Load values
	QSettings settings;
	QListWidgetItem* item = m_images->item(0);
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

	m_images->clearSelection();

	QProgressDialog progress(tr("Copying images..."), tr("Cancel"), 0, count, this);
	progress.setMinimumDuration(500);
	progress.setWindowModality(Qt::WindowModal);

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QCoreApplication::processEvents();

	m_image_tags->clearFilter();
	for (int i = 0; i < count; i++) {
		progress.setValue(i);
		if (progress.wasCanceled()) {
			break;
		}

		addImage(images.at(i));

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

	Q_EMIT newGame(image, m_slider->value());
}

//-----------------------------------------------------------------------------

void NewGameTab::addImageClicked()
{
	addImages(AddImage::getOpenFileNames(this));
}

//-----------------------------------------------------------------------------

void NewGameTab::removeImage()
{
	// Find selected images
	const QList<QListWidgetItem*> items = m_images->selectedItems();
	if (items.isEmpty()) {
		return;
	}
	QStringList selected_images;
	for (QListWidgetItem* item : items) {
		selected_images.append(item->data(ImageRole).toString());
	}

	// Find games that would be affected
	QStringList games;

	QXmlStreamReader xml;
	QXmlStreamAttributes attributes;
	const QStringList files = QDir(Path::saves(), "*.xml").entryList(QDir::Files);
	for (const QString& game : files) {
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
			if (selected_images.contains(attributes.value("image").toString())) {
				games.append(game);
			}
		}
	}

	// Prompt about image removal
	const int plural = selected_images.size();
	QMessageBox mbox(this);
	mbox.setIcon(QMessageBox::Question);
	mbox.setWindowTitle(tr("Remove Image"));
	mbox.setText(tr("Remove %n selected image(s)?", nullptr, plural));
	if (!games.isEmpty()) {
		mbox.setIcon(QMessageBox::Warning);
		mbox.setInformativeText(tr("Saved games using these image(s) will be deleted.", nullptr, plural));
	}
	mbox.setStandardButtons(QMessageBox::Cancel);
	mbox.addButton(tr("Remove"), QMessageBox::AcceptRole);
	mbox.setDefaultButton(QMessageBox::Cancel);
	if (mbox.exec() == QMessageBox::Cancel) {
		return;
	}

	// Remove images
	QSettings details(Path::image("details"), QSettings::IniFormat);
	for (const QString& current_image : std::as_const(selected_images)) {
		// Remove from image details
		details.remove(current_image);

		// Delete image
		QFile::remove(Path::image(current_image));

		// Delete thumbnail
		removeThumbnail(current_image.section(".", 0, 0));

		// Remove from tags
		m_image_tags->removeImage(current_image);
	}

	// Remove saved games depending on images
	for (const QString& game : std::as_const(games)) {
		QFile::remove(Path::save(game));
	}

	// Remove listwidget items
	for (QListWidgetItem* item : items) {
		delete item;
	}

	// Handle all images being removed
	if (m_images->count() == 0) {
		m_accept_button->setEnabled(false);

		m_slider->setMaximum(-1);
		m_count->clear();

		QSettings settings;
		settings.remove("NewGame/Image");
		settings.remove("NewGame/Pieces");
	}
}

//-----------------------------------------------------------------------------

void NewGameTab::editImageProperties()
{
	QListWidgetItem* item = m_images->currentItem();
	if (!item || item->isHidden()) {
		return;
	}

	QString filename = item->data(ImageRole).toString();
	ImagePropertiesDialog dialog(item->icon(), item->text(), m_image_tags, filename, window());
	if (dialog.exec() == QDialog::Accepted) {
		// Update name
		item->setText(dialog.name());
		if (item->text() != item->data(NameRole).toString()) {
			item->setData(NameRole, item->text());

			QSettings details(Path::image("details"), QSettings::IniFormat);
			details.setValue(filename + "/Name", item->text());
			Q_EMIT imageRenamed(filename, item->text());

			m_images->sortItems();
			m_images->scrollToItem(item);
		}

		// Update tags
		item->setData(TagsRole, m_image_tags->tags(item->data(ImageRole).toString()));
		updateToolTip(item);

	}
}

//-----------------------------------------------------------------------------

void NewGameTab::imageSelected()
{
	const qsizetype size = m_images->selectedItems().size();
	if (size == 0) {
		m_accept_button->setEnabled(false);
		m_tag_action->setEnabled(false);
		m_remove_action->setEnabled(false);
		return;
	}

	const bool enabled = size == 1;
	m_accept_button->setEnabled(enabled);
	m_tag_action->setEnabled(enabled);

	// Prevent removing the image of the game currently open
	const QString image = m_images->currentItem()->data(ImageRole).toString();
	m_remove_action->setEnabled(QSettings().value("OpenGame/Image").toString() != image);

	m_image_size = QImageReader(Path::image(image)).size();
	if (m_image_size.width() > m_image_size.height()) {
		m_ratio = static_cast<float>(m_image_size.height()) / static_cast<float>(m_image_size.width());
	} else {
		m_ratio = static_cast<float>(m_image_size.width()) / static_cast<float>(m_image_size.height());
	}

	int max = std::lround(std::sqrt(250.0f / m_ratio));
	int min = std::lround(std::sqrt(2.5f / m_ratio));
	int value = min;
	if (m_images->count() > 1) {
		value = std::lround(static_cast<float>(max * m_slider->value()) / static_cast<float>(m_slider->maximum()));
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
		int side2 = std::max(std::lround(side1 * m_ratio), 1L);
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

void NewGameTab::updateTagsStrings()
{
	QListWidgetItem* item;
	int count = m_images->count();
	for (int i = 0; i < count; ++i) {
		item = m_images->item(i);
		item->setData(TagsRole, m_image_tags->tags(item->data(ImageRole).toString()));
		updateToolTip(item);
	}
}

//-----------------------------------------------------------------------------

void NewGameTab::addImage(const QString& image)
{
	QImageReader reader(image);
	if (!reader.canRead()) {
		return;
	}

	// Find image ID
	QString filename;
	int image_id = 0;
	QString image_hash = hash(image);

	QSettings details(Path::image("details"), QSettings::IniFormat);
	const QStringList images = QDir(Path::images(), "*.*").entryList(QDir::Files);
	for (const QString& file : images) {
		image_id = std::max(image_id, file.section(".", 0, 0).toInt());

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

	QListWidgetItem* item = nullptr;
	if (filename.isEmpty()) {
		// Find filename
		QFileInfo info(image);
		filename = QString("%1.%2").arg(image_id).arg(info.suffix().toLower());
		details.setValue(filename + "/SHA1", image_hash);
		details.setValue(filename + "/Name", info.completeBaseName());
		m_image_tags->addImage(filename);

		// Copy and rotate image
		reader.setAutoTransform(true);
		if (reader.transformation() == QImageIOHandler::TransformationNone) {
			QFile::copy(image, Path::image(filename));
		} else {
			QImage image = reader.read();
			image.save(Path::image(filename), "", 100);
		}

		// Remove old thumbnail if it exists
		removeThumbnail(QString::number(image_id));
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
		m_images->setCurrentItem(item);
	}
	m_images->setCurrentItem(item);
	m_images->scrollToItem(item, QAbstractItemView::PositionAtTop);
}

//-----------------------------------------------------------------------------

QListWidgetItem* NewGameTab::createItem(const QString& image, const QSettings& details)
{
	const qreal pixelratio = devicePixelRatioF();
	QListWidgetItem* item = ThumbnailLoader::createItem(Path::image(image), details.value(image + "/Name", tr("Untitled")).toString(), m_images, pixelratio);
	item->setData(ImageRole, image);
	item->setData(NameRole, item->text());
	item->setData(TagsRole, m_image_tags->tags(image));
	updateToolTip(item);
	return item;
}

//-----------------------------------------------------------------------------

void NewGameTab::removeThumbnail(const QString& image_id)
{
	QDir dir(Path::thumbnails());
	const QStringList thumbnails = dir.entryList({ image_id + ".*", image_id + "@*" }, QDir::Files);
	for (const QString& file : thumbnails) {
		dir.remove(file);
	}
}

//-----------------------------------------------------------------------------
