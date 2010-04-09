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

#include "thumbnail_loader.h"

#include <QDateTime>
#include <QFileInfo>
#include <QListWidgetItem>

//-----------------------------------------------------------------------------

ThumbnailList::ThumbnailList(QObject* parent)
	: QObject(parent)
{
	m_loading = ThumbnailLoader::loadingIcon();

	m_loader = new ThumbnailLoader;
	connect(m_loader, SIGNAL(generated(const QString&)), this, SLOT(generated(const QString&)));
}

//-----------------------------------------------------------------------------

ThumbnailList::~ThumbnailList()
{
	m_loader->stop();
	m_loader->wait();
	delete m_loader;
}

//-----------------------------------------------------------------------------

void ThumbnailList::addItem(QListWidgetItem* item, const QString& image, const QString& thumbnail)
{
	QFileInfo info(thumbnail);
	if (!info.exists() || info.lastModified() < QFileInfo(image).lastModified()) {
		item->setIcon(m_loading);
		item->setData(Qt::UserRole + 1, thumbnail);
		m_items[image] = item;
		m_loader->add(image, thumbnail);
	} else {
		item->setIcon(QPixmap(thumbnail));
	}
}

//-----------------------------------------------------------------------------

void ThumbnailList::generated(const QString& path)
{
	Q_ASSERT(m_items.contains(path));
	QListWidgetItem* item = m_items[path];
	item->setIcon(QIcon(item->data(Qt::UserRole + 1).toString()));
}

//-----------------------------------------------------------------------------
