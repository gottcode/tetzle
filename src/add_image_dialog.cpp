/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#include "add_image_dialog.h"

#include "thumbnail_list.h"

#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDirModel>
#include <QHeaderView>
#include <QImageReader>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>

/*****************************************************************************/

static QString previewFileName(const QString& path)
{
	QByteArray hash = QCryptographicHash::hash(QFileInfo(path).canonicalFilePath().toUtf8(), QCryptographicHash::Sha1);
	return QString("previews/" + hash.toHex() + ".png");
}

/*****************************************************************************/

AddImageDialog::AddImageDialog(QWidget* parent)
:	QDialog(parent)
{
	setWindowTitle(tr("Add Image"));
	connect(this, SIGNAL(accepted()), this, SLOT(storePath()));

	// Load image filters
	foreach (QByteArray type, QImageReader::supportedImageFormats()) {
		m_preview_filters.append("*." + type);
	}

	// Create dialog buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_open_button = buttons->button(QDialogButtonBox::Open);
	m_open_button->setEnabled(false);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	// Setup thumbnail list
	int max_thumbnails = QSettings().value("AddImage/MaximumPreviews", 10000).toInt();
	if (max_thumbnails < 0)
		max_thumbnails = 10000;
	m_thumbnails = new ThumbnailList("previews", max_thumbnails, this);

	// Create filesystem treeview
	m_folders_model = new QDirModel(QStringList() << "*", QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase | QDir::LocaleAware, this);
	m_folders_view = new QTreeView(this);
	m_folders_view->header()->hide();
	m_folders_view->setModel(m_folders_model);
	m_folders_view->setColumnHidden(1, true);
	m_folders_view->setColumnHidden(2, true);
	m_folders_view->setColumnHidden(3, true);
	connect(m_folders_view->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(folderSelected(const QModelIndex&)));

	// Create files list
	m_files_widget = new QListWidget(this);
	m_files_widget->setSpacing(12);
	m_files_widget->setViewMode(QListView::IconMode);
	m_files_widget->setIconSize(QSize(100, 100));
	m_files_widget->setMovement(QListView::Static);
	m_files_widget->setResizeMode(QListView::Adjust);
	m_files_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	connect(m_files_widget, SIGNAL(itemSelectionChanged()), this, SLOT(fileSelectionChanged()));
	connect(m_files_widget, SIGNAL(activated(const QModelIndex&)), this, SLOT(accept()));

	// Layout dialog
	m_contents = new QSplitter(this);
	m_contents->addWidget(m_folders_view);
	m_contents->addWidget(m_files_widget);
	m_contents->setStretchFactor(1, 1);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_contents);
	layout->addSpacing(18);
	layout->addWidget(buttons);

	// Load home directory
	m_folders_view->setRootIndex(m_folders_model->index("/"));
	m_current_folder = QSettings().value("AddImage/Path", QDir::homePath()).toString();
	QString path;
	foreach (QString part, m_current_folder.split("/")) {
		path.append("/");
		path.append(part);
		m_folders_view->setExpanded(m_folders_model->index(path), true);
	}
	m_folders_view->selectionModel()->setCurrentIndex(m_folders_model->index(path), QItemSelectionModel::Select);
	m_folders_view->scrollTo(m_folders_model->index(path), QAbstractItemView::PositionAtTop);

	// Resize dialog and splitter
	QSettings settings;
	resize(settings.value("AddImage/Size", QSize(675, 430)).toSize());
	m_contents->restoreState(settings.value("AddImage/Splitter").toByteArray());
}

/*****************************************************************************/

void AddImageDialog::hideEvent(QHideEvent* event)
{
	QSettings settings;
	settings.setValue("AddImage/Size", size());
	settings.setValue("AddImage/Splitter", m_contents->saveState());
	QDialog::hideEvent(event);
}

/*****************************************************************************/

void AddImageDialog::folderSelected(const QModelIndex& index)
{
	m_thumbnails->stop();
	m_files_widget->scrollToTop();
	m_files_widget->clear();

	m_current_folder = m_folders_model->filePath(index) + "/";

	QListWidgetItem* item;
	QString file, preview;
	QStringList files = QDir(m_current_folder).entryList(m_preview_filters, QDir::Files, QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
	foreach (file, files) {
		item = new QListWidgetItem(m_files_widget);
		item->setToolTip(file);

		file.prepend(m_current_folder);
		item->setData(Qt::UserRole, file);

		m_thumbnails->addItem(item, file, previewFileName(file));
	}
	m_thumbnails->start();
}

/*****************************************************************************/

void AddImageDialog::fileSelectionChanged()
{
	QList<QListWidgetItem*> items = m_files_widget->selectedItems();
	m_open_button->setEnabled(items.count());

	images.clear();
	foreach (QListWidgetItem* item, items) {
		images.append(item->data(Qt::UserRole).toString());
	}
}

/*****************************************************************************/

void AddImageDialog::storePath()
{
	QSettings().setValue("AddImage/Path", m_current_folder);
}

/*****************************************************************************/
