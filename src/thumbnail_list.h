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

#ifndef THUMBNAIL_LIST
#define THUMBNAIL_LIST

#include <QHash>
#include <QObject>
#include <QPixmap>
class QListWidgetItem;
class ThumbnailLoader;

class ThumbnailList : public QObject
{
	Q_OBJECT
public:
	ThumbnailList(QObject* parent = 0);
	~ThumbnailList();

	void addItem(QListWidgetItem* item, const QString& image, const QString& thumbnail);

private slots:
	void generated(const QString& path);

private:
	ThumbnailLoader* m_loader;
	QPixmap m_loading;
	QHash<QString, QListWidgetItem*> m_items;
};

#endif
