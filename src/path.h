/*
	SPDX-FileCopyrightText: 2010-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_PATH_H
#define TETZLE_PATH_H

#include <QString>

/**
 * Shared functions to find locations of files.
 */
class Path
{
public:
	/**
	 * Set up player data path.
	 *
	 * @param userdir the location for portable mode support
	 */
	static void load(const QString& userdir);

	/**
	 * Find the path of an image.
	 *
	 * @param file the image to find
	 *
	 * @return path of image
	 */
	static QString image(const QString& file);

	/**
	 * Find the path of thumbnail for an image.
	 *
	 * @param image the image to find
	 * @param pixelratio the pixel ratio to render at
	 *
	 * @return path of thumbnail
	 */
	static QString thumbnail(const QString& image, qreal pixelratio);

	/**
	 * Find the path of a saved gave.
	 *
	 * @param file the save game to find
	 *
	 * @return path of saved game
	 */
	static QString save(const QString& file);

	/**
	 * Find the path of a saved gave.
	 *
	 * @param game the save game to find
	 *
	 * @return path of saved game
	 */
	static QString save(int game);

	/**
	 * Fetch path of images.
	 *
	 * @return path of images
	 */
	static QString images();

	/**
	 * Fetch path of thumbnails.
	 *
	 * @return path of thumbnails
	 */
	static QString thumbnails();

	/**
	 * Fetch path of save games.
	 *
	 * @return path of save games
	 */
	static QString saves();

private:
	/**
	 * Fetch old path of Tetzle player data.
	 *
	 * @return old path of Tetzle player data
	 */
	static QString oldDataPath();

private:
	static QString m_path; ///< path for player data
};

#endif // TETZLE_PATH_H
