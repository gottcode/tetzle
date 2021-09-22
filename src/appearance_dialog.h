/*
	SPDX-FileCopyrightText: 2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

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
	ColorButton* m_background;
	ColorButton* m_shadow;
	ColorButton* m_highlight;
	QLabel* m_preview;
	static bool m_bevels_enabled;
};

#endif

