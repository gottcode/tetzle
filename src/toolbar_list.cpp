/*
	SPDX-FileCopyrightText: 2011-2016 Graeme Gott <graeme@gottcode.org>

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
	m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
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

QIcon ToolBarList::fetchIcon(const QString& name)
{
	QIcon icon(QString(":/tango/64x64/%1.png").arg(name));
	icon.addFile(QString(":/tango/48x48/%1.png").arg(name));
	icon.addFile(QString(":/tango/32x32/%1.png").arg(name));
	icon.addFile(QString(":/tango/24x24/%1.png").arg(name));
	icon.addFile(QString(":/tango/22x22/%1.png").arg(name));
	icon.addFile(QString(":/tango/16x16/%1.png").arg(name));
	return QIcon::fromTheme(name, icon);
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
