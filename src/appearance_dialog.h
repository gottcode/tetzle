/*
	SPDX-FileCopyrightText: 2010-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_APPEARANCE_DIALOG_H
#define TETZLE_APPEARANCE_DIALOG_H

class ColorButton;

#include <QDialog>
class QAbstractButton;
class QCheckBox;
class QLabel;

/**
 *  Dialog to set game appearance.
 */
class AppearanceDialog : public QDialog
{
	Q_OBJECT

public:
	/**
	 * Construct a dialog to set game appearance.
	 *
	 * @param parent the parent widget of the dialog
	 */
	explicit AppearanceDialog(QWidget* parent = nullptr);

	/**
	 * Fetch if pieces have bevels.
	 *
	 * @return @c true if pieces have bevels
	 */
	bool hasBevels() const;

	/**
	 * Fetch if pieces have shadows.
	 *
	 * @return @c true if pieces have shadows
	 */
	bool hasShadows() const;

	/**
	 * Fetch board colors.
	 *
	 * @return QPalette containing board colors
	 */
	QPalette colors() const;

	/**
	 * Create shadow for unselected pieces.
	 *
	 * @param pixelratio the pixel ratio to render at
	 *
	 * @return QPixmap containing shadow image
	 */
	static QPixmap shadow(qreal pixelratio);

	/**
	 * Create shadow for selected pieces.
	 *
	 * @param pixelratio the pixel ratio to render at
	 *
	 * @return QPixmap containing shadow image
	 */
	static QPixmap shadowSelected(qreal pixelratio);

public Q_SLOTS:
	/**
	 * Store game appearance.
	 */
	void accept() override;

private Q_SLOTS:
	/**
	 * Reset appearance back to default values.
	 */
	void restoreDefaults();

	/**
	 * Update the preview image to show chosen game appearance settings.
	 */
	void updatePreview();

private:
	/**
	 * Create shadow for pieces.
	 *
	 * @param color the color of the shadow
	 * @param pixelratio the pixel ratio to render at
	 *
	 * @return QPixmap containing shadow image
	 */
	static QPixmap coloredShadow(const QColor& color, qreal pixelratio);

private:
	QCheckBox* m_has_bevels; ///< do pieces have bevels
	QCheckBox* m_has_shadows; ///< do pieces have shadows
	ColorButton* m_background; ///< background color for board
	ColorButton* m_shadow; ///< color of shadow for unselected pieces
	ColorButton* m_highlight; ///< color of shadow for selected pieces
	QLabel* m_preview; ///< preview of chosen game appearance settings
};

#endif // TETZLE_APPEARANCE_DIALOG_H
