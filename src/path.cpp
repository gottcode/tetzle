/*
	SPDX-FileCopyrightText: 2010-2020 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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
