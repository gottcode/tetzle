/*
	SPDX-FileCopyrightText: 2010-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef PATH_H
#define PATH_H

#include <QtGlobal>
class QString;

class Path
{
public:
	static QString datapath();
	static QString oldDataPath();

	static QString image(const QString& file);
	static QString thumbnail(const QString& image, qreal pixelratio);
	static QString save(const QString& file);
	static QString save(int game);

	static QString images();
	static QString thumbnails();
	static QString saves();
};

#endif
