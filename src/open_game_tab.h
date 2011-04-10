/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#ifndef OPEN_GAME_TAB_H
#define OPEN_GAME_TAB_H

#include <QWidget>
class QListWidget;
class QPushButton;

class OpenGameTab : public QWidget
{
	Q_OBJECT

public:
	OpenGameTab(int current_id, QDialog* parent = 0);

signals:
	void openGame(int id);

private slots:
	void accept();
	void deleteGame();

private:
	QDialog* m_parent;
	QListWidget* m_games;
	QPushButton* m_accept_button;
};

#endif
