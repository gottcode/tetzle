/*
	SPDX-FileCopyrightText: 2011-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TOOLBAR_LIST_H
#define TOOLBAR_LIST_H

#include <QListWidget>
class QToolBar;

class ToolBarList : public QListWidget
{
public:
	ToolBarList(QWidget* parent = 0);

	void addToolBarAction(QAction* action);
	QIcon fetchIcon(const QString& name);

protected:
	virtual void updateGeometries();

private:
	QToolBar* m_toolbar;
};

#endif
