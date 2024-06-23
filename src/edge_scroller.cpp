/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "edge_scroller.h"

#include <QPainter>
#include <QTimer>

//-----------------------------------------------------------------------------

EdgeScroller::EdgeScroller(int horizontal, int vertical, QWidget* parent)
	: QWidget(parent)
	, m_horizontal(horizontal)
	, m_vertical(vertical)
	, m_hovered(false)
{
	m_timer = new QTimer(this);
	m_timer->setInterval(20);
	m_timer->setSingleShot(false);
	connect(m_timer, &QTimer::timeout, this, [this]() {
		Q_EMIT scroll(m_horizontal, m_vertical);
	});
}

//-----------------------------------------------------------------------------

void EdgeScroller::enterEvent(QEnterEvent*)
{
	m_hovered = true;
	m_timer->start();
	update();
}

//-----------------------------------------------------------------------------

void EdgeScroller::leaveEvent(QEvent*)
{
	m_hovered = false;
	m_timer->stop();
	update();
}

//-----------------------------------------------------------------------------

void EdgeScroller::paintEvent(QPaintEvent*)
{
	QColor color = palette().highlight().color();

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);

	if (m_hovered) {
		color.setAlpha(64);
		painter.setBrush(color);
		painter.drawRoundedRect(rect(), 3, 3);
	}
}

//-----------------------------------------------------------------------------
