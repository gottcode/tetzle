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

#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
class QAction;
class QLabel;
class QSlider;
class Board;

class ZoomSlider : public QWidget
{
	Q_OBJECT
public:
	ZoomSlider(QWidget* parent = 0);

signals:
	void valueChanged(int);

public slots:
	void setValue(int value);
	void setRange(int min, int max);

private:
	QLabel* m_label;
	QSlider* m_slider;
};

class Window : public QMainWindow
{
	Q_OBJECT
public:
	Window();

protected:
	virtual void changeEvent(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);

private slots:
	void newGame();
	void openGame();
	void gameFinished();
	void showControls();
	void showAbout();

private:
	QAction* m_open_action;
	QAction* m_zoom_fit_action;
	ZoomSlider* m_slider;
	Board* m_board;
};

#endif // WINDOW_H
