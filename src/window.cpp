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

#include "window.h"

#include "board.h"
#include "new_game_dialog.h"
#include "open_game_dialog.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>

/*****************************************************************************/

ZoomSlider::ZoomSlider(QWidget* parent)
:	QWidget(parent)
{
	m_label = new QLabel(tr("0%"), this);
	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	connect(m_slider, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(10, 0, 10, 0);
	layout->addWidget(m_label);
	layout->addWidget(m_slider);
}

/*****************************************************************************/

void ZoomSlider::setValue(int value)
{
	m_slider->setValue(value);
	m_label->setText(tr("%1%").arg(value * 100 / m_slider->maximum()));
}

/*****************************************************************************/

void ZoomSlider::setRange(int min, int max)
{
	m_slider->setRange(min, max);
}

/*****************************************************************************/

Window::Window()
:	m_board(0)
{
	setWindowTitle(tr("Tetzle"));
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	setWindowIcon(QIcon(":/tetzle.png"));
#endif
	resize(640, 480);

	// Add statusbar
	m_slider = new ZoomSlider(this);
	m_slider->setVisible(false);
	statusBar()->addPermanentWidget(m_slider);

	QLabel* status_message = new QLabel(this);
	status_message->setContentsMargins(10, 0, 10, 0);
	statusBar()->addPermanentWidget(status_message);

	// Add contents
	m_board = new Board(this);
	connect(m_board, SIGNAL(statusMessage(const QString&)), status_message, SLOT(setText(const QString&)));
	connect(m_board, SIGNAL(overviewShown()), this, SLOT(overviewShown()));
	connect(m_board, SIGNAL(overviewHidden()), this, SLOT(overviewHidden()));
	connect(m_board, SIGNAL(finished()), this, SLOT(gameFinished()));
	connect(m_board, SIGNAL(zoomChanged(int)), m_slider, SLOT(setValue(int)));
	connect(m_board, SIGNAL(zoomRangeChanged(int, int)), m_slider, SLOT(setRange(int, int)));
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
	m_show_overview_action = menu->addAction(tr("&Show Overview"), m_board, SLOT(showOverview()), tr("Tab"));
	m_show_overview_action->setEnabled(false);
	m_hide_overview_action = menu->addAction(tr("&Hide Overview"), m_board, SLOT(hideOverview()), tr("Tab"));
	m_hide_overview_action->setVisible(false);

	menu->addSeparator();

	QAction* fullscreen_action = menu->addAction(tr("Fullscreen"));
	connect(fullscreen_action, SIGNAL(toggled(bool)), this, SLOT(setFullScreen(bool)));
	fullscreen_action->setCheckable(true);
#if !defined(Q_OS_MAC)
	fullscreen_action->setShortcut(tr("F11"));
#else
	fullscreen_action->setShortcut(tr("Ctrl+F"));
#endif

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
	if (QDir("saves/", "*.xml").count()) {
		m_open_action->setEnabled(true);
		openGame();
	} else {
		newGame();
	}

	// Create auto-save timer
	QTimer* timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), m_board, SLOT(saveGame()));
	timer->start(300000);
}

/*****************************************************************************/

void Window::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::WindowStateChange)
		m_board->updateGL();
	return QMainWindow::changeEvent(event);
}

/*****************************************************************************/

void Window::closeEvent(QCloseEvent* event)
{
	QSettings().setValue("Geometry", saveGeometry());

	m_board->saveGame();

	QMainWindow::closeEvent(event);
}

/*****************************************************************************/

void Window::newGame()
{
	m_board->saveGame();

	NewGameDialog dialog(this);
	connect(&dialog, SIGNAL(accepted()), m_slider, SLOT(hide()));
	connect(&dialog, SIGNAL(newGame(const QString&, int)), m_board, SLOT(newGame(const QString&, int)));
	if (dialog.exec() == QDialog::Accepted) {
		m_open_action->setEnabled(QDir("saves/", "*.xml").count() > 0);
		m_show_overview_action->setEnabled(true);
		m_zoom_fit_action->setEnabled(true);
		m_slider->setVisible(true);
	}
}

/*****************************************************************************/

void Window::openGame()
{
	m_board->saveGame();

	OpenGameDialog dialog(m_board->id(), this);
	connect(&dialog, SIGNAL(accepted()), m_slider, SLOT(hide()));
	connect(&dialog, SIGNAL(newGame()), this, SLOT(newGame()));
	connect(&dialog, SIGNAL(openGame(int)), m_board, SLOT(openGame(int)));
	if (dialog.exec() == QDialog::Accepted) {
		m_open_action->setEnabled(QDir("saves/", "*.xml").count() > 1);
		m_show_overview_action->setEnabled(true);
		m_zoom_fit_action->setEnabled(true);
		m_slider->setVisible(true);
	} else {
		m_open_action->setEnabled(QDir("saves/", "*.xml").count() > 0);
	}
}

/*****************************************************************************/

void Window::gameFinished()
{
	m_show_overview_action->setEnabled(false);
	m_open_action->setEnabled(QDir("saves/", "*.xml").count() > 0);
}

/*****************************************************************************/

void Window::overviewShown()
{
	m_hide_overview_action->setVisible(true);
	m_show_overview_action->setVisible(false);
}

/*****************************************************************************/

void Window::overviewHidden()
{
	m_show_overview_action->setVisible(true);
	m_hide_overview_action->setVisible(false);
}

/*****************************************************************************/

void Window::setFullScreen(bool enable)
{
	if (enable)
		showFullScreen();
	else
		showNormal();
}

/*****************************************************************************/

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
	layout->addWidget(new QLabel(tr("Left Click"), &dialog), 0, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Drop Pieces:</b>"), &dialog), 1, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Left Click"), &dialog), 1, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Select Pieces:</b>"), &dialog), 2, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Left Drag"), &dialog), 2, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Rotate Pieces:</b>"), &dialog), 3, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Right Click or Control + Left Click"), &dialog), 3, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Drag Puzzle:</b>"), &dialog), 4, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Middle Click or Shift + Left Click"), &dialog), 4, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Zoom Puzzle:</b>"), &dialog), 5, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Scrollwheel or +/-"), &dialog), 5, 2, Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("<b>Toggle Overview:</b>"), &dialog), 6, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(new QLabel(tr("Tab"), &dialog), 6, 2, Qt::AlignLeft | Qt::AlignVCenter);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
	connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
	layout->addWidget(buttons, 8, 0, 1, 3);

	dialog.exec();
}

/*****************************************************************************/

void Window::showAbout()
{
	QMessageBox::about(this, tr("About"), tr("<center><big><b>Tetzle 1.1.0</b></big><br/>A jigsaw puzzle with tetrominoes for pieces<br/><small>Copyright &copy; 2008 Graeme Gott</small></center>"));
}

/*****************************************************************************/
