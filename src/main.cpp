/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include <QApplication>
#include <QDir>
#include <QSettings>

#include "locale_dialog.h"
#include "path.h"
#include "window.h"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	app.setApplicationName("Tetzle");
	app.setOrganizationDomain("gottcode.org");
	app.setOrganizationName("GottCode");

	QDir dir(app.applicationDirPath());
	if (dir.exists("jhead") || dir.exists("jhead.exe")) {
		QString path = QString::fromLocal8Bit(qgetenv("PATH"));
#if !defined(Q_OS_WIN)
		QChar separator = ':';
#else
		QChar separator = ';';
#endif
		if (!path.split(separator).contains(dir.absolutePath())) {
			path += separator + dir.absolutePath();
			qputenv("PATH", path.toLocal8Bit());
		}
	}

	QStringList files = app.arguments().mid(1);

	// Load application language
	LocaleDialog::loadTranslator();

	// Update data location
	QString path = Path::datapath();
	if (!QFile::exists(path)) {
#if defined(Q_OS_MAC)
		QString oldpath = QDir::homePath() + "/Library/Application Support/GottCode/Tetzle/";
#elif defined(Q_OS_UNIX)
		QString oldpath = getenv("$XDG_DATA_HOME");
		if (oldpath.isEmpty()) {
			oldpath = QDir::homePath() + "/.local/share/";
		}
		oldpath += "/games/tetzle/";
#elif defined(Q_OS_WIN32)
		QString oldpath = QDir::homePath() + "/Application Data/GottCode/Tetzle/";
#endif
		if (!QFile::exists(oldpath)) {
			dir = QDir::home();
			dir.mkpath(path);
		} else {
			QFile::rename(oldpath, path);
		}
	}
	dir.setPath(path);
	dir.mkpath(path + "/images/");
	dir.mkpath(path + "/images/thumbnails/");
	dir.mkpath(path + "/saves/");

	// Update settings layout
	QSettings settings;
	if (settings.value("Version", 0).toInt() < 2) {
		settings.setValue("NewGame/Image", settings.value("Image"));
		settings.remove("Image");
		settings.setValue("NewGame/Pieces", settings.value("Pieces"));
		settings.remove("Pieces");
		settings.setValue("AddImage/Path", settings.value("AddImagePath"));
		settings.remove("AddImagePath");
		settings.remove("MaximumPreviews");
		settings.remove("AddImage/MaximumPreviews");
		settings.setValue("Version", 2);
	}

	Window window(files);

	return app.exec();
}
