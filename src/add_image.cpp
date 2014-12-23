/***********************************************************************
 *
 * Copyright (C) 2010, 2012, 2014 Graeme Gott <graeme@gottcode.org>
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

#include "add_image.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QImageReader>
#include <QMimeData>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QWidget>

//-----------------------------------------------------------------------------

void AddImage::dragEnterEvent(QDragEnterEvent* event)
{
	bool accept = false;

	QList<QUrl> urls = event->mimeData()->urls();
	for (const QUrl& url : urls) {
		if (QDir::match(supportedFormats(), url.toLocalFile())) {
			accept = true;
			break;
		}
	}

	if (accept) {
		event->acceptProposedAction();
	}
}

//-----------------------------------------------------------------------------

QStringList AddImage::dropEvent(QDropEvent* event)
{
	event->setDropAction(Qt::CopyAction);

	QStringList files;
	QList<QUrl> urls = event->mimeData()->urls();
	for (const QUrl& url : urls) {
		QString file = url.toLocalFile();
		if (QDir::match(supportedFormats(), file)) {
			files.append(file);
		}
	}

	if (!files.isEmpty()) {
		event->acceptProposedAction();
	}

	return files;
}

//-----------------------------------------------------------------------------

QStringList AddImage::getOpenFileNames(QWidget* parent)
{
	QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	dir = QSettings().value("AddImage/Path", dir).toString();
	QStringList images = QFileDialog::getOpenFileNames(parent, tr("Open Image"), dir, supportedFormats());
	if (!images.isEmpty()) {
		QSettings().setValue("AddImage/Path", QFileInfo(images.last()).absolutePath());
	}
	return images;
}

//-----------------------------------------------------------------------------

QString AddImage::supportedFormats()
{
	static QString string;
	if (string.isEmpty()) {
		QStringList formats;
		for (const QByteArray& type : QImageReader::supportedImageFormats()) {
			formats.append("*." + type);
		}
		string = "Images(" + formats.join(" ") + ")";
	}
	return string;
}

//-----------------------------------------------------------------------------
