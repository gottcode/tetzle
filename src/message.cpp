/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "message.h"

#include <QApplication>
#include <QPainter>
#include <QTimeLine>
#include <QTimer>
#include <QWidget>

//-----------------------------------------------------------------------------

Message::Message(QWidget* parent)
	: QObject(parent)
	, m_parent(parent)
	, m_visible(false)
	, m_color(Qt::white)
{
	m_hide_timer = new QTimer(this);
	m_hide_timer->setInterval(2000);
	m_hide_timer->setSingleShot(true);
	connect(m_hide_timer, &QTimer::timeout, this, &Message::hide);

	m_fade_timer = new QTimeLine(160, this);
	m_fade_timer->setEasingCurve(QEasingCurve::Linear);
	m_fade_timer->setDirection(QTimeLine::Backward);
	m_fade_timer->setFrameRange(0, 10);
	m_fade_timer->setUpdateInterval(16);
	connect(m_fade_timer, &QTimeLine::frameChanged, this, &Message::fade);
}

//-----------------------------------------------------------------------------

void Message::draw(QPainter& painter) const
{
	if (!m_visible && (m_fade_timer->state() == QTimeLine::NotRunning)) {
		return;
	}

	// Find message size
	const QFont font("Sans", 24);
	const QFontMetrics metrics(font);
	const int width = metrics.boundingRect(m_text).width();
	const int height = metrics.height();

	painter.save();
	painter.translate(m_parent->rect().center() - QRect(0, 0, width + height, height * 2).center());

	// Draw black background
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(0, 0, 0, 200));
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.drawRoundedRect(0, 0, width + height, height * 2, 10, 10);

	// Draw message
	painter.setFont(font);
	painter.setPen(Qt::white);
	painter.setRenderHint(QPainter::TextAntialiasing, true);
	painter.drawText(height / 2, height / 2 + metrics.ascent(), m_text);

	painter.restore();
}

//-----------------------------------------------------------------------------

void Message::setText(const QString& text)
{
	m_text = text;
}

//-----------------------------------------------------------------------------

void Message::setVisible(bool visible, bool stay)
{
	m_visible = visible;

	m_color.setAlpha(255);
	if (m_visible) {
		m_fade_timer->stop();
	} else {
		m_fade_timer->start();
	}

	if (m_visible && !stay) {
		m_hide_timer->start();
	} else {
		m_hide_timer->stop();
	}

	m_parent->update();
}

//-----------------------------------------------------------------------------

void Message::hide()
{
	setVisible(false);
}

//-----------------------------------------------------------------------------

void Message::show()
{
	setVisible(true);
}

//-----------------------------------------------------------------------------

void Message::fade(int frame)
{
	m_color.setAlpha(frame * 25);
	m_parent->update();
}

//-----------------------------------------------------------------------------
