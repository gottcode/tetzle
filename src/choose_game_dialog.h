/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef CHOOSE_GAME_DIALOG_H
#define CHOOSE_GAME_DIALOG_H

class NewGameTab;

#include <QDialog>
class QTabWidget;

class ChooseGameDialog : public QDialog
{
	Q_OBJECT

public:
	ChooseGameDialog(const QStringList& files, int current_id, QWidget* parent = 0);

	static QStringList currentGames();

signals:
	void newGame(const QString& image, int difficulty);
	void openGame(int id);

protected:
	virtual void dragEnterEvent(QDragEnterEvent* event);
	virtual void dropEvent(QDropEvent* event);
	virtual void hideEvent(QHideEvent* event);

private:
	QTabWidget* m_tabs;
	NewGameTab* m_new_game_tab;
};

#endif
