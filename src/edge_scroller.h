/*
	SPDX-FileCopyrightText: 2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_EDGE_SCROLLER_H
#define TETZLE_EDGE_SCROLLER_H

#include <QWidget>
class QTimer;

class EdgeScroller : public QWidget
{
	Q_OBJECT

public:
	explicit EdgeScroller(int horizontal, int vertical, QWidget* parent = nullptr);

Q_SIGNALS:
	void scroll(int horizontal, int vertical);

protected:
	void enterEvent(QEnterEvent*) override;
	void leaveEvent(QEvent*) override;
	void paintEvent(QPaintEvent*) override;

private:
	QTimer* m_timer;
	const int m_horizontal;
	const int m_vertical;
	bool m_hovered;
};

#endif // TETZLE_EDGE_SCROLLER_H
