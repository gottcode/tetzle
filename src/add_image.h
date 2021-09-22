/*
	SPDX-FileCopyrightText: 2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef ADD_IMAGE_H
#define ADD_IMAGE_H

#include <QCoreApplication>
class QDragEnterEvent;
class QDropEvent;
class QString;
class QWidget;

class AddImage
{
	Q_DECLARE_TR_FUNCTIONS(AddImage)

public:
	static void dragEnterEvent(QDragEnterEvent* event);
	static QStringList dropEvent(QDropEvent* event);
	static QStringList getOpenFileNames(QWidget* parent = 0);
	static QString supportedFormats();
};

#endif
