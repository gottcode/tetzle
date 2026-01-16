/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_WINDOW_H
#define TETZLE_WINDOW_H

class Board;
class ZoomSlider;

#include <QMainWindow>
class QAction;
class QProgressBar;

/**
 * Main window for game.
 */
class Window : public QMainWindow
{
	Q_OBJECT

public:
	/**
	 * Construct a main window.
	 *
	 * @param files a list of images to add for new games
	 */
	explicit Window(const QStringList& files = QStringList());

	/**
	 * Add a list of images for new games.
	 *
	 * @param files a list of images to add for new games
	 */
	void addImages(const QStringList& files);

protected:
	/**
	 * Save current game on close, as well as window state.
	 *
	 * @note Does note save window state in Wayland.
	 */
	void closeEvent(QCloseEvent* event) override;

	/**
	 * Handle player dragging over window. This allows them to add images.
	 */
	void dragEnterEvent(QDragEnterEvent* event) override;

	/**
	 * Handle player dropping over window. This allows them to add images.
	 */
	void dropEvent(QDropEvent* event) override;

private Q_SLOTS:
	/**
	 * Start a new game.
	 *
	 * @param files a list of images to add for new game
	 */
	void chooseGame(const QStringList& files);

	/**
	 * Handle game finishing.
	 */
	void gameFinished();

	/**
	 * Set the window fullscreen.
	 *
	 * @param enable @c true if it is fullscreen
	 */
	void setFullScreen(bool enable);

	/**
	 * Prompt player to change locale.
	 */
	void setLocale();

	/**
	 * Prompt player to change board appearance settings.
	 */
	void showAppearance();

	/**
	 * Show player description of controls.
	 */
	void showControls();

	/**
	 * Show player message about program.
	 */
	void showAbout();

private:
	QAction* m_zoom_fit_action; ///< action to zoom board to best fit
	QAction* m_toggle_overview_action; ///< action to show or hide overview
	ZoomSlider* m_slider; ///< slider controlling zoom of board
	QProgressBar* m_completed; ///< progressbar showing completion status of game
	Board* m_board; ///< play area of game
};

#endif // TETZLE_WINDOW_H
