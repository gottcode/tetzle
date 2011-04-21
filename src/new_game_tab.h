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

#ifndef NEW_GAME_TAB_H
#define NEW_GAME_TAB_H

class TagManager;
class ToolBarList;

#include <QWidget>
class QAction;
class QLabel;
class QListWidgetItem;
class QPushButton;
class QSettings;
class QSlider;
class QSplitter;

class NewGameTab : public QWidget
{
	Q_OBJECT

public:
	NewGameTab(const QStringList& files, QDialog* parent = 0);

	void addImages(const QStringList& images);

signals:
	void imageRenamed(const QString& image, const QString& name);
	void newGame(const QString& image, int difficulty);

protected:
	virtual void hideEvent(QHideEvent* event);

private slots:
	void accept();
	void addImage();
	void removeImage();
	void changeTags();
	void imageChanged(QListWidgetItem* item);
	void imageSelected(QListWidgetItem* item);
	void pieceCountChanged(int value);
	void filterImages(const QStringList& filter);

private:
	void addImage(const QString& image);
	QListWidgetItem* createItem(const QString& image, const QSettings& details);

private:
	QSplitter* m_image_contents;
	TagManager* m_image_tags;
	ToolBarList* m_images;
	QAction* m_remove_action;
	QAction* m_tag_action;

	QSlider* m_slider;
	QLabel* m_count;
	QSize m_image_size;
	float m_ratio;

	QPushButton* m_accept_button;
};

#endif
