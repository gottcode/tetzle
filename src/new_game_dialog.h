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

#ifndef NEW_GAME_DIALOG_H
#define NEW_GAME_DIALOG_H

#include <QDialog>
class QCheckBox;
class QComboBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSlider;
class LabelManager;
class ThumbnailList;

class NewGameDialog : public QDialog
{
	Q_OBJECT
public:
	NewGameDialog(QWidget* parent = 0);

public slots:
	virtual void accept();

signals:
	void newGame(const QString& image, int difficulty);

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void addImage();
	void removeImage();
	void changeLabels();
	void imageSelected(QListWidgetItem* item);
	void pieceCountChanged(int value);
	void filterImages(const QString& filter);

private:
	void addImage(const QString& image);

private:
	LabelManager* m_image_labels;
	QComboBox* m_images_filter;
	QListWidget* m_images;
	ThumbnailList* m_thumbnails;
	QCheckBox* m_letterbox;
	QPushButton* m_remove_button;
	QPushButton* m_label_button;
	QSlider* m_slider;
	QLabel* m_count;
	QSize m_image_size;
	QPushButton* m_accept_button;
};

#endif
