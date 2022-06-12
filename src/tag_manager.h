/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_TAG_MANAGER_H
#define TETZLE_TAG_MANAGER_H

class ToolBarList;

#include <QHash>
#include <QStringList>
#include <QWidget>
class QAction;
class QListWidgetItem;

class TagManager : public QWidget
{
	Q_OBJECT

public:
	explicit TagManager(QWidget* parent = nullptr);

	QStringList images(const QString& tag) const;
	QStringList tags() const;
	QString tags(const QString& image) const;

	void clearFilter();
	void addImage(const QString& image);
	void removeImage(const QString& image);
	void setImageTags(const QString& image, const QStringList& tags);

Q_SIGNALS:
	void filterChanged(const QStringList& images);
	void tagsChanged();

protected:
	void changeEvent(QEvent* event) override;

private Q_SLOTS:
	void addTag();
	void removeTag();
	void currentTagChanged(QListWidgetItem* item);
	void tagChanged(QListWidgetItem* item);
	void updateFilter();

private:
	void storeTags();

private:
	QHash<QString, QStringList> m_tags;
	QStringList m_untagged;
	ToolBarList* m_filter;
	QListWidgetItem* m_all_images_item;
	QListWidgetItem* m_untagged_item;
	QAction* m_remove_action;
};

#endif // TETZLE_TAG_MANAGER_H
