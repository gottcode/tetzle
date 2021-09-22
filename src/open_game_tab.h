/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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

public slots:
	void imageRenamed(const QString& image, const QString& name);

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
