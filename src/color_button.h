/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_COLOR_BUTTON_H
#define TETZLE_COLOR_BUTTON_H

#include <QColor>
#include <QPushButton>

/**
 * Button to choose color.
 */
class ColorButton : public QPushButton
{
	Q_OBJECT

public:
	/**
	 * Construct a button for choosing a color.
	 *
	 * @param parent the parent widget of the button
	 */
	explicit ColorButton(QWidget* parent = nullptr);

	/**
	 * Fetch the color chosen by the player.
	 *
	 * @return the color as a QColor
	 */
	QColor color() const
	{
		return m_color;
	}

	/**
	 * Fetch the color chosen by the player.
	 *
	 * @return the color as a QString
	 */
	QString toString() const
	{
		return m_color.name();
	}

public Q_SLOTS:
	/**
	 * Set the color of the button.
	 *
	 * @param color the color to use for the button
	 */
	void setColor(const QColor& color);

Q_SIGNALS:
	/**
	 * Signal that the button color has changed.
	 *
	 * @param color the new color of the button
	 */
	void changed(const QColor& color);

private Q_SLOTS:
	/**
	 * Prompt player for new color.
	 */
	void chooseColor();

private:
	QColor m_color; ///< the color of the button
};

#endif // TETZLE_COLOR_BUTTON_H
