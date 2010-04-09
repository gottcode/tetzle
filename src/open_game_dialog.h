/***********************************************************************
 *
 * Copyright (C) 2008, 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef OPEN_GAME_DIALOG_H
#define OPEN_GAME_DIALOG_H

#include <QDialog>
class QListWidget;
class QPushButton;
class ThumbnailList;

class OpenGameDialog : public QDialog
{
	Q_OBJECT
public:
	OpenGameDialog(int current_id, QWidget* parent = 0);

public slots:
	virtual void accept();

signals:
	void newGame();
	void openGame(int id);

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void deleteGame();

private:
	QListWidget* m_games;
	ThumbnailList* m_thumbnails;
	QPushButton* m_accept_button;
};

#endif
