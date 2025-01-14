/*
	SPDX-FileCopyrightText: 2010-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "appearance_dialog.h"

#include "color_button.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSettings>

//-----------------------------------------------------------------------------

AppearanceDialog::AppearanceDialog(QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setWindowTitle(tr("Appearance"));

	// Create preview widget
	m_preview = new QLabel(this);
	m_preview->setAlignment(Qt::AlignCenter);
	m_preview->setAutoFillBackground(true);
	m_preview->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	// Create options widgets
	QGroupBox* options_group = new QGroupBox(tr("Options"), this);

	m_has_bevels = new QCheckBox(tr("Beveled borders"), options_group);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(m_has_bevels, &QCheckBox::checkStateChanged, this, &AppearanceDialog::updatePreview);
#else
	connect(m_has_bevels, &QCheckBox::stateChanged, this, &AppearanceDialog::updatePreview);
#endif

	m_has_shadows = new QCheckBox(tr("Drop shadows"), options_group);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(m_has_shadows, &QCheckBox::checkStateChanged, this, &AppearanceDialog::updatePreview);
#else
	connect(m_has_shadows, &QCheckBox::stateChanged, this, &AppearanceDialog::updatePreview);
#endif

	// Create colors widgets
	QGroupBox* colors_group = new QGroupBox(tr("Colors"), this);

	m_background = new ColorButton(colors_group);
	connect(m_background, &ColorButton::changed, this, &AppearanceDialog::updatePreview);

	m_shadow = new ColorButton(colors_group);
	connect(m_shadow, &ColorButton::changed, this, &AppearanceDialog::updatePreview);

	m_highlight = new ColorButton(colors_group);
	connect(m_highlight, &ColorButton::changed, this, &AppearanceDialog::updatePreview);

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &AppearanceDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &AppearanceDialog::reject);
	connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &AppearanceDialog::restoreDefaults);

	// Lay out dialog
	QFormLayout* colors_layout = new QFormLayout(colors_group);
	colors_layout->addRow(tr("Background:"), m_background);
	colors_layout->addRow(tr("Shadow:"), m_shadow);
	colors_layout->addRow(tr("Highlight:"), m_highlight);

	QVBoxLayout* options_layout = new QVBoxLayout(options_group);
	options_layout->addWidget(m_has_bevels);
	options_layout->addWidget(m_has_shadows);

	QGridLayout* layout = new QGridLayout(this);
	layout->setSpacing(12);
	layout->setColumnStretch(1, 1);
	layout->setRowStretch(2, 1);

	layout->addWidget(options_group, 0, 0);
	layout->addWidget(colors_group, 1, 0);
	layout->addWidget(m_preview, 0, 1, 3, 1);
	layout->addWidget(buttons, 4, 0, 1, 2);

	// Load settings
	const QSettings settings;
	m_background->setColor(settings.value("Colors/Background", QColor(0x9d, 0x89, 0x75)).value<QColor>());
	m_shadow->setColor(settings.value("Colors/Shadow", QColor(Qt::black)).value<QColor>());
	m_highlight->setColor(settings.value("Colors/Highlight", QColor(Qt::white)).value<QColor>());
	m_has_bevels->setChecked(settings.value("Appearance/Bevels", true).toBool());
	m_has_shadows->setChecked(settings.value("Appearance/Shadows", true).toBool());
	updatePreview();
}

//-----------------------------------------------------------------------------

bool AppearanceDialog::hasBevels() const
{
	return m_has_bevels->isChecked();
}

//-----------------------------------------------------------------------------

bool AppearanceDialog::hasShadows() const
{
	return m_has_shadows->isChecked();
}

//-----------------------------------------------------------------------------

QPalette AppearanceDialog::colors() const
{
	QPalette palette;
	palette.setColor(QPalette::Base, m_background->color());
	palette.setColor(QPalette::Text, m_shadow->color());
	palette.setColor(QPalette::Highlight, m_highlight->color());
	return palette;
}

//-----------------------------------------------------------------------------

QPixmap AppearanceDialog::shadow(qreal pixelratio)
{
	return coloredShadow(QSettings().value("Colors/Shadow", QColor(Qt::black)).value<QColor>(), pixelratio);
}

//-----------------------------------------------------------------------------

QPixmap AppearanceDialog::shadowSelected(qreal pixelratio)
{
	return coloredShadow(QSettings().value("Colors/Highlight", QColor(Qt::white)).value<QColor>(), pixelratio);
}

//-----------------------------------------------------------------------------

void AppearanceDialog::accept()
{
	QSettings settings;
	settings.setValue("Colors/Background", m_background->color().name());
	settings.setValue("Colors/Shadow", m_shadow->color().name());
	settings.setValue("Colors/Highlight", m_highlight->color().name());
	settings.setValue("Appearance/Bevels", m_has_bevels->isChecked());
	settings.setValue("Appearance/Shadows", m_has_shadows->isChecked());
	QDialog::accept();
}

//-----------------------------------------------------------------------------

void AppearanceDialog::restoreDefaults()
{
	m_background->setColor(QColor(0x9d, 0x89, 0x75));
	m_shadow->setColor(Qt::black);
	m_highlight->setColor(Qt::white);
	m_has_bevels->setChecked(true);
	m_has_shadows->setChecked(true);
	updatePreview();
}

//-----------------------------------------------------------------------------

void AppearanceDialog::updatePreview()
{
	const qreal pixelratio = devicePixelRatioF();

	QPixmap bumpmap(512, 512);
	bumpmap.fill(QColor(128, 128, 128));
	if (m_has_bevels->isChecked()) {
		QPainter painter(&bumpmap);
		painter.drawPixmap(0, 0, QPixmap(":/bumpmap.png"));
	}
	bumpmap = bumpmap.scaled(bumpmap.size() * pixelratio, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	bumpmap.setDevicePixelRatio(pixelratio);

	QPixmap pixmap(352 * pixelratio, 256 * pixelratio);
	pixmap.setDevicePixelRatio(pixelratio);
	pixmap.fill(m_background->color());
	{
		QPainter painter(&pixmap);

		// Draw example piece
		if (m_has_shadows->isChecked()) {
			QPixmap shadow = coloredShadow(m_shadow->color(), pixelratio);
			for (int i = 0; i < 3; ++i) {
				painter.drawPixmap(0, i * 64, shadow);
			}
			painter.drawPixmap(64, 64, shadow);
		}
		painter.drawPixmap(32, 32, bumpmap, 288 * pixelratio, 416 * pixelratio, 64 * pixelratio, 64 * pixelratio);
		painter.drawPixmap(32, 96, bumpmap, 32 * pixelratio, 32 * pixelratio, 64 * pixelratio, 64 * pixelratio);
		painter.drawPixmap(96, 96, bumpmap, 160 * pixelratio, 416 * pixelratio, 64 * pixelratio, 64 * pixelratio);
		painter.drawPixmap(32, 160, bumpmap, 32 * pixelratio, 416 * pixelratio, 64 * pixelratio, 64 * pixelratio);

		// Draw example highlighted piece
		painter.translate(160, 0);

		if (m_has_shadows->isChecked()) {
			QPixmap highlight = coloredShadow(m_highlight->color(), pixelratio);
			for (int i = 0; i < 3; ++i) {
				painter.drawPixmap(0, i * 64, highlight);
			}
			painter.drawPixmap(64, 64, highlight);
		}
		painter.drawPixmap(32, 32, bumpmap, 288 * pixelratio, 416 * pixelratio, 64 * pixelratio, 64 * pixelratio);
		painter.drawPixmap(32, 96, bumpmap, 32 * pixelratio, 32 * pixelratio, 64 * pixelratio, 64 * pixelratio);
		painter.drawPixmap(96, 96, bumpmap, 160 * pixelratio, 416 * pixelratio, 64 * pixelratio, 64 * pixelratio);
		painter.drawPixmap(32, 160, bumpmap, 32 * pixelratio, 416 * pixelratio, 64 * pixelratio, 64 * pixelratio);
	}

	QPalette palette = m_preview->palette();
	palette.setColor(m_preview->backgroundRole(), m_background->color());
	m_preview->setPalette(palette);

	m_preview->setPixmap(pixmap);
}

//-----------------------------------------------------------------------------

QPixmap AppearanceDialog::coloredShadow(const QColor& color, qreal pixelratio)
{
	const QPixmap source(":/shadow.png");
	QImage shadow(source.size(), QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&shadow);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.drawPixmap(0, 0, source);
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.fillRect(shadow.rect(), color);
	painter.end();

	QPixmap result = QPixmap::fromImage(shadow.scaled(shadow.size() * pixelratio, Qt::IgnoreAspectRatio, Qt::SmoothTransformation),
			Qt::AvoidDither | Qt::AutoColor | Qt::NoOpaqueDetection);
	result.setDevicePixelRatio(pixelratio);

	return result;
}

//-----------------------------------------------------------------------------
