/***********************************************************************
 *
 * Copyright (C) 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef ADD_IMAGE_H
#define ADD_IMAGE_H

class QDragEnterEvent;
class QDropEvent;
class QString;
class QStringList;
class QWidget;

class AddImage
{
public:
	static void dragEnterEvent(QDragEnterEvent* event);
	static QStringList dropEvent(QDropEvent* event);
	static QStringList getOpenFileNames(QWidget* parent = 0);
	static QString supportedFormats();
};

#endif
