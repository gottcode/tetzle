/*
	SPDX-FileCopyrightText: 2011-2015 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "thumbnail_delegate.h"

#include <QApplication>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>

#include <algorithm>

//-----------------------------------------------------------------------------

namespace
{
	enum ItemsRole
	{
		SmallDisplayRole = Qt::UserRole
	};
}

//-----------------------------------------------------------------------------

ThumbnailDelegate::ThumbnailDelegate(QListWidget* list)
	: QStyledItemDelegate(list),
	m_list(list),
	m_small_font_metrics(m_small_font)
{
	setFont(list->font());
	list->installEventFilter(this);
}

//-----------------------------------------------------------------------------

void ThumbnailDelegate::paint(QPainter* painter, const QStyleOptionViewItem& opt, const QModelIndex& index) const
{
	QStyleOptionViewItem option = opt;
	initStyleOption(&option, index);

	painter->save();

	// Find colors
	QIcon::Mode mode = QIcon::Normal;
#ifndef Q_OS_MAC
	QPalette::ColorGroup cg = option.widget->hasFocus() ? QPalette::Normal : QPalette::Inactive;
#else
	QPalette::ColorGroup cg = QApplication::focusWidget() ? QPalette::Normal : QPalette::Inactive;
#endif
	QColor color = option.palette.color(cg, QPalette::Text);
	QColor background_color;
	if (option.state & QStyle::State_Selected) {
		mode = QIcon::Selected;
		color = option.palette.color(cg, QPalette::HighlightedText);
		background_color = option.palette.color(cg, QPalette::Highlight);
	}

	// Calculate element locations
	Qt::Alignment alignment = Qt::AlignVCenter;
	int line_height = option.fontMetrics.lineSpacing();
	int small_line_height = m_small_font_metrics.lineSpacing();
	QRect thumbnail_rect, text_rect, small_text_rect;
	if (option.decorationPosition == QStyleOptionViewItem::Top) {
		// Centered text with icon on top
		alignment |= Qt::AlignHCenter;
		thumbnail_rect.setRect(option.rect.left() + 2 + (option.rect.width() - 74) / 2, option.rect.top() + 2, 74, 74);
		int text_width = option.rect.width() - 4;
		text_rect.setRect(option.rect.left() + 2, thumbnail_rect.bottom() + 1, text_width, line_height);
		small_text_rect.setRect(text_rect.left(), text_rect.bottom() + 1, text_width, small_line_height);
	} else if (option.direction == Qt::LeftToRight) {
		// Left aligned
		alignment |= Qt::AlignLeft;
		thumbnail_rect.setRect(option.rect.left() + 5, option.rect.top() + 2 + (option.rect.height() - 74) / 2, 74, 74);
		int text_height = line_height + small_line_height;
		int text_width = option.rect.width() - 4 - thumbnail_rect.width();
		text_rect.setRect(thumbnail_rect.right() + 7, option.rect.top() + 2 + (option.rect.height() - text_height) / 2, text_width, line_height);
		small_text_rect.setRect(text_rect.left(), text_rect.bottom() + 1, text_width, small_line_height);
	} else {
		// Right aligned
		alignment |= Qt::AlignRight;
		thumbnail_rect.setRect(option.rect.right() - 80, option.rect.top() + 2 + (option.rect.height() - 74) / 2, 74, 74);
		int text_height = line_height + small_line_height;
		int text_width = thumbnail_rect.left() - 6;
		text_rect.setRect(2, option.rect.top() + 2 + (option.rect.height() - text_height) / 2, text_width, line_height);
		small_text_rect.setRect(2, text_rect.bottom() + 1, text_width, small_line_height);
	}

	// Draw background
	QStyle* style = option.widget ? option.widget->style() : QApplication::style();
	if (background_color.isValid()) {
		option.backgroundBrush = background_color;
	}
	style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

	// Draw decoration
	const QPixmap thumbnail = option.icon.pixmap(74, 74, mode);
	painter->drawPixmap(thumbnail_rect, thumbnail);

	// Draw text
	painter->setFont(option.font);
	QString text = painter->fontMetrics().elidedText(index.data(Qt::DisplayRole).toString(), option.textElideMode, text_rect.width(), option.displayAlignment);
	painter->setPen(color);
	painter->drawText(text_rect, alignment, text);

	// Draw small text
	painter->setFont(m_small_font);
	text = painter->fontMetrics().elidedText(index.data(SmallDisplayRole).toString(), option.textElideMode, small_text_rect.width(), option.displayAlignment);
	color.setAlphaF(0.6);
	painter->setPen(color);
	painter->drawText(small_text_rect, alignment, text);

	painter->restore();
}

//-----------------------------------------------------------------------------

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(index);
	int text_height = option.fontMetrics.lineSpacing() + m_small_font_metrics.lineSpacing();
	if (option.decorationPosition == QStyleOptionViewItem::Top) {
		return QSize(150, 78 + text_height);
	} else {
		return QSize(option.rect.width(), std::max(78, text_height + 4));
	}
}

//-----------------------------------------------------------------------------

bool ThumbnailDelegate::eventFilter(QObject* object, QEvent* event)
{
	if (object == m_list) {
		if (event->type() == QEvent::FontChange) {
			setFont(m_list->font());
		}
		return false;
	} else {
		return QStyledItemDelegate::eventFilter(object, event);
	}
}

//-----------------------------------------------------------------------------

void ThumbnailDelegate::setFont(const QFont& font)
{
	QFontInfo info(font);
	m_small_font = QFont(info.family(), info.pointSize() - 2);
	m_small_font.setItalic(true);
	m_small_font_metrics = QFontMetrics(m_small_font);
}

//-----------------------------------------------------------------------------
