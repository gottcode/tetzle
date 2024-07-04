/*
	SPDX-FileCopyrightText: 2011-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "toolbar_list.h"

#include <QToolBar>

//-----------------------------------------------------------------------------

ToolBarList::ToolBarList(QWidget* parent)
	: QListWidget(parent)
{
	m_toolbar = new QToolBar(this);
	m_toolbar->setFloatable(false);
	m_toolbar->setMovable(false);
	m_toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	m_toolbar->setStyleSheet("QToolBar { border-top: 1px solid palette(mid); }");
	m_toolbar->hide();

	setContextMenuPolicy(Qt::ActionsContextMenu);
	setMovement(QListView::Static);
	setResizeMode(QListView::Adjust);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
}

//-----------------------------------------------------------------------------

void ToolBarList::addToolBarAction(QAction* action)
{
	if (action) {
		addAction(action);
		m_toolbar->addAction(action);
		m_toolbar->show();
		updateGeometries();
	}
}

//-----------------------------------------------------------------------------

void ToolBarList::addToolBarWidget(QWidget* widget)
{
	if (widget) {
		m_toolbar->addWidget(widget);
		m_toolbar->show();
		updateGeometries();
	}
}

//-----------------------------------------------------------------------------

void ToolBarList::updateGeometries()
{
	// Resize viewport margins
	QSize hint = m_toolbar->isHidden() ? QSize(0,0) : m_toolbar->sizeHint();
	setViewportMargins(0, 0, 0, hint.height());

	// Resize toolbar
	QRect rect = viewport()->geometry();
	QRect geometry(rect.left(), rect.top() + rect.height(), rect.width(), hint.height());
	m_toolbar->setGeometry(geometry);

	QListWidget::updateGeometries();
}

//-----------------------------------------------------------------------------
