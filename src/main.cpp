/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2018, 2019 Graeme Gott <graeme@gottcode.org>
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
#include <QCommandLineParser>
#include <QDir>
#include <QFileOpenEvent>
#include <QLabel>
#include <QSettings>

#include "board.h"
#include "locale_dialog.h"
#include "path.h"
#include "window.h"

//-----------------------------------------------------------------------------

class Application : public QApplication
{
public:
	Application(int& argc, char** argv);

	void createWindow();

protected:
	virtual bool event(QEvent* e);

private:
	QStringList m_files;
	Window* m_window;
};

//-----------------------------------------------------------------------------

Application::Application(int& argc, char** argv)
	: QApplication(argc, argv),
	m_window(0)
{
	setApplicationName("Tetzle");
	setApplicationVersion(VERSIONSTR);
	setApplicationDisplayName(Window::tr("Tetzle"));
	setOrganizationDomain("gottcode.org");
	setOrganizationName("GottCode");
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
	setWindowIcon(QIcon::fromTheme("tetzle", QIcon(":/tetzle.png")));
	setDesktopFileName("tetzle");
#endif
	setAttribute(Qt::AA_UseHighDpiPixmaps, true);

	m_files = arguments().mid(1);
	processEvents();
}

//-----------------------------------------------------------------------------

void Application::createWindow()
{
	// Remove command-line parameters from file list
	for (int i = 0; i < m_files.size(); ++i) {
		if (m_files.at(i).startsWith("--")) {
			m_files.removeAt(i);
			--i;
		}
	}

	m_window = new Window(m_files);
}

//-----------------------------------------------------------------------------

bool Application::event(QEvent* e)
{
	if (e->type() != QEvent::FileOpen) {
		return QApplication::event(e);
	} else {
		QString file = static_cast<QFileOpenEvent*>(e)->file();
		if (m_window) {
			m_window->addImages(QStringList(file));
		} else {
			m_files.append(file);
		}
		e->accept();
		return true;
	}
}

//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
#if !defined(Q_OS_MAC)
	if (!qEnvironmentVariableIsSet("QT_DEVICE_PIXEL_RATIO")
			&& !qEnvironmentVariableIsSet("QT_AUTO_SCREEN_SCALE_FACTOR")
			&& !qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
			&& !qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS")) {
		QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	}
#endif
	Application app(argc, argv);

	// Load application language
	LocaleDialog::loadTranslator("tetzle_");

	// Load command-line settings
	QSettings settings;
	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::translate("Window", "A jigsaw puzzle with tetrominoes for pieces"));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOption(QCommandLineOption(QStringList() << "G" << "graphics-layer",
		QCoreApplication::translate("main", "Select OpenGL version."),
		QCoreApplication::translate("main", "version")));
	parser.process(app);

	// Set OpenGL version
	QString requested = settings.value("GraphicsLayer").toString();
	if (parser.isSet("graphics-layer")) {
		requested = parser.value("graphics-layer");
	}
	requested = requested.remove('.');
	requested = requested.split(' ').first();

	GraphicsLayer::setVersion(requested.toInt());

	// Create data location
	QString path = Path::datapath();
	if (!QFile::exists(path)) {
		QDir dir(path);
		dir.mkpath(dir.absolutePath());

		// Migrate data from old location
		QString oldpath = Path::oldDataPath();
		if (QFile::exists(oldpath)) {
			QStringList old_dirs = QStringList() << "";

			QDir olddir(oldpath);
			for (int i = 0; i < old_dirs.count(); ++i) {
				QString subpath = old_dirs.at(i);
				dir.mkpath(path + "/" + subpath);
				olddir.setPath(oldpath + "/" + subpath);

				QStringList subdirs = olddir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
				for (const QString& subdir : subdirs) {
					old_dirs.append(subpath + "/" + subdir);
				}

				QStringList files = olddir.entryList(QDir::Files);
				for (const QString& file : files) {
					QFile::rename(olddir.absoluteFilePath(file), path + "/" + subpath + "/" + file);
				}
			}

			olddir.setPath(oldpath);
			for (int i = old_dirs.count() - 1; i >= 0; --i) {
				olddir.rmdir(oldpath + "/" + old_dirs.at(i));
			}
		}
	}
	QDir dir(path);
	dir.mkpath(path + "/saves/");

	// Update thumbnails
	if (dir.exists("previews")) {
		QLabel label(Board::tr("Please Wait"), 0, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
		label.setWindowTitle(Window::tr("Tetzle"));
		label.setAlignment(Qt::AlignCenter);
		label.setContentsMargins(10, 10, 10, 10);
		label.setFixedSize(label.sizeHint());
		label.show();
		app.processEvents();

		QStringList old_dirs;
		old_dirs += (path + "/previews/");
		old_dirs += (path + "/images/thumbnails/");
		old_dirs += (path + "/images/thumbnails/large/");
		old_dirs += (path + "/images/thumbnails/small/");
		for (int i = 0; i < old_dirs.count(); ++i) {
			dir.setPath(old_dirs.at(i));
			QStringList entries = dir.entryList(QDir::Files);
			for (const QString& info : entries) {
				dir.remove(info);
			}
		}

		dir.setPath(path);
		while (!old_dirs.isEmpty()) {
			dir.rmpath(old_dirs.takeLast());
		}
	}
	dir.mkpath(path + "/images/");
	dir.mkpath(path + "/images/thumbnails/");

	// Update settings layout
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

	// Reset tracking of the game currently open
	settings.remove("OpenGame");

	app.createWindow();

	return app.exec();
}

//-----------------------------------------------------------------------------
