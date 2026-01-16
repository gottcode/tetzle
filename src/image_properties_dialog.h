/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_IMAGE_PROPERTIES_DIALOG_H
#define TETZLE_IMAGE_PROPERTIES_DIALOG_H

class TagManager;

#include <QDialog>
class QLineEdit;
class QListWidget;
class QToolButton;

/**
 * Dialog to set properties of an image.
 */
class ImagePropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	/**
	 * Construct a dialog to set the properties of an image.
	 *
	 * @param icon the preview of the image
	 * @param name the name of the image
	 * @param manager the manager of the tags
	 * @param image the image identifier
	 * @param parent the parent widget of the dialog
	 */
	ImagePropertiesDialog(const QIcon& icon, const QString& name, TagManager* manager, const QString& image, QWidget* parent = nullptr);

	/**
	 * Fetch image name.
	 *
	 * @return name of image
	 */
	QString name() const;

	/**
	 * Filter events of #m_add_tag_name to add a tag when enter is pressed.
	 */
	bool eventFilter(QObject* watched, QEvent* event) override;

public Q_SLOTS:
	/**
	 * Set the properties of the image.
	 */
	void accept() override;

protected:
	/**
	 * Store the size of the dialog for next use.
	 */
	void hideEvent(QHideEvent* event) override;

private Q_SLOTS:
	/**
	 * Handle adding a new tag.
	 */
	void addTag();

private:
	QString m_image; ///< the image to modify
	TagManager* m_manager; ///< the manager of all tags
	QLineEdit* m_name; ///< the name of the image
	QListWidget* m_tags; ///< the tags of the image
	QLineEdit* m_add_tag_name; ///< entry for the name of a new tag
	QToolButton* m_add_tag_button; ///< button to add a tag
};

#endif // TETZLE_IMAGE_PROPERTIES_DIALOG_H
