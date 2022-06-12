/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_OPEN_GAME_TAB_H
#define TETZLE_OPEN_GAME_TAB_H

#include <QWidget>
class QListWidget;
class QPushButton;

class OpenGameTab : public QWidget
{
	Q_OBJECT

public:
	explicit OpenGameTab(int current_id, QDialog* parent = nullptr);

public Q_SLOTS:
	void imageRenamed(const QString& image, const QString& name);

Q_SIGNALS:
	void openGame(int id);

private Q_SLOTS:
	void accept();
	void deleteGame();

private:
	QDialog* m_parent;
	QListWidget* m_games;
	QPushButton* m_accept_button;
};

#endif // TETZLE_OPEN_GAME_TAB_H
