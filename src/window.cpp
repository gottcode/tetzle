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

#include "window.h"

#include "add_image.h"
#include "appearance_dialog.h"
#include "board.h"
#include "new_game_dialog.h"
#include "open_game_dialog.h"
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
	setWindowTitle(tr("Tetzle"));
	setWindowIcon(QIcon(":/tetzle.png"));
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
	connect(m_board, SIGNAL(completionChanged(int)), m_completed, SLOT(setValue(int)));
	connect(m_board, SIGNAL(overviewToggled(bool)), this, SLOT(overviewToggled(bool)));
	connect(m_board, SIGNAL(finished()), this, SLOT(gameFinished()));
	connect(m_board, SIGNAL(clearMessage()), statusBar(), SLOT(clearMessage()));
	connect(m_board, SIGNAL(showMessage(const QString&)), statusBar(), SLOT(showMessage(const QString&)));
	connect(m_board, SIGNAL(zoomChanged(int, float)), m_slider, SLOT(setValue(int, float)));
	connect(m_slider, SIGNAL(valueChanged(int)), m_board, SLOT(zoom(int)));
	setCentralWidget(m_board);

	// Add menus
	QMenu* menu;

	menu = menuBar()->addMenu(tr("&Game"));
	menu->addAction(tr("&New"), this, SLOT(newGame()), tr("Ctrl+N"));
	m_open_action = menu->addAction(tr("&Open"), this, SLOT(openGame()), tr("Ctrl+O"));
	menu->addSeparator();
	QAction* retrieve_pieces_action = menu->addAction(tr("&Retrieve Pieces"), m_board, SLOT(retrievePieces()), tr("Ctrl+R"));
	retrieve_pieces_action->setEnabled(false);
	connect(m_board, SIGNAL(retrievePiecesAvailable(bool)), retrieve_pieces_action, SLOT(setEnabled(bool)));
	menu->addSeparator();
	menu->addAction(tr("&Quit"), this, SLOT(close()), tr("Ctrl+Q"));

	menu = menuBar()->addMenu(tr("&View"));
	QAction* zoom_in_action = menu->addAction(tr("Zoom &In"), m_board, SLOT(zoomIn()), tr("+"));
	zoom_in_action->setEnabled(false);
	connect(m_board, SIGNAL(zoomInAvailable(bool)), zoom_in_action, SLOT(setEnabled(bool)));
	QAction* zoom_out_action = menu->addAction(tr("Zoom &Out"), m_board, SLOT(zoomOut()), tr("-"));
	zoom_out_action->setEnabled(false);
	connect(m_board, SIGNAL(zoomOutAvailable(bool)), zoom_out_action, SLOT(setEnabled(bool)));
	m_zoom_fit_action = menu->addAction(tr("Best &Fit"), m_board, SLOT(zoomFit()));
	m_zoom_fit_action->setEnabled(false);
	menu->addSeparator();
	m_toggle_overview_action = menu->addAction(tr("Show O&verview"), m_board, SLOT(toggleOverview()), tr("Tab"));
	m_toggle_overview_action->setEnabled(false);
	menu->addSeparator();

	QAction* fullscreen_action = menu->addAction(tr("Fullscreen"));
	connect(fullscreen_action, SIGNAL(toggled(bool)), this, SLOT(setFullScreen(bool)));
	fullscreen_action->setCheckable(true);
#if !defined(Q_OS_MAC)
	fullscreen_action->setShortcut(tr("F11"));
#else
	fullscreen_action->setShortcut(tr("Ctrl+F"));
#endif
	menu->addSeparator();
	menu->addAction(tr("Appearance"), this, SLOT(showAppearance()));

	menu = menuBar()->addMenu(tr("&Help"));
	menu->addAction(tr("&Controls"), this, SLOT(showControls()));
	menu->addSeparator();
	menu->addAction(tr("&About"), this, SLOT(showAbout()));
	menu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));

	// Restore geometry
	QSettings settings;
	if (settings.contains("Geometry")) {
		restoreGeometry(settings.value("Geometry").toByteArray());
	} else {
		resize(settings.value("Size", QSize(640, 480)).toSize());
		settings.remove("Size");
	}

	// Start or load a game
	show();
	m_open_action->setEnabled(!OpenGameDialog::games().isEmpty());
	if (files.isEmpty() && m_open_action->isEnabled()) {
		openGame();
	} else {
		newGame(files);
	}

	// Create auto-save timer
	QTimer* timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), m_board, SLOT(saveGame()));
	timer->start(300000);
}

//-----------------------------------------------------------------------------

void Window::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::WindowStateChange) {
		m_board->updateGL();
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
		newGame(files);
	}
}

//-----------------------------------------------------------------------------

void Window::newGame(const QStringList& files)
{
	m_board->saveGame();

	NewGameDialog dialog(files, this);
	connect(&dialog, SIGNAL(accepted()), m_slider, SLOT(hide()));
	connect(&dialog, SIGNAL(accepted()), m_completed, SLOT(hide()));
	connect(&dialog, SIGNAL(newGame(const QString&, int)), m_board, SLOT(newGame(const QString&, int)));
	if (dialog.exec() == QDialog::Accepted) {
		m_open_action->setEnabled(!OpenGameDialog::games().isEmpty());
		m_toggle_overview_action->setEnabled(true);
		m_zoom_fit_action->setEnabled(true);
		m_slider->show();
		m_completed->show();
	}
}

//-----------------------------------------------------------------------------

void Window::openGame()
{
	m_board->saveGame();

	OpenGameDialog dialog(m_board->id(), this);
	connect(&dialog, SIGNAL(accepted()), m_slider, SLOT(hide()));
	connect(&dialog, SIGNAL(accepted()), m_completed, SLOT(hide()));
	connect(&dialog, SIGNAL(newGame(const QStringList&)), this, SLOT(newGame(const QStringList&)));
	connect(&dialog, SIGNAL(openGame(int)), m_board, SLOT(openGame(int)));
	if (dialog.exec() == QDialog::Accepted) {
		m_open_action->setEnabled(OpenGameDialog::games().count() > 1);
		m_toggle_overview_action->setEnabled(true);
		m_zoom_fit_action->setEnabled(true);
		m_slider->show();
		m_completed->show();
	} else {
		m_open_action->setEnabled(!OpenGameDialog::games().isEmpty());
	}
}

//-----------------------------------------------------------------------------

void Window::gameFinished()
{
	m_toggle_overview_action->setEnabled(false);
	m_open_action->setEnabled(!OpenGameDialog::games().isEmpty());
}

//-----------------------------------------------------------------------------

void Window::overviewToggled(bool visible)
{
	if (visible) {
		m_toggle_overview_action->setText(tr("Hide O&verview"));
	} else {
		m_toggle_overview_action->setText(tr("Show O&verview"));
	}
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

void Window::showAppearance()
{
	AppearanceDialog dialog(this);
	if (dialog.exec() == QDialog::Accepted) {
		m_board->setColors(dialog.colors());
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
	QMessageBox::about(this, tr("About"), tr(
		"<center><big><b>Tetzle 1.2.1</b></big><br/>"
		"A jigsaw puzzle with tetrominoes for pieces<br/>"
		"<small>Copyright &copy; 2008-2010 Graeme Gott</small></center>"
	));
}

//-----------------------------------------------------------------------------
