/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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

	void addImages(const QStringList& files);

protected:
	virtual void changeEvent(QEvent* event);
	virtual void closeEvent(QCloseEvent* event);
	virtual void dragEnterEvent(QDragEnterEvent* event);
	virtual void dropEvent(QDropEvent* event);

private slots:
	void chooseGame(const QStringList& files = QStringList());
	void gameFinished();
	void setFullScreen(bool enable);
	void setLocale();
	void showAppearance();
	void showControls();
	void showAbout();

private:
	QAction* m_zoom_fit_action;
	QAction* m_toggle_overview_action;
	ZoomSlider* m_slider;
	QProgressBar* m_completed;
	Board* m_board;
};

#endif
