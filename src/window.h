/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_WINDOW_H
#define TETZLE_WINDOW_H

class Board;
class ZoomSlider;

#include <QMainWindow>
class QAction;
class QProgressBar;

class Window : public QMainWindow
{
	Q_OBJECT

public:
	explicit Window(const QStringList& files = QStringList());

	void addImages(const QStringList& files);

protected:
	void changeEvent(QEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

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

#endif // TETZLE_WINDOW_H
