/***********************************************************************
 *
 * Copyright (C) 2010-2020 Graeme Gott <graeme@gottcode.org>
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

#include <QDir>
#include <QFile>
#include <QString>
#include <QStandardPaths>
#include <QStringList>

#include <cstdlib>

//-----------------------------------------------------------------------------

QString Path::datapath()
{
	static QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
	return path;
}

//-----------------------------------------------------------------------------

QString Path::oldDataPath()
{
	QStringList oldpaths;
	QString oldpath;

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	// Data path from Qt 4 version of 2.0
	oldpaths.append(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data/GottCode/Tetzle");
#endif

	// Data path from 1.0
#if defined(Q_OS_MAC)
	oldpath = QDir::homePath() + "/Library/Application Support/GottCode/Tetzle/";
#elif defined(Q_OS_UNIX)
	oldpath = getenv("$XDG_DATA_HOME");
	if (oldpath.isEmpty()) {
		oldpath = QDir::homePath() + "/.local/share/";
	}
	oldpath += "/games/tetzle/";
#elif defined(Q_OS_WIN32)
	oldpath = QDir::homePath() + "/Application Data/GottCode/Tetzle/";
#endif
	if (!oldpaths.contains(oldpath)) {
		oldpaths.append(oldpath);
	}

	// Check if an old data location exists
	oldpath.clear();
	for (const QString& testpath : oldpaths) {
		if (QFile::exists(testpath)) {
			oldpath = testpath;
			break;
		}
	}

	return oldpath;
}

//-----------------------------------------------------------------------------

QString Path::image(const QString& file)
{
	return images() + file;
}

//-----------------------------------------------------------------------------

QString Path::thumbnail(const QString& image, qreal pixelratio)
{
	QString pixel;
	if (pixelratio > 1.0) {
		pixel = QString("@%1x").arg(pixelratio);
	}
	return thumbnails() + image + pixel + ".png";
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
