/***********************************************************************
 *
 * Copyright (C) 2008, 2011, 2012, 2014 Graeme Gott <graeme@gottcode.org>
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

#include "thumbnail_loader.h"

#include "path.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPersistentModelIndex>

//-----------------------------------------------------------------------------

struct Thumbnail {
	QListWidget* list;
	QPersistentModelIndex index;
	QString image;
	QString thumbnail;
};

Q_DECLARE_METATYPE(Thumbnail)

//-----------------------------------------------------------------------------

namespace
{
	class ThumbnailItem : public QListWidgetItem
	{
	public:
		ThumbnailItem(const QString& text = QString());

		virtual bool operator<(const QListWidgetItem& other) const;

	private:
		enum ItemRoles
		{
			ImageRole = Qt::UserRole + 1
		};
	};

	ThumbnailItem::ThumbnailItem(const QString& text)
		: QListWidgetItem(QPixmap(":/loading.png"), text)
	{
	}

	bool ThumbnailItem::operator<(const QListWidgetItem& other) const
	{
		int compare = text().localeAwareCompare(other.text());
		if (compare == 0) {
			compare = data(ImageRole).toString().localeAwareCompare(other.data(ImageRole).toString());
		}
		return compare < 0;
	}
}

//-----------------------------------------------------------------------------

ThumbnailLoader::ThumbnailLoader(QObject* parent) :
	QThread(parent),
	m_done(false)
{
	connect(this, &ThumbnailLoader::loaded, this, &ThumbnailLoader::imageLoaded, Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------

ThumbnailLoader::~ThumbnailLoader()
{
	m_mutex.lock();
	m_done = true;
	m_mutex.unlock();
	wait();
}

//-----------------------------------------------------------------------------

QListWidgetItem* ThumbnailLoader::createItem(const QString& image, const QString& text, QListWidget* list)
{
	static ThumbnailLoader* loader = 0;
	if (loader == 0) {
		qRegisterMetaType<Thumbnail>("Thumbnail");
		loader = new ThumbnailLoader(QCoreApplication::instance());
	}

	QListWidgetItem* item = new ThumbnailItem(text);
	list->addItem(item);

	QFileInfo image_info(image);
	QFileInfo thumb_info(Path::thumbnail(image_info.baseName()));
	Thumbnail details = { list, list->model()->index(list->row(item), 0), image, thumb_info.filePath() };

	if (!thumb_info.exists() || thumb_info.lastModified() < image_info.lastModified()) {
		loader->m_mutex.lock();
		loader->m_details.append(details);
		loader->m_mutex.unlock();
		if (!loader->isRunning()) {
			loader->start();
		}
	} else {
		loader->imageLoaded(details);
	}

	return item;
}

//-----------------------------------------------------------------------------

void ThumbnailLoader::run()
{
	forever {
		// Fetch next thumbnail to process
		m_mutex.lock();
		if (m_done || m_details.isEmpty()) {
			m_mutex.unlock();
			break;
		}
		Thumbnail details = m_details.takeFirst();
		m_mutex.unlock();

		// Skip already generated thumbnails
		if (!QFile::exists(details.image)) {
			continue;
		} else if (QFile::exists(details.thumbnail)) {
			emit loaded(details);
			continue;
		}

		// Generate thumbnail
		QImageReader source(details.image);
		QSize size = source.size();
		if (size.width() > 64 || size.height() > 64) {
			size.scale(64, 64, Qt::KeepAspectRatio);
			source.setScaledSize(size);
		}

		QImage thumbnail(74, 74, QImage::Format_ARGB32);
		thumbnail.fill(0);
		{
			QPainter painter(&thumbnail);
			painter.setRenderHint(QPainter::Antialiasing);
			painter.translate(32 - (size.width() / 2), 32 - (size.height() / 2));

			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor(0,0,0,11));
			painter.drawRoundedRect(0, 0, size.width() + 10, size.height() + 10, 4.5, 4.5);
			painter.setBrush(QColor(0,0,0,29));
			painter.drawRoundedRect(1, 1, size.width() + 8, size.height() + 8, 4.5, 4.5);
			painter.setBrush(QColor(0,0,0,65));
			painter.drawRoundedRect(2, 2, size.width() + 6, size.height() + 6, 4.5, 4.5);
			painter.setBrush(QColor(0,0,0,108));
			painter.drawRoundedRect(3, 3, size.width() + 4, size.height() + 4, 4.5, 4.5);

			painter.fillRect(2, 2, size.width() + 4, size.height() + 4, Qt::white);
			painter.drawImage(4, 4, source.read(), 0, 0, -1, -1, Qt::AutoColor | Qt::AvoidDither);
		}
		thumbnail.save(details.thumbnail, 0, 0);

		emit loaded(details);
	}
}

//-----------------------------------------------------------------------------

void ThumbnailLoader::imageLoaded(const Thumbnail& details)
{
	if (details.index.isValid()) {
		details.list->item(details.index.row())->setIcon(QPixmap(details.thumbnail));
	}
}

//-----------------------------------------------------------------------------
