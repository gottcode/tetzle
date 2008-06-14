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

#ifndef LABEL_MANAGER_H
#define LABEL_MANAGER_H

#include <QMap>
#include <QSet>
#include <QStringList>

class LabelManager : public QObject
{
public:
	LabelManager(QObject* parent = 0);

	QStringList labels(bool list_empty = false) const;
	QStringList images(const QString& label) const;
	bool isLabelEmpty(const QString& label) const;

	bool addLabel(const QString& label);
	bool renameLabel(const QString& label, const QString& old_label);
	bool removeLabel(const QString& label);
	void addImage(const QString& image, const QString& label);
	void removeImage(const QString& image, const QString& label);
	void removeImage(const QString& image);

private:
	void storeLabels();

	QMap<QString, QStringList> m_labels;
};

#endif // LABEL_MANAGER_H
