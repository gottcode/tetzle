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

#include "thumbnail_list.h"

#include <QtConcurrentRun>
#include <QDateTime>
#include <QFileInfo>
#include <QFuture>
#include <QFutureWatcher>
#include <QListWidgetItem>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QSignalMapper>

//-----------------------------------------------------------------------------

namespace
{

void createThumbnail(const QString& file, const QString& preview)
{
	QImageReader source(file);
	QSize size = source.size();
	if (size.width() > 92 || size.height() > 92) {
		size.scale(92, 92, Qt::KeepAspectRatio);
		source.setScaledSize(size);
	}

	QImage thumbnail(100, 100, QImage::Format_ARGB32);
	thumbnail.fill(0);
	{
		QPainter painter(&thumbnail);
		painter.translate(46 - (size.width() / 2), 46 - (size.height() / 2));
		painter.fillRect(0, 0, size.width() + 8, size.height() + 8, QColor(0, 0, 0, 50));
		painter.fillRect(1, 1, size.width() + 6, size.height() + 6, QColor(0, 0, 0, 75));
		painter.fillRect(2, 2, size.width() + 4, size.height() + 4, Qt::white);
		painter.drawImage(4, 4, source.read(), 0, 0, -1, -1, Qt::AutoColor | Qt::AvoidDither);
	}
	thumbnail.save(preview, 0, 0);
}

}

//-----------------------------------------------------------------------------

ThumbnailList::ThumbnailList(QWidget* parent)
	: QListWidget(parent)
{
	setIconSize(QSize(100, 100));
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	m_mapper = new QSignalMapper(this);
	connect(m_mapper, SIGNAL(mapped(QString)), this, SLOT(generated(QString)));
}

//-----------------------------------------------------------------------------

QListWidgetItem* ThumbnailList::addImage(const QString& image)
{
	QListWidgetItem* item = new QListWidgetItem(this);

	QFileInfo image_info(image);
	QFileInfo thumb_info("images/thumbnails/" + image_info.baseName() + ".png");
	QString thumbnail = thumb_info.filePath();

	if (!thumb_info.exists() || thumb_info.lastModified() < image_info.lastModified()) {
		item->setIcon(QPixmap(":/loading.png"));
		item->setData(Qt::UserRole + 1, thumbnail);
		m_items[image] = item;

		QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
		connect(watcher, SIGNAL(finished()), m_mapper, SLOT(map()));
		connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));
		m_mapper->setMapping(watcher, image);

		QFuture<void> future = QtConcurrent::run(createThumbnail, image, thumbnail);
		watcher->setFuture(future);
		m_futures.addFuture(future);

		setCursor(Qt::BusyCursor);
	} else {
		item->setIcon(QPixmap(thumbnail));
	}

	return item;
}

//-----------------------------------------------------------------------------

void ThumbnailList::generated(const QString& path)
{
	Q_ASSERT(m_items.contains(path));
	QListWidgetItem* item = m_items[path];
	item->setIcon(QPixmap(item->data(Qt::UserRole + 1).toString()));
	if (m_futures.futures().last().isFinished()) {
		m_futures.clearFutures();
		unsetCursor();
	}
}

//-----------------------------------------------------------------------------
