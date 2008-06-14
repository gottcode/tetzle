/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#ifndef THUMBNAIL_H
#define THUMBNAIL_H

#include <QThread>

class Thumbnail : public QThread
{
	Q_OBJECT
public:
	Thumbnail(const QString& image, const QString& thumbnail);

signals:
	void generated(const QString& image);

protected:
	virtual void run();

private:
	QString m_image;
	QString m_thumbnail;
};

#endif // THUMBNAIL_H
