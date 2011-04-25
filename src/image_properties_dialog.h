/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#ifndef IMAGE_PROPERTIES_DIALOG_H
#define IMAGE_PROPERTIES_DIALOG_H

class TagManager;

#include <QDialog>
class QLineEdit;
class QListWidget;
class QPushButton;

class ImagePropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	ImagePropertiesDialog(const QIcon& icon, const QString& name, TagManager* manager, const QString& image, QWidget* parent = 0);

	QString name() const;

public slots:
	virtual void accept();

protected:
	virtual void hideEvent(QHideEvent* event);

private:
	QString m_image;
	TagManager* m_manager;
	QLineEdit* m_name;
	QListWidget* m_tags;
};

#endif
