/***********************************************************************
 *
 * Copyright (C) 2010, 2011, 2012 Graeme Gott <graeme@gottcode.org>
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

#include "path.h"

#include <QString>
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
#include <QStandardPaths>
#else
#include <QDesktopServices>
#endif

//-----------------------------------------------------------------------------

QString Path::datapath()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
	static QString path = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/";
#else
	static QString path = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/";
#endif
	return path;
}

//-----------------------------------------------------------------------------

QString Path::image(const QString& file)
{
	return images() + file;
}

//-----------------------------------------------------------------------------

QString Path::thumbnail(const QString& image)
{
	return thumbnails() + image + ".png";
}

//-----------------------------------------------------------------------------

QString Path::save(const QString& file)
{
	return saves() + file;
}

//-----------------------------------------------------------------------------

QString Path::save(int game)
{
	return save(QString::number(game) + ".xml");
}

//-----------------------------------------------------------------------------

QString Path::images()
{
	return datapath() + "images/";
}

//-----------------------------------------------------------------------------

QString Path::thumbnails()
{
	return datapath() + "images/thumbnails/";
}

//-----------------------------------------------------------------------------

QString Path::saves()
{
	return datapath() + "saves/";
}

//-----------------------------------------------------------------------------
