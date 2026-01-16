/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_TAG_MANAGER_H
#define TETZLE_TAG_MANAGER_H

#include <QComboBox>
#include <QHash>
#include <QStringList>
class QDialog;
class QListWidget;
class QListWidgetItem;
class QPushButton;

/**
 * Combobox that manages tags.
 */
class TagManager : public QComboBox
{
	Q_OBJECT

public:
	/**
	 * Construct a tag manager widget.
	 *
	 * @param parent the parent widget of the tag manager
	 */
	explicit TagManager(QWidget* parent = nullptr);

	/**
	 * Check if a tag exists.
	 *
	 * @param tag the tag to check
	 *
	 * @return @c true if a tag exists
	 */
	bool hasTag(const QString& tag) const
	{
		return tag.isEmpty() || m_tags.contains(tag);
	}

	/**
	 * Fetch images contained in a tag.
	 *
	 * @param tag the tag to look up
	 *
	 * @return list of images contained in tag
	 */
	QStringList images(const QString& tag) const;

	/**
	 * Fetch sorted list of all tags.
	 *
	 * @return sorted list of all tags
	 */
	QStringList tags() const;

	/**
	 * Fetch tags that contain an image.
	 *
	 * @param image the image to check
	 *
	 * @return tags containing image, sorted and combined into single string
	 */
	QString tags(const QString& image) const;

	/**
	 * Show all images.
	 */
	void clearFilter();

	/**
	 * Add an image.
	 *
	 * @param image the image to add
	 */
	void addImage(const QString& image);

	/**
	 * Remove an image from all tags.
	 *
	 * @param image the image to remove
	 */
	void removeImage(const QString& image);

	/**
	 * Set the list of tags for an image.
	 *
	 * @param image the image to modify
	 * @param tags the tags which contain the image
	 */
	void setImageTags(const QString& image, const QStringList& tags);

Q_SIGNALS:
	/**
	 * Signal to only show certain images.
	 *
	 * @param images the list of images to show
	 */
	void filterChanged(const QStringList& images);

	/**
	 * Signal that the list of tags has changed.
	 */
	void tagsChanged();

private Q_SLOTS:
	/**
	 * Add a new untitled tag.
	 */
	void addTag();

	/**
	 * Remove selected tag.
	 */
	void removeTag();

	/**
	 * Update filter of which images to show.
	 *
	 * @param index only show images in this tag
	 */
	void currentTagChanged(int index);

	/**
	 * Handle renaming of a tag. Prevents empty or duplicate tags.
	 *
	 * @param item the item representing the tag
	 */
	void tagChanged(QListWidgetItem* item);

private:
	/**
	 * Construct a listwidget item.
	 *
	 * @param tag the tag to create a listwidget item for
	 */
	QListWidgetItem* createTag(const QString& tag);

	/**
	 * Save tags.
	 */
	void storeTags() const;

	/**
	 * Update combobox items to match list of tags.
	 */
	void updateFilter();

private:
	QHash<QString, QStringList> m_tags; ///< list of tags
	QStringList m_all; ///< all images

	QDialog* m_manage_dialog; ///< dialog to add, edit, or remove tags
	QListWidget* m_tags_list; ///< list of tags in dialog
	QPushButton* m_remove_tag_button; ///< accept button for dialog
};

#endif // TETZLE_TAG_MANAGER_H
