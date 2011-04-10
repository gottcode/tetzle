/***********************************************************************
 *
 * Copyright (C) 2011 Graeme Gott <graeme@gottcode.org>
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
