/*
	SPDX-FileCopyrightText: 2011-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_TOOLBAR_LIST_H
#define TETZLE_TOOLBAR_LIST_H

#include <QListWidget>
class QToolBar;

class ToolBarList : public QListWidget
{
public:
	explicit ToolBarList(QWidget* parent = nullptr);

	void addToolBarAction(QAction* action);
	void addToolBarWidget(QWidget* widget);

protected:
	void updateGeometries() override;

private:
	QToolBar* m_toolbar;
};

#endif // TETZLE_TOOLBAR_LIST_H
