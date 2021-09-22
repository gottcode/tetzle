/*
	SPDX-FileCopyrightText: 2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_THUMBNAIL_DELEGATE_H
#define TETZLE_THUMBNAIL_DELEGATE_H

#include <QFontMetrics>
#include <QStyledItemDelegate>
class QListWidget;

class ThumbnailDelegate : public QStyledItemDelegate
{
public:
	explicit ThumbnailDelegate(QListWidget* list = 0);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

protected:
	virtual bool eventFilter(QObject* object, QEvent* event);

private:
	void setFont(const QFont& font);

private:
	QListWidget* m_list;
	QFont m_small_font;
	QFontMetrics m_small_font_metrics;
};

#endif // TETZLE_THUMBNAIL_DELEGATE_H
