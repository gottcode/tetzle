/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#ifndef ADD_IMAGE_DIALOG_H
#define ADD_IMAGE_DIALOG_H

#include <QDialog>
#include <QMap>
class QDirModel;
class QListWidget;
class QListWidgetItem;
class QModelIndex;
class QPushButton;
class QSplitter;
class QTreeView;
class ThumbnailList;

class AddImageDialog : public QDialog
{
	Q_OBJECT
public:
	AddImageDialog(QWidget* parent = 0);

	QStringList images;

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void folderSelected(const QModelIndex& index);
	void fileSelectionChanged();
	void storePath();

private:
	QSplitter* m_contents;
	QDirModel* m_folders_model;
	QTreeView* m_folders_view;
	QListWidget* m_files_widget;
	QPushButton* m_open_button;
	QString m_current_folder;

	QStringList m_preview_filters;
	ThumbnailList* m_thumbnails;
};

#endif // ADD_IMAGE_DIALOG_H
