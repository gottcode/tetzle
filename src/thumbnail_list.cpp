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

#include "thumbnail_list.h"

#include "thumbnail.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QPainter>
#include <QPixmapCache>

/*****************************************************************************/

ThumbnailList::ThumbnailList(QObject* parent)
:	QObject(parent),
	m_max_thumbnails(0x7FFFFFFF)
{
}

/*****************************************************************************/

ThumbnailList::ThumbnailList(const QString& directory, int max_thumbnails, QObject* parent)
:	QObject(parent),
	m_max_thumbnails(max_thumbnails)
{
	// Load list of current thumbnails
	m_thumbnails = QDir(directory, "*", QDir::Time | QDir::Reversed, QDir::Files).entryList();
	int count = m_thumbnails.count();
	for (int i = 0; i < count; ++i) {
		m_thumbnails[i].prepend(directory + "/");
	}
	count = m_thumbnails.count() - m_max_thumbnails;
	for (int i = 0; i < count; ++i) {
		QFile(m_thumbnails.takeFirst()).remove();
	}
}

/*****************************************************************************/

void ThumbnailList::addItem(QListWidgetItem* item, const QString& image, const QString& thumbnail)
{
	QFileInfo info(thumbnail);
	if (!info.exists() || info.lastModified() < QFileInfo(image).lastModified()) {
		item->setIcon(loadingIcon());
		Item thumb = { item, thumbnail };
		m_items[image] = thumb;
	} else {
		item->setIcon(QPixmap(thumbnail));
	}
}

/*****************************************************************************/

void ThumbnailList::start()
{
	if (m_items.count()) {
		QString image = m_items.begin().key();
		Thumbnail* thumbnail = new Thumbnail(image, m_items.begin().value().m_thumbnail);
		connect(thumbnail, SIGNAL(generated(const QString&)), this, SLOT(previewGenerated(const QString&)));
		thumbnail->start();
	}
}

/*****************************************************************************/

void ThumbnailList::stop()
{
	m_items.clear();
}

/*****************************************************************************/

void ThumbnailList::previewGenerated(const QString& path)
{
	if (!m_items.contains(path))
		return;

	const Item& item = m_items[path];
	item.m_item->setIcon(QIcon(item.m_thumbnail));

	m_thumbnails.append(item.m_thumbnail);
	int count = (m_thumbnails.count() - 1) - m_max_thumbnails;
	for (int i = 0; i < count; ++i) {
		QFile(m_thumbnails.takeFirst()).remove();
	}
	m_items.erase(m_items.find(path));

	if (m_items.count()) {
		QString file = m_items.begin().key();
		Thumbnail* thumbnail = new Thumbnail(file, m_items.begin().value().m_thumbnail);
		connect(thumbnail, SIGNAL(generated(const QString&)), this, SLOT(previewGenerated(const QString&)));
		thumbnail->start();
	}
}

/*****************************************************************************/

QPixmap ThumbnailList::loadingIcon()
{
	QPixmap result;
	if (!QPixmapCache::find("loadingIcon", result)) {
		result = QPixmap(100, 100);
		result.fill(QColor(0, 0, 0, 0));

		QPainter painter(&result);
		painter.translate(32, 32);
		painter.setRenderHint(QPainter::Antialiasing, true);

		painter.setPen(QColor(100, 100, 100));
		painter.setBrush(QColor(200, 200, 200));
		painter.drawEllipse(0, 0, 36, 36);

		painter.setBrush(Qt::white);
		painter.drawEllipse(2, 2, 32, 32);

		painter.setPen(QPen(QColor(100, 100, 100), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter.drawPoint(18, 6);
		painter.drawPoint(18, 30);
		painter.drawPoint(6, 18);
		painter.drawPoint(30, 18);

		painter.setPen(QPen(QColor(0, 0, 0), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter.setBrush(QColor(0, 0, 0, 0));
		painter.drawEllipse(16, 16, 4, 4);
		painter.drawLine(20, 20, 27, 24);

		painter.setPen(QPen(QColor(0, 0, 0), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		painter.drawLine(19, 16, 22, 6);

		painter.end();
		QPixmapCache::insert("loadingIcon", result);
	}
	return result;
}

/*****************************************************************************/
