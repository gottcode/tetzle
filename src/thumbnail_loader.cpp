/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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

// Exported by QtGui
void qt_blurImage(QPainter* p, QImage& blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);

//-----------------------------------------------------------------------------

struct Thumbnail
{
	QListWidget* list;
	QPersistentModelIndex index;
	QString image;
	QString thumbnail;
	qreal pixelratio;
};

Q_DECLARE_METATYPE(Thumbnail)

//-----------------------------------------------------------------------------

namespace
{

class ThumbnailItem : public QListWidgetItem
{
public:
	explicit ThumbnailItem(const QString& text = QString());

	bool operator<(const QListWidgetItem& other) const override;

private:
	enum ItemRoles
	{
		ImageRole = Qt::UserRole + 1
	};
};

ThumbnailItem::ThumbnailItem(const QString& text)
	: QListWidgetItem(QIcon::fromTheme("image-loading"), text)
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

ThumbnailLoader::ThumbnailLoader(QObject* parent)
	: QThread(parent)
	, m_done(false)
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

QListWidgetItem* ThumbnailLoader::createItem(const QString& image, const QString& text, QListWidget* list, qreal pixelratio)
{
	static ThumbnailLoader* loader = nullptr;
	if (!loader) {
		qRegisterMetaType<Thumbnail>("Thumbnail");
		loader = new ThumbnailLoader(QCoreApplication::instance());
	}

	QListWidgetItem* item = new ThumbnailItem(text);
	list->addItem(item);

	const QFileInfo image_info(image);
	const QFileInfo thumb_info(Path::thumbnail(image_info.baseName(), pixelratio));
	const Thumbnail details{ list, list->model()->index(list->row(item), 0), image, thumb_info.filePath(), pixelratio };

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
	Q_FOREVER {
		// Fetch next thumbnail to process
		m_mutex.lock();
		if (m_done || m_details.isEmpty()) {
			m_mutex.unlock();
			break;
		}
		const Thumbnail details = m_details.takeFirst();
		m_mutex.unlock();

		// Skip already generated thumbnails
		if (!QFile::exists(details.image)) {
			continue;
		} else if (QFile::exists(details.thumbnail)) {
			Q_EMIT loaded(details);
			continue;
		}

		// Generate thumbnail
		QImageReader source(details.image);
		QSize size = source.size();
		if (size.width() > 64 || size.height() > 64) {
			size.scale(64, 64, Qt::KeepAspectRatio);
			source.setScaledSize(size * details.pixelratio);
		}

		QImage thumbnail(74 * details.pixelratio, 74 * details.pixelratio, QImage::Format_ARGB32);
		thumbnail.setDevicePixelRatio(details.pixelratio);
		thumbnail.fill(0);
		{
			QPainter painter(&thumbnail);

			QImage shadow(thumbnail.size(), QImage::Format_ARGB32_Premultiplied);
			shadow.fill(0);

			QPainter shadow_painter(&shadow);
			shadow_painter.setRenderHint(QPainter::Antialiasing);
			shadow_painter.setPen(Qt::NoPen);
			shadow_painter.translate(35 - (size.width() / 2), 35 - (size.height() / 2));
			shadow_painter.fillRect(QRectF(0, 0, size.width() + 4, size.height() + 4), Qt::black);
			shadow_painter.end();

			painter.save();
			painter.setClipping(false);
			qt_blurImage(&painter, shadow, 8, true, false);
			painter.restore();

			painter.translate(32 - (size.width() / 2), 32 - (size.height() / 2));
			painter.fillRect(2, 2, size.width() + 4, size.height() + 4, Qt::white);
			QImage image = source.read();
			image.setDevicePixelRatio(details.pixelratio);
			painter.drawImage(4, 4, image, 0, 0, -1, -1, Qt::AutoColor | Qt::AvoidDither);
		}
		thumbnail.save(details.thumbnail, 0, 0);

		Q_EMIT loaded(details);
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
