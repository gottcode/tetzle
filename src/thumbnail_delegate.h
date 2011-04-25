/***********************************************************************
 *
 * Copyright (C) 2011 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#ifndef THUMBNAIL_DELEGATE_H
#define THUMBNAIL_DELEGATE_H

#include <QFontMetrics>
#include <QStyledItemDelegate>
class QListWidget;

class ThumbnailDelegate : public QStyledItemDelegate
{
public:
	ThumbnailDelegate(QListWidget* list = 0);

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

#endif
