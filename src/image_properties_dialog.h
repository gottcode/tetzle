/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_IMAGE_PROPERTIES_DIALOG_H
#define TETZLE_IMAGE_PROPERTIES_DIALOG_H

class TagManager;

#include <QDialog>
class QLineEdit;
class QListWidget;
class QToolButton;

class ImagePropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	ImagePropertiesDialog(const QIcon& icon, const QString& name, TagManager* manager, const QString& image, QWidget* parent = nullptr);

	QString name() const;

public Q_SLOTS:
	void accept() override;

protected:
	void hideEvent(QHideEvent* event) override;

private Q_SLOTS:
	void addTag();

private:
	QString m_image;
	TagManager* m_manager;
	QLineEdit* m_name;
	QListWidget* m_tags;
	QLineEdit* m_add_tag_name;
	QToolButton* m_add_tag_button;
};

#endif // TETZLE_IMAGE_PROPERTIES_DIALOG_H
