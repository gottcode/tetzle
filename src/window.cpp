/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011, 2014, 2015 Graeme Gott <graeme@gottcode.org>
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

#include "window.h"

#include "add_image.h"
#include "appearance_dialog.h"
#include "board.h"
#include "choose_game_dialog.h"
#include "locale_dialog.h"
#include "zoom_slider.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>

//-----------------------------------------------------------------------------

Window::Window(const QStringList& files)
	: m_board(0)
{
	setAcceptDrops(true);
	resize(640, 480);

	// Add statusbar
	m_slider = new ZoomSlider(this);
	statusBar()->addPermanentWidget(m_slider);

	m_completed = new QProgressBar(this);
	m_completed->setRange(0, 100);
	m_completed->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
	statusBar()->addPermanentWidget(m_completed);

	statusBar()->setMinimumHeight(statusBar()->sizeHint().height());
	m_slider->hide();
	m_completed->hide();

	// Add contents
	m_board = new Board(this);
	connect(m_board, &Board::completionChanged, m_completed, &QProgressBar::setValue);
	connect(m_board, &Board::finished, this, &Window::gameFinished);
	connect(m_board, &Board::clearMessage, statusBar(), &QStatusBar::clearMessage);
	connect(m_board, &Board::showMessage, statusBar(), &QStatusBar::showMessage);
	connect(m_board, &Board::zoomChanged, m_slider, &ZoomSlider::setValue);
	connect(m_slider, &ZoomSlider::valueChanged, m_board, &Board::zoom);
	setCentralWidget(m_board);

	// Add menus
	QMenu* menu;

	menu = menuBar()->addMenu(tr("&Game"));
	QAction* choose_action = menu->addAction(tr("&Choose..."), this, SLOT(chooseGame()));
	choose_action->setShortcuts(QList<QKeySequence>() << QKeySequence::New << QKeySequence::Open);
	menu->addSeparator();
	QAction* retrieve_pieces_action = menu->addAction(tr("&Retrieve Pieces"), m_board, SLOT(retrievePieces()), tr("Ctrl+R"));
	retrieve_pieces_action->setEnabled(false);
	connect(m_board, &Board::retrievePiecesAvailable, retrieve_pieces_action, &QAction::setEnabled);
	menu->addSeparator();
	QAction* quit_action = menu->addAction(tr("&Quit"), this, SLOT(close()), QKeySequence::Quit);
	quit_action->setMenuRole(QAction::QuitRole);

	menu = menuBar()->addMenu(tr("&View"));
	QAction* zoom_in_action = menu->addAction(tr("Zoom &In"), m_board, SLOT(zoomIn()), tr("+"));
	zoom_in_action->setEnabled(false);
	connect(m_board, &Board::zoomInAvailable, zoom_in_action, &QAction::setEnabled);
	QAction* zoom_out_action = menu->addAction(tr("Zoom &Out"), m_board, SLOT(zoomOut()), tr("-"));
	zoom_out_action->setEnabled(false);
	connect(m_board, &Board::zoomOutAvailable, zoom_out_action, &QAction::setEnabled);
	m_zoom_fit_action = menu->addAction(tr("Best &Fit"), m_board, SLOT(zoomFit()));
	m_zoom_fit_action->setEnabled(false);
	menu->addSeparator();
	m_toggle_overview_action = menu->addAction(tr("Show O&verview"), m_board, SLOT(toggleOverview()), tr("Tab"));
	m_toggle_overview_action->setCheckable(true);
	m_toggle_overview_action->setEnabled(false);
	connect(m_board, &Board::overviewToggled, m_toggle_overview_action, &QAction::setChecked);
	menu->addSeparator();

	QAction* fullscreen_action = menu->addAction(tr("F&ullscreen"));
	connect(fullscreen_action, &QAction::toggled, this, &Window::setFullScreen);
	fullscreen_action->setCheckable(true);
#if !defined(Q_OS_MAC)
	fullscreen_action->setShortcut(tr("F11"));
#else
	fullscreen_action->setShortcut(tr("Ctrl+F"));
#endif

	menu = menuBar()->addMenu(tr("&Settings"));
	menu->addAction(tr("&Appearance..."), this, SLOT(showAppearance()));
	menu->addAction(tr("&Language..."), this, SLOT(setLocale()));

	menu = menuBar()->addMenu(tr("&Help"));
	menu->addAction(tr("&Controls"), this, SLOT(showControls()), QKeySequence::HelpContents);
	menu->addSeparator();
	QAction* about_action = menu->addAction(tr("&About"), this, SLOT(showAbout()));
	about_action->setMenuRole(QAction::AboutRole);
	QAction* about_qt_action = menu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
	about_qt_action->setMenuRole(QAction::AboutQtRole);

	// Restore geometry
	QSettings settings;
	if (settings.contains("Geometry")) {
		restoreGeometry(settings.value("Geometry").toByteArray());
	} else {
		resize(settings.value("Size", QSize(1024, 768)).toSize());
		settings.remove("Size");
	}

	// Start or load a game
	show();
	chooseGame(files);

	// Create auto-save timer
	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, m_board, &Board::saveGame);
	timer->start(300000);
}

