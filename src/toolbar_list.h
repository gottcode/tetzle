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

#ifndef TOOLBAR_LIST_H
#define TOOLBAR_LIST_H

#include <QListWidget>
class QToolBar;

class ToolBarList : public QListWidget
{
public:
	ToolBarList(QWidget* parent = 0);

	void addToolBarAction(QAction* action);

protected:
	virtual void updateGeometries();

private:
	QToolBar* m_toolbar;
};

#endif
