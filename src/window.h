/***********************************************************************
 *
 * Copyright (C) 2008, 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
class QAction;
class QProgressBar;
class Board;
class ZoomSlider;

class Window : public QMainWindow
{
	Q_OBJECT
public:
	Window(const QStringList& files = QStringList());

protected:
	virtual void changeEvent(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
	virtual void dragEnterEvent(QDragEnterEvent* event);
	virtual void dropEvent(QDropEvent* event);

private slots:
	void newGame(const QStringList& files = QStringList());
	void openGame();
	void gameFinished();
	void overviewToggled(bool visible);
	void setFullScreen(bool enable);
	void showControls();
	void showAbout();

private:
	QAction* m_open_action;
	QAction* m_zoom_fit_action;
	QAction* m_toggle_overview_action;
	ZoomSlider* m_slider;
	QProgressBar* m_completed;
	Board* m_board;
};

#endif
