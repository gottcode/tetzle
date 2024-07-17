/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_EDGE_SCROLLER_H
#define TETZLE_EDGE_SCROLLER_H

#include <QPolygon>
#include <QWidget>
class QTimer;

/**
 * Widget to handle scrolling at edge of play area.
 */
class EdgeScroller : public QWidget
{
	Q_OBJECT

public:
	/**
	 * Construct an edge scroller widget.
	 *
	 * @param horizontal negative scrolls left, positive scrolls right
	 * @param vertical negative scrolls up, positive scrolls down
	 * @param parent the parent widget of the scroller
	 */
	explicit EdgeScroller(int horizontal, int vertical, QWidget* parent = nullptr);

Q_SIGNALS:
	/**
	 * Signal that the play area should scroll.
	 *
	 * @param horizontal how far to scroll horizontally
	 * @param vertical how far to scroll vertically
	 */
	void edgeScroll(int horizontal, int vertical);

protected:
	/**
	 * Handle mouse entering scroll area.
	 * Shows semi-transparent background and restarts scroll timer.
	 */
	void enterEvent(QEnterEvent*) override;

	/**
	 * Handle mouse leaving scroll area. Stops scroll timer.
	 */
	void leaveEvent(QEvent*) override;

	/**
	 * Accelerate scrolling if player has pressed left button.
	 */
	void mousePressEvent(QMouseEvent* event) override;

	/**
	 * Resume slower scrolling.
	 */
	void mouseReleaseEvent(QMouseEvent* event) override;

	/**
	 * Draw scroller. Draws background if mouse is over scroller,
	 * and draws the background brighter if the mouse is pressed.
	 */
	void paintEvent(QPaintEvent*) override;

private:
	QTimer* m_scroll_timer; ///< the timer to emit scroll events
	QTimer* m_start_timer; ///< the timer for a delay before it starts scrolling
	QPolygon m_arrow; ///< the arrow to draw in the middle of the scroller
	const int m_horizontal; ///< which direction to scroll horizontally
	const int m_vertical; ///< which direction to scroll vertically
	int m_speed; ///< how fast to scroll
	bool m_hovered; ///< if the mouse is over the scroller
};

#endif // TETZLE_EDGE_SCROLLER_H
