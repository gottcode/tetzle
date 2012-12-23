/***********************************************************************
 *
 * Copyright (C) 2008, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#ifndef THUMBNAIL_LOADER_H
#define THUMBNAIL_LOADER_H

struct Thumbnail;

#include <QAtomicInt>
#include <QMutex>
#include <QThread>
class QListWidget;
class QListWidgetItem;

class ThumbnailLoader : public QThread
{
	Q_OBJECT

	ThumbnailLoader(QObject* parent = 0);
public:
	~ThumbnailLoader();

	static QListWidgetItem* createItem(const QString& image, const QString& text, QListWidget* list);

signals:
	void loaded(const Thumbnail& details);

protected:
	virtual void run();

private slots:
	void imageLoaded(const Thumbnail& details);

private:
	bool m_done;
	QList<Thumbnail> m_details;
	QMutex m_mutex;
};

#endif
