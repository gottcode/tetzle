/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_OVERVIEW_H
#define TETZLE_OVERVIEW_H

#include <QGraphicsView>

class Overview : public QGraphicsView
{
	Q_OBJECT

public:
	explicit Overview(QWidget* parent = nullptr);

	void load(const QImage& image, qreal pixelratio);
	void reset();

Q_SIGNALS:
	void toggled(bool visible);

protected:
	void hideEvent(QHideEvent* event) override;
	void moveEvent(QMoveEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void setPixmap(const QPixmap& pixmap);
	void zoomIn();
	void zoomOut();
	void zoom(int level);

private:
	QGraphicsPixmapItem* m_pixmap;
	int m_min_scale_level;
	int m_scale_level;
	bool m_default;
};

#endif // TETZLE_OVERVIEW_H
