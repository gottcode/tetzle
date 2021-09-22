/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef OVERVIEW_H
#define OVERVIEW_H

#include <QGraphicsView>

class Overview : public QGraphicsView
{
	Q_OBJECT
public:
	Overview(QWidget* parent = 0);

	void load(const QImage& image, qreal pixelratio);
	void reset();

signals:
	void toggled(bool visible);

protected:
	virtual void hideEvent(QHideEvent* event);
	virtual void moveEvent(QMoveEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void showEvent(QShowEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

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

#endif
