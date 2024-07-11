/*
	SPDX-FileCopyrightText: 2010-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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

	const QList<QUrl> urls = event->mimeData()->urls();
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
	const QList<QUrl> urls = event->mimeData()->urls();
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
		const QList<QByteArray> imageformats = QImageReader::supportedImageFormats();
		for (const QByteArray& type : imageformats) {
			formats.append("*." + type);
		}
		string = tr("Images") + " (" + formats.join(" ") + ")";
	}
	return string;
}

//-----------------------------------------------------------------------------
