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

#include <QFutureSynchronizer>
#include <QHash>
#include <QListWidget>
#include <QPixmap>
class QListWidgetItem;
class QSignalMapper;

class ThumbnailList : public QListWidget
{
	Q_OBJECT
public:
	ThumbnailList(QWidget* parent = 0);

	QListWidgetItem* addImage(const QString& image);

private slots:
	void generated(const QString& path);

private:
	QHash<QString, QListWidgetItem*> m_items;
	QSignalMapper* m_mapper;
	QFutureSynchronizer<void> m_futures;
};

#endif
