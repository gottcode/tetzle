/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_OVERVIEW_H
#define TETZLE_OVERVIEW_H

#include <QGraphicsView>

/**
 * Window to show the game image as a guide.
 */
class Overview : public QGraphicsView
{
	Q_OBJECT

public:
	/**
	 * Construct an overview window.
	 *
	 * @param parent the parent widget of the overview
	 */
	explicit Overview(QWidget* parent = nullptr);

	/**
	 * Load an image.
	 *
	 * @param image the image to load
	 * @param pixelratio the pixel ratio to render at
	 */
	void load(const QImage& image, qreal pixelratio);

	/**
	 * Show loading image and reset zoom.
	 */
	void reset();

Q_SIGNALS:
	/**
	 * Signal that the visibility was changed.
	 *
	 * @param visible @c true if the overview is visible
	 */
	void toggled(bool visible);

protected:
	/**
	 * Emit toggled(false) as overview is hidden.
	 */
	void hideEvent(QHideEvent* event) override;

	/**
	 * Save window location.
	 *
	 * @note Restoring location does not work in Wayland.
	 */
	void moveEvent(QMoveEvent* event) override;

	/**
	 * Save window size and handle resizing viewport.
	 */
	void resizeEvent(QResizeEvent* event) override;

	/**
	 * Emit toggled(true) as overview is shown.
	 */
	void showEvent(QShowEvent* event) override;

	/**
	 * Allow player to zoom overview by scrolling.
	 */
	void wheelEvent(QWheelEvent* event) override;

private:
	/**
	 * Set the image of the overview.
	 *
	 * @param pixmap the pixmap to show
	 */
	void setPixmap(const QPixmap& pixmap);

	/**
	 * Increase the scale factor of the overview.
	 */
	void zoomIn();

	/**
	 * Decrease the scale factor of the overview.
	 */
	void zoomOut();

	/**
	 * Set the scale factor of the overview.
	 *
	 * @param level the zoom level to use
	 */
	void zoom(int level);

private:
	QGraphicsPixmapItem* m_pixmap; ///< image to show in overview
	int m_min_scale_level; ///< smallest zoom level
	int m_scale_level; ///< current zoom level
	bool m_default; ///< stores if the minimum size is the same as the viewport
};

#endif // TETZLE_OVERVIEW_H
