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

#ifndef THUMBNAIL_LIST
#define THUMBNAIL_LIST

#include <QMap>
#include <QObject>
#include <QStringList>
class QListWidgetItem;
class QPixmap;

class ThumbnailList : public QObject
{
	Q_OBJECT
public:
	ThumbnailList(QObject* parent = 0);
	ThumbnailList(const QString& directory, int max_thumbnails, QObject* parent = 0);

	void addItem(QListWidgetItem* item, const QString& image, const QString& thumbnail);
	void start();
	void stop();

private slots:
	void previewGenerated(const QString& path);

private:
	QPixmap loadingIcon();

	struct Item
	{
		QListWidgetItem* m_item;
		QString m_thumbnail;
	};

	QMap<QString, Item> m_items;
	QStringList m_thumbnails;
	int m_max_thumbnails;
};

#endif // THUMBNAIL_LIST
