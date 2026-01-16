/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_TOOLBAR_LIST_H
#define TETZLE_TOOLBAR_LIST_H

#include <QListWidget>
class QToolBar;

/**
 * List widget with toolbar on top.
 */
class ToolBarList : public QListWidget
{
public:
	/**
	 * Construct a listwidget.
	 *
	 * @param parent the parent widget of the listwidget
	 */
	explicit ToolBarList(QWidget* parent = nullptr);

	/**
	 * Add an action to toolbar.
	 *
	 * @param action the action to add
	 */
	void addToolBarAction(QAction* action);

	/**
	 * Add a widget to toolbar.
	 *
	 * @param widget the widget to add
	 */
	void addToolBarWidget(QWidget* widget);

protected:
	/**
	 * Resize toolbar to fill top of viewport.
	 */
	void updateGeometries() override;

private:
	QToolBar* m_toolbar; ///< toolbar for list widget
};

#endif // TETZLE_TOOLBAR_LIST_H
