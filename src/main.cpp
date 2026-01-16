/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "locale_dialog.h"
#include "path.h"
#include "window.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QSettings>

//-----------------------------------------------------------------------------

/**
 * %Application instance to handle player dropping images on program.
 */
class Application : public QApplication
{
public:
	/**
	 * Construct an application instance.
	 *
	 * @param argc amount of arguments
	 * @param argv array of arguments
	 */
	Application(int& argc, char** argv);

	/**
	 * Create the main window.
	 *
	 * @param files list of images to add to new game
	 */
	void createWindow(const QStringList& files);

protected:
	/**
	 * Handle player dropping images by passing them to main window.
	 */
	bool event(QEvent* e) override;

private:
	QStringList m_files; ///< list of images to pass to main window
	Window* m_window; ///< main window of game
};

//-----------------------------------------------------------------------------

Application::Application(int& argc, char** argv)
	: QApplication(argc, argv)
	, m_window(nullptr)
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

	processEvents();
}

//-----------------------------------------------------------------------------

void Application::createWindow(const QStringList& files)
{
	m_files += files;
	m_window = new Window(m_files);
}

//-----------------------------------------------------------------------------

bool Application::event(QEvent* e)
{
	if (e->type() != QEvent::FileOpen) {
		return QApplication::event(e);
	} else {
		const QString file = static_cast<QFileOpenEvent*>(e)->file();
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

	// Load application data
	const QString appdir = app.applicationDirPath();
	const QString datadir = QDir::cleanPath(appdir + "/" + TETZLE_DATADIR);

	// Handle portability
	QString userdir;
#ifdef Q_OS_MAC
	const QFileInfo portable(appdir + "/../../../Data");
#else
	const QFileInfo portable(appdir + "/Data");
#endif
	if (portable.exists() && portable.isWritable()) {
		userdir = portable.absoluteFilePath();
		QSettings::setDefaultFormat(QSettings::IniFormat);
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, userdir + "/Settings");
	}

	// Load application language
	LocaleDialog::loadTranslator("tetzle_", datadir);

	// Load command-line settings
	QSettings settings;
	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::translate("Window", "A jigsaw puzzle with tetrominoes for pieces"));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("files", QCoreApplication::translate("main", "Images to add to the choose game dialog."), "[files]");
	parser.process(app);

	// Create data location
	Path::load(userdir);

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

	app.createWindow(parser.positionalArguments());

	return app.exec();
}

//-----------------------------------------------------------------------------
