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

#ifndef APPEARANCE_DIALOG_H
#define APPEARANCE_DIALOG_H

#include <QDialog>
class QAbstractButton;
class QCheckBox;
class QLabel;
class ColorButton;

class AppearanceDialog : public QDialog
{
	Q_OBJECT
public:
	AppearanceDialog(QWidget* parent = 0);

	bool hasBevels() const;
	bool hasShadows() const;
	bool hasMessage() const;
	QPalette colors() const;

	static void setBevelsEnabled(bool enabled);

public slots:
	void accept();

private slots:
	void restoreDefaults();
	void updatePreview();

private:
	QCheckBox* m_has_bevels;
	QCheckBox* m_has_shadows;
	QCheckBox* m_has_message;
	ColorButton* m_background;
	ColorButton* m_shadow;
	ColorButton* m_highlight;
	QLabel* m_preview;
	static bool m_bevels_enabled;
};

#endif