//-----------------------------------------------------------------------------

void Window::addImages(const QStringList& files)
{
	chooseGame(files);
}

//-----------------------------------------------------------------------------

void Window::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::WindowStateChange) {
		m_board->update();
	}
	return QMainWindow::changeEvent(event);
}

//-----------------------------------------------------------------------------

void Window::closeEvent(QCloseEvent* event)
{
	QSettings().setValue("Geometry", saveGeometry());

	m_board->saveGame();

	QMainWindow::closeEvent(event);
}

//-----------------------------------------------------------------------------

void Window::dragEnterEvent(QDragEnterEvent* event)
{
	AddImage::dragEnterEvent(event);
}

//-----------------------------------------------------------------------------

void Window::dropEvent(QDropEvent* event)
{
	QStringList files = AddImage::dropEvent(event);
	if (!files.isEmpty()) {
		chooseGame(files);
	}
}

//-----------------------------------------------------------------------------

void Window::chooseGame(const QStringList& files)
{
	m_board->saveGame();

	ChooseGameDialog dialog(files, m_board->id(), this);
	connect(&dialog, &ChooseGameDialog::newGame, m_board, &Board::newGame);
	connect(&dialog, &ChooseGameDialog::openGame, m_board, &Board::openGame);
	if (dialog.exec() == QDialog::Accepted) {
		m_toggle_overview_action->setEnabled(true);
		m_zoom_fit_action->setEnabled(true);
		m_slider->show();
		m_completed->show();
	}
}

//-----------------------------------------------------------------------------

void Window::gameFinished()
{
	m_toggle_overview_action->setEnabled(false);
}

//-----------------------------------------------------------------------------

void Window::setFullScreen(bool enable)
{
	if (enable) {
		showFullScreen();
	} else {
		showNormal();
	}
}

//-----------------------------------------------------------------------------

void Window::setLocale()
{
	LocaleDialog dialog(this);
	dialog.exec();
}

//-----------------------------------------------------------------------------

void Window::showAppearance()
{
	AppearanceDialog dialog(this);
	if (dialog.exec() == QDialog::Accepted) {
		m_board->setAppearance(dialog);
	}
}

//-----------------------------------------------------------------------------

void Window::showControls()
{
	QDialog dialog(this);
	dialog.setWindowTitle(tr("Controls"));

	QGridLayout* layout = new QGridLayout(&dialog);
	layout->setMargin(12);
	layout->setSpacing(0);
	layout->setColumnMinimumWidth(1, 6);
	layout->setRowMinimumHeight(7, 12);
	layout->addWidget(new QLabel(tr("<b>Pick Up Pieces:</b>"), &dialog), 0, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Left Click or Space"), &dialog), 0, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Drop Pieces:</b>"), &dialog), 1, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Left Click or Space"), &dialog), 1, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Select Pieces:</b>"), &dialog), 2, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Left Drag"), &dialog), 2, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Rotate Pieces:</b>"), &dialog), 3, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Right Click, Control + Left Click, or R"), &dialog), 3, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Drag Puzzle:</b>"), &dialog), 4, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Middle Click, Shift + Left Click, or Arrows"), &dialog), 4, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Zoom Puzzle:</b>"), &dialog), 5, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Scrollwheel or +/-"), &dialog), 5, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Move Cursor:</b>"), &dialog), 6, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Move mouse or W,A,D,S"), &dialog), 6, 2, Qt::AlignLeft | Qt::AlignVCenter);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
	connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
	layout->addWidget(buttons, 8, 0, 1, 3);

	dialog.exec();
}

//-----------------------------------------------------------------------------

void Window::showAbout()
{
	QMessageBox::about(this, tr("About Tetzle"), QString("<p align='center'><big><b>%1 %2</b></big><br/>%3<br/><small>%4<br/>%5</small></p>")
		.arg(tr("Tetzle"), QCoreApplication::applicationVersion(),
			tr("A jigsaw puzzle with tetrominoes for pieces"),
			tr("Copyright &copy; 2008-%1 Graeme Gott").arg("2015"),
			tr("Released under the <a href=%1>GPL 3</a> license").arg("\"http://www.gnu.org/licenses/gpl.html\""))
	);
}

//-----------------------------------------------------------------------------
