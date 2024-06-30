/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_NEW_GAME_TAB_H
#define TETZLE_NEW_GAME_TAB_H

class TagManager;
class ToolBarList;

#include <QWidget>
class QAction;
class QLabel;
class QListWidgetItem;
class QPushButton;
class QSettings;
class QSlider;

class NewGameTab : public QWidget
{
	Q_OBJECT

public:
	explicit NewGameTab(const QStringList& files, QDialog* parent = nullptr);

	void addImages(const QStringList& images);

Q_SIGNALS:
	void imageRenamed(const QString& image, const QString& name);
	void newGame(const QString& image, int difficulty);

private Q_SLOTS:
	void accept();
	void addImageClicked();
	void removeImage();
	void editImageProperties();
	void imageSelected();
	void pieceCountChanged(int value);
	void filterImages(const QStringList& filter);
	void updateTagsStrings();

private:
	void addImage(const QString& image);
	QListWidgetItem* createItem(const QString& image, const QSettings& details);
	void removeThumbnail(const QString& image_id);

private:
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

#endif // TETZLE_NEW_GAME_TAB_H
