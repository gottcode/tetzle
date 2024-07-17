/*
	SPDX-FileCopyrightText: 2010-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_MESSAGE_H
#define TETZLE_MESSAGE_H

#include <QColor>
#include <QObject>
class QPainter;
class QTimeLine;
class QTimer;
class QWidget;

/**
 * Overlay to draw messages on board.
 */
class Message : public QObject
{
	Q_OBJECT

public:
	/**
	 * Construct an overlay message.
	 *
	 * @param parent the parent widget of the message
	 */
	explicit Message(QWidget* parent);

	/**
	 * Draw overlay message.
	 */
	void draw(QPainter& painter) const;

	/**
	 * Set the overlay message.
	 *
	 * @param text the message
	 */
	void setText(const QString& text);

	/**
	 * Set if the overlay message is visible.
	 *
	 * @param visible @c true if message is visible
	 * @param stay @c true if it should not fade after 2 seconds
	 */
	void setVisible(bool visible, bool stay = true);

public Q_SLOTS:
	/**
	 * Hide the overlay message.
	 */
	void hide();

	/**
	 * Show the overlay message.
	 */
	void show();

private Q_SLOTS:
	/**
	 * Fade the overlay message.
	 *
	 * @param frame how far through the fade
	 */
	void fade(int frame);

private:
	QWidget* m_parent; ///< parent widget of message
	QTimer* m_hide_timer; ///< timer controlling when to hide message
	QTimeLine* m_fade_timer; ///< timer controlling how long it fades

	QString m_text; ///< text of message
	bool m_visible; ///< if message is visible

	qreal m_opacity; ///< opacity of message, used to fade out
};

#endif // TETZLE_MESSAGE_H
