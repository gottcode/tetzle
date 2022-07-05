/*
	SPDX-FileCopyrightText: 2010-2022 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_PATH_H
#define TETZLE_PATH_H

#include <QString>

class Path
{
public:
	static void load(const QString& userdir);

	static QString image(const QString& file);
	static QString thumbnail(const QString& image, qreal pixelratio);
	static QString save(const QString& file);
	static QString save(int game);

	static QString images();
	static QString thumbnails();
	static QString saves();

private:
	static QString oldDataPath();

private:
	static QString m_path;
};

#endif // TETZLE_PATH_H
