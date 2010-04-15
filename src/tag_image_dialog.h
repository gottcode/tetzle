/***********************************************************************
 *
 * Copyright (C) 2008, 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef TAG_IMAGE_DIALOG_H
#define TAG_IMAGE_DIALOG_H

#include <QDialog>
class QListWidget;
class QListWidgetItem;
class QPushButton;
class TagManager;

class TagImageDialog : public QDialog
{
	Q_OBJECT
public:
	TagImageDialog(const QString& image, TagManager* manager, QString& filter, QWidget* parent = 0);

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void addTag();
	void removeTag();
	void tagChanged(QListWidgetItem* tag);

private:
	QString m_image;
	TagManager* m_manager;
	QString& m_filter;

	QListWidget* m_tags;
	QPushButton* m_remove_button;
};

#endif
