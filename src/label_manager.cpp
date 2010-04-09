/***********************************************************************
 *
 * Copyright (C) 2008, 2010 Graeme Gott <graeme@gottcode.org>
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

#include "label_manager.h"

#include <QDir>
#include <QSettings>

//-----------------------------------------------------------------------------

LabelManager::LabelManager(QObject* parent)
	: QObject(parent)
{
	QSettings file("images/labels", QSettings::IniFormat);
	file.beginGroup("Labels");
	QStringList labels = file.childKeys();
	QStringList images;
	QDir folder("images/", "*.*");
	foreach (QString label, labels) {
		images = file.value(label).toStringList();
		QMutableStringListIterator i(images);
		while (i.hasNext()) {
			i.next();
			if (!folder.exists(i.value())) {
				i.remove();
			}
		}
		m_labels[label] = images;
	}
}

//-----------------------------------------------------------------------------

QStringList LabelManager::labels(bool list_empty) const
{
	QStringList labels;
	if (!list_empty) {
		QMap<QString, QStringList>::const_iterator i;
		for (i = m_labels.constBegin(); i != m_labels.constEnd(); ++i) {
			if (!i.value().isEmpty()) {
				labels.append(i.key());
			}
		}
	} else {
		labels = m_labels.keys();
	}
	labels.sort();
	labels.prepend(tr("All"));
	return labels;
}

//-----------------------------------------------------------------------------

QStringList LabelManager::images(const QString& label) const
{
	if (label != tr("All")) {
		return m_labels.value(label);
	} else {
		return QDir("images/", "*.*", QDir::Name | QDir::LocaleAware, QDir::Files).entryList();
	}
}

//-----------------------------------------------------------------------------

bool LabelManager::isLabelEmpty(const QString& label) const
{
	return images(label).isEmpty();
}

//-----------------------------------------------------------------------------

bool LabelManager::addLabel(const QString& label)
{
	if (label == tr("All") || label.isEmpty() || m_labels.constFind(label) != m_labels.constEnd()) {
		return false;
	}

	m_labels.insert(label, QStringList());
	storeLabels();

	return true;
}

//-----------------------------------------------------------------------------

bool LabelManager::renameLabel(const QString& label, const QString& old_label)
{
	if (m_labels.constFind(label) != m_labels.constEnd() || m_labels.constFind(old_label) == m_labels.constEnd() || label.isEmpty()) {
		return false;
	}

	m_labels.insert(label, m_labels.take(old_label));
	storeLabels();

	return true;
}

//-----------------------------------------------------------------------------

bool LabelManager::removeLabel(const QString& label)
{
	if (m_labels.constFind(label) == m_labels.constEnd()) {
		return false;
	}

	m_labels.remove(label);
	storeLabels();

	return true;
}

//-----------------------------------------------------------------------------

void LabelManager::addImage(const QString& image, const QString& label)
{
	QMap<QString, QStringList>::iterator i = m_labels.find(label);
	if (i != m_labels.end() && !i.value().contains(image)) {
		i.value().append(image);
		storeLabels();
	}
}

//-----------------------------------------------------------------------------

void LabelManager::removeImage(const QString& image, const QString& label)
{
	QMap<QString, QStringList>::iterator i = m_labels.find(label);
	if (i != m_labels.end()) {
		i.value().removeAll(image);
		storeLabels();
	}
}

//-----------------------------------------------------------------------------

void LabelManager::removeImage(const QString& image)
{
	QMutableMapIterator<QString, QStringList> i(m_labels);
	while (i.hasNext()) {
		i.next();
		i.value().removeAll(image);
	}
	storeLabels();
}

//-----------------------------------------------------------------------------

void LabelManager::storeLabels()
{
	QSettings file("images/labels", QSettings::IniFormat);
	file.clear();
	file.beginGroup("Labels");
	QMapIterator<QString, QStringList> i(m_labels);
	while (i.hasNext()) {
		i.next();
		file.setValue(i.key(), i.value());
	}
}

//-----------------------------------------------------------------------------
