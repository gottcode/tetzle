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

#include <QApplication>
#include <QDir>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

#include "window.h"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	app.setApplicationName("Tetzle");
	app.setOrganizationDomain("gottcode.org");
	app.setOrganizationName("GottCode");

	QTranslator qt_translator;
	qt_translator.load("qt_" + QLocale::system().name());
	app.installTranslator(&qt_translator);

	QTranslator tetzle_translator;
	tetzle_translator.load("tetzle_" + QLocale::system().name());
	app.installTranslator(&tetzle_translator);

	QDir dir = QDir::home();
#if defined(Q_OS_MAC)
	QString path = QDir::homePath() + "/Library/Application Support/GottCode/Tetzle/";
#elif defined(Q_OS_UNIX)
	QString path = getenv("$XDG_DATA_HOME");
	if (path.isEmpty())
		path = QDir::homePath() + "/.local/share/";
	path += "/games/tetzle/";
#elif defined(Q_OS_WIN32)
	QString path = QDir::homePath() + "/Application Data/GottCode/Tetzle/";
#endif
	dir.mkpath(path);
	dir.mkpath(path + "images/");
	dir.mkpath(path + "images/thumbnails/");
	dir.mkpath(path + "previews/");
	dir.mkpath(path + "saves/");
	QDir::setCurrent(path);

	// Update settings layout
	QSettings settings;
	if (settings.value("Version", 0).toInt() < 1) {
		settings.setValue("NewGame/Image", settings.value("Image"));
		settings.remove("Image");
		settings.setValue("NewGame/Pieces", settings.value("Pieces"));
		settings.remove("Pieces");
		settings.setValue("AddImage/Path", settings.value("AddImagePath"));
		settings.remove("AddImagePath");
		if (settings.contains("MaximumPreviews")) {
			settings.setValue("AddImage/MaximumPreviews", settings.value("MaximumPreviews"));
			settings.remove("MaximumPreviews");
		}
		settings.setValue("Version", 1);
	}

	Window window;

	return app.exec();
}
