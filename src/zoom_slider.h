/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_ZOOM_SLIDER_H
#define TETZLE_ZOOM_SLIDER_H

#include <QWidget>
class QSlider;
class QToolButton;

/**
 * Controls zoom level of board.
 */
class ZoomSlider : public QWidget
{
	Q_OBJECT

public:
	/**
	 * Construct zoom slider widget.
	 *
	 * @param parent the parent widget of the slider
	 */
	explicit ZoomSlider(QWidget* parent = nullptr);

	/**
	 * Fetch maximum zoom level.
	 *
	 * @return maximum zoom level
	 */
	static int maxScaleLevel()
	{
		return 9;
	}

	/**
	 * Fetch scale factor for zoom level.
	 *
	 * @return scale factor for zoom level
	 */
	static float scaleFactor(int level);

Q_SIGNALS:
	/**
	 * Signal that the zoom level has changed.
	 */
	void valueChanged(int);

	/**
	 * Signal to zoom board to best fit.
	 */
	void zoomFit();

	/**
	 * Signal to zoom in on board.
	 */
	void zoomIn();

	/**
	 * Signal to zoom out of board.
	 */
	void zoomOut();

	/**
	 * Signal if player can zoom in on board.
	 *
	 * @param available @c true if player can zoom in
	 */
	void zoomInAvailable(bool available);

	/**
	 * Signal if player can zoom out of board.
	 *
	 * @param available @c true if player can zoom out
	 */
	void zoomOutAvailable(bool available);

public Q_SLOTS:
	/**
	 * Set the zoom level.
	 *
	 * @param level the zoom level
	 */
	void setValue(int level);

private:
	QSlider* m_slider; ///< slider to control zoom
	QToolButton* m_zoom_in; ///< button to zoom in
	QToolButton* m_zoom_out; ///< button to zoom out
};

#endif // TETZLE_ZOOM_SLIDER_H
