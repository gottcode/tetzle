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

/**
 * Tab to set the parameters of a new game.
 */
class NewGameTab : public QWidget
{
	Q_OBJECT

public:
	/**
	 * Construct a new game tab.
	 *
	 * @param files a list of images to add
	 * @param parent the parent widget of the tab
	 */
	explicit NewGameTab(const QStringList& files, QDialog* parent = nullptr);

	/**
	 * Add to list of images.
	 *
	 * @param images a list of images to add
	 */
	void addImages(const QStringList& images);

Q_SIGNALS:
	/**
	 * Signal that an image was renamed.
	 *
	 * @param image the image identifier
	 * @param name the new image name
	 */
	void imageRenamed(const QString& image, const QString& name);

	/**
	 * Signal to start a new game.
	 *
	 * @param image the image to use on the pieces
	 * @param difficulty how many pieces to generate
	 */
	void newGame(const QString& image, int difficulty);

private Q_SLOTS:
	/**
	 * Start a new game.
	 */
	void accept();

	/**
	 * Prompt player for image to add.
	 */
	void addImageClicked();

	/**
	 * Remove selected images.
	 */
	void removeImage();

	/**
	 * Edit properties of selected image.
	 */
	void editImageProperties();

	/**
	 * Handle images being selected. Enables or disables actions, and updates pieces slider.
	 */
	void imageSelected();

	/**
	 * Update piece count display.
	 *
	 * @param value how many pieces
	 */
	void pieceCountChanged(int value);

	/**
	 * Filter what images are shown.
	 *
	 * @param filter the list of images that can be shown
	 */
	void filterImages(const QStringList& filter);

	/**
	 * Update the text and tooltips of the images.
	 */
	void updateTagsStrings();

protected:
	/**
	 * Unset default button.
	 */
	void hideEvent(QHideEvent* event) override;

	/**
	 * Set default button to #m_accept_button.
	 */
	void showEvent(QShowEvent* event) override;

private:
	/**
	 * Add image to list.
	 *
	 * @param image image to add
	 */
	void addImage(const QString& image);

	/**
	 * Create a listwidget item.
	 *
	 * @param image the image of the item
	 * @param details where to find the image name
	 */
	QListWidgetItem* createItem(const QString& image, const QSettings& details);

	/**
	 * Remove an image thumbnail.
	 *
	 * @param image_id remove thumbnail of this image
	 */
	void removeThumbnail(const QString& image_id) const;

private:
	TagManager* m_image_tags; ///< manager of tags
	ToolBarList* m_images; ///< list of images
	QAction* m_remove_action; ///< action to remove images
	QAction* m_tag_action; ///< action to edit image properties

	QSlider* m_slider; ///< amount of pieces
	QLabel* m_count; ///< display of amount of pieces
	QSize m_image_size; ///< size of selected image, used to find piece count
	float m_ratio; ///< aspect ratio of selected image, used to find piece count

	QPushButton* m_accept_button; ///< button to start new game
};

#endif // TETZLE_NEW_GAME_TAB_H
