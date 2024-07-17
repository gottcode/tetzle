/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_CHOOSE_GAME_DIALOG_H
#define TETZLE_CHOOSE_GAME_DIALOG_H

class NewGameTab;

#include <QDialog>
class QTabWidget;

/**
 * Dialog to choose game.
 */
class ChooseGameDialog : public QDialog
{
	Q_OBJECT

public:
	/**
	 * Construct a choose game dialog populated with current games.
	 *
	 * @param files images to add to the new game tab
	 * @param current_id the current game in progress
	 * @param parent the parent widget of the dialog
	 */
	ChooseGameDialog(const QStringList& files, int current_id, QWidget* parent = nullptr);

	/**
	 * Fetch list of current games.
	 *
	 * @return list of current games
	 */
	static QStringList currentGames();

Q_SIGNALS:
	/**
	 * Signal to start a new game.
	 *
	 * @param image the image to use on the pieces
	 * @param difficulty how many pieces to generate
	 */
	void newGame(const QString& image, int difficulty);

	/**
	 * Signal to start a previous game.
	 *
	 * @param id the game to start
	 */
	void openGame(int id);

protected:
	/**
	 * Handle player dragging over dialog. This allows them to add images.
	 */
	void dragEnterEvent(QDragEnterEvent* event) override;

	/**
	 * Handle player dropping over dialog. This allows them to add images.
	 */
	void dropEvent(QDropEvent* event) override;

	/**
	 * Store the size of the dialog for next use.
	 */
	void hideEvent(QHideEvent* event) override;

private:
	QTabWidget* m_tabs; ///< the tabs for current and new games
	NewGameTab* m_new_game_tab; ///< the contents of the new game tab
};

#endif // TETZLE_CHOOSE_GAME_DIALOG_H
