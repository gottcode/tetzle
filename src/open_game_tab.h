/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_OPEN_GAME_TAB_H
#define TETZLE_OPEN_GAME_TAB_H

#include <QWidget>
class QListWidget;
class QPushButton;

/**
 * Tab to choose which saved game to open.
 */
class OpenGameTab : public QWidget
{
	Q_OBJECT

public:
	/**
	 * Construct an open game tab.
	 *
	 * @param current_id the currently open game to exclude
	 * @param parent the parent widget of the tab
	 */
	explicit OpenGameTab(int current_id, QDialog* parent = nullptr);

public Q_SLOTS:
	/**
	 * Handle image being renamed.
	 *
	 * @param image the image identifier
	 * @param name the new image name
	 */
	void imageRenamed(const QString& image, const QString& name);

Q_SIGNALS:
	/**
	 * Signal to start saved game.
	 *
	 * @param id the game to start
	 */
	void openGame(int id);

protected:
	/**
	 * Unset default button.
	 */
	void hideEvent(QHideEvent* event) override;

	/**
	 * Set default button to #m_accept_button.
	 */
	void showEvent(QShowEvent* event) override;

private Q_SLOTS:
	/**
	 * Start a saved game.
	 */
	void accept();

	/**
	 * Prompt player to delete a saved game.
	 */
	void deleteGame();

private:
	QListWidget* m_games; ///< list of saved games
	QPushButton* m_accept_button; ///< button to start saved game
};

#endif // TETZLE_OPEN_GAME_TAB_H
