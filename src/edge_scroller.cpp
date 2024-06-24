/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "edge_scroller.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

//-----------------------------------------------------------------------------

EdgeScroller::EdgeScroller(int horizontal, int vertical, QWidget* parent)
	: QWidget(parent)
	, m_horizontal(horizontal)
	, m_vertical(vertical)
	, m_speed(1)
	, m_hovered(false)
{
	m_timer = new QTimer(this);
	m_timer->setInterval(20);
	m_timer->setSingleShot(false);
	connect(m_timer, &QTimer::timeout, this, [this]() {
		Q_EMIT scroll(m_horizontal * m_speed, m_vertical * m_speed);
	});

	setCursor(horizontal ? Qt::SizeHorCursor : Qt::SizeVerCursor);

	if (horizontal > 0) {
		// Left
		m_arrow << QPoint(0, 10) << QPoint(10, 0) << QPoint(10, 20);
	} else if (horizontal < 0) {
		// Right
		m_arrow << QPoint(0, 0) << QPoint(10, 10) << QPoint(0, 20);
	} else if (vertical > 0 ) {
		// Up
		m_arrow << QPoint(0, 10) << QPoint(10, 0) << QPoint(20, 10);
	} else {
		// Down
		m_arrow << QPoint(0, 0) << QPoint(20, 0) << QPoint(10, 10);
	}
}

//-----------------------------------------------------------------------------

void EdgeScroller::enterEvent(QEnterEvent*)
{
	m_hovered = true;
	m_speed = 1;
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

void EdgeScroller::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_speed = 5;
	}
	event->accept();
}

//-----------------------------------------------------------------------------

void EdgeScroller::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		m_speed = 1;
	}
	event->accept();
}

//-----------------------------------------------------------------------------

void EdgeScroller::paintEvent(QPaintEvent*)
{
	QColor color = palette().highlight().color();

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);

	if (m_hovered) {
		color.setAlpha((m_speed > 1) ? 192 : 64);
		painter.setBrush(color);
		painter.drawRoundedRect(rect(), 3, 3);
	}

	color.setAlpha(m_hovered ? 255 : 96);
	painter.setBrush(color);
	painter.translate(rect().center() - m_arrow.boundingRect().center() + QPoint(1,1));
	painter.drawPolygon(m_arrow);
}

//-----------------------------------------------------------------------------
