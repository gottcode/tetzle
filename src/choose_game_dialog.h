/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_CHOOSE_GAME_DIALOG_H
#define TETZLE_CHOOSE_GAME_DIALOG_H

class NewGameTab;

#include <QDialog>
class QTabWidget;

class ChooseGameDialog : public QDialog
{
	Q_OBJECT

public:
	ChooseGameDialog(const QStringList& files, int current_id, QWidget* parent = nullptr);

	static QStringList currentGames();

Q_SIGNALS:
	void newGame(const QString& image, int difficulty);
	void openGame(int id);

protected:
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void hideEvent(QHideEvent* event) override;

private:
	QTabWidget* m_tabs;
	NewGameTab* m_new_game_tab;
};

#endif // TETZLE_CHOOSE_GAME_DIALOG_H
