/*
	SPDX-FileCopyrightText: 2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_ADD_IMAGE_H
#define TETZLE_ADD_IMAGE_H

#include <QCoreApplication>
class QDragEnterEvent;
class QDropEvent;
class QString;
class QWidget;

/**
 * Shared functions for adding images.
 */
class AddImage
{
	Q_DECLARE_TR_FUNCTIONS(AddImage)

public:
	/**
	 * Process drag enter @a event to see if it has images.
	 */
	static void dragEnterEvent(QDragEnterEvent* event);

	/**
	 * Process drop @a event for list of images.
	 *
	 * @return list of images contained in drop event
	 */
	static QStringList dropEvent(QDropEvent* event);

	/**
	 * Prompt player to add images.
	 *
	 * @param parent the parent widget of the file dialog
	 *
	 * @return list of images chosen by player
	 */
	static QStringList getOpenFileNames(QWidget* parent = nullptr);

	/**
	 * Fetch list of supported image formats.
	 *
	 * @return list of supported image formats
	 */
	static QString supportedFormats();
};

#endif // TETZLE_ADD_IMAGE_H
