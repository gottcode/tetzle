/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef IMAGE_PROPERTIES_DIALOG_H
#define IMAGE_PROPERTIES_DIALOG_H

class TagManager;

#include <QDialog>
class QLineEdit;
class QListWidget;
class QPushButton;

class ImagePropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	ImagePropertiesDialog(const QIcon& icon, const QString& name, TagManager* manager, const QString& image, QWidget* parent = 0);

	QString name() const;

public slots:
	virtual void accept();

protected:
	virtual void hideEvent(QHideEvent* event);

private:
	QString m_image;
	TagManager* m_manager;
	QLineEdit* m_name;
	QListWidget* m_tags;
};

#endif
