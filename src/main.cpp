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
	setApplicationVersion("2.0.0");
	setOrganizationDomain("gottcode.org");
	setOrganizationName("GottCode");
	{
		QIcon fallback(":/hicolor/128x128/apps/tetzle.png");
		fallback.addFile(":/hicolor/64x64/apps/tetzle.png");
		fallback.addFile(":/hicolor/48x48/apps/tetzle.png");
		fallback.addFile(":/hicolor/32x32/apps/tetzle.png");
		fallback.addFile(":/hicolor/22x22/apps/tetzle.png");
		fallback.addFile(":/hicolor/16x16/apps/tetzle.png");
		setWindowIcon(QIcon::fromTheme("tetzle", fallback));
	}

	m_files = arguments().mid(1);
	processEvents();
}

//-----------------------------------------------------------------------------

void Application::createWindow()
{
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
	Application app(argc, argv);

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

	// Load application language
	LocaleDialog::loadTranslator("tetzle_");

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
#if defined(Q_WS_X11)
		extern void qt_x11_wait_for_window_manager(QWidget* widget);
		qt_x11_wait_for_window_manager(&label);
#endif

		QStringList old_dirs;
		old_dirs += (path + "/previews/");
		old_dirs += (path + "/images/thumbnails/");
		old_dirs += (path + "/images/thumbnails/large/");
		old_dirs += (path + "/images/thumbnails/small/");
		for (int i = 0; i < old_dirs.count(); ++i) {
			dir.setPath(old_dirs.at(i));
			QStringList entries = dir.entryList(QDir::Files);
			foreach (QString info, entries) {
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

	app.createWindow();

	return app.exec();
}

//-----------------------------------------------------------------------------
