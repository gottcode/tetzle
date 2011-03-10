/***********************************************************************
 *
 * Copyright (C) 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include "appearance_dialog.h"

#include "color_button.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSettings>

//-----------------------------------------------------------------------------

namespace
{

QPixmap coloredShadow(const QColor& color)
{
	QPixmap shadow = QPixmap(":/shadow.png").alphaChannel();

	QPixmap copy(shadow.size());
	copy.fill(color);
	copy.setAlphaChannel(shadow);

	return copy;
}

}

//-----------------------------------------------------------------------------

AppearanceDialog::AppearanceDialog(QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setWindowTitle(tr("Appearance"));

	m_preview = new QLabel(this);
	m_preview->setAlignment(Qt::AlignCenter);
	m_preview->setAutoFillBackground(true);
	m_preview->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	m_background = new ColorButton(this);
	connect(m_background, SIGNAL(changed(const QColor&)), this, SLOT(updatePreview()));

	m_shadow = new ColorButton(this);
	connect(m_shadow, SIGNAL(changed(const QColor&)), this, SLOT(updatePreview()));

	m_highlight = new ColorButton(this);
	connect(m_highlight, SIGNAL(changed(const QColor&)), this, SLOT(updatePreview()));

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, Qt::Horizontal, this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	connect(buttons->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), this, SLOT(restoreDefaults()));

	QGridLayout* layout = new QGridLayout(this);
	layout->setColumnStretch(3, 1);
	layout->setColumnMinimumWidth(2, 12);
	layout->setRowStretch(0, 1);
	layout->setRowStretch(4, 1);
	layout->setRowMinimumHeight(5, 12);

	layout->addWidget(new QLabel(tr("Background:"), this), 1, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(m_background, 1, 1);
	layout->addWidget(new QLabel(tr("Shadow:"), this), 2, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(m_shadow, 2, 1);
	layout->addWidget(new QLabel(tr("Highlight:"), this), 3, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(m_highlight, 3, 1);

	layout->addWidget(m_preview, 0, 3, 5, 1);
	layout->addWidget(buttons, 6, 0, 1, 4);

	QPalette palette;
	QSettings settings;
	m_background->setColor(settings.value("Colors/Background", palette.color(QPalette::Base)).value<QColor>());
	m_shadow->setColor(settings.value("Colors/Shadow", palette.color(QPalette::Text)).value<QColor>());
	m_highlight->setColor(settings.value("Colors/Highlight", palette.color(QPalette::Highlight)).value<QColor>());
	updatePreview();
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

void AppearanceDialog::accept()
{
	QSettings settings;
	settings.setValue("Colors/Background", m_background->color());
	settings.setValue("Colors/Shadow", m_shadow->color());
	settings.setValue("Colors/Highlight", m_highlight->color());
	QDialog::accept();
}

//-----------------------------------------------------------------------------

void AppearanceDialog::restoreDefaults()
{
	QPalette palette;
	m_background->setColor(palette.color(QPalette::Base));
	m_shadow->setColor(palette.color(QPalette::Text));
	m_highlight->setColor(palette.color(QPalette::Highlight));
	updatePreview();
}

//-----------------------------------------------------------------------------

void AppearanceDialog::updatePreview()
{
	QPixmap bumpmap(":/bumpmap.png");

	QPixmap pixmap(352, 256);
	pixmap.fill(m_background->color());
	{
		QPainter painter(&pixmap);

		// Draw example piece
		QPixmap shadow = coloredShadow(m_shadow->color());
		for (int i = 0; i < 3; ++i) {
			painter.drawPixmap(0, i * 64, shadow);
		}
		painter.drawPixmap(64, 64, shadow);
		painter.drawPixmap(32, 32, bumpmap, 288, 416, 64, 64);
		painter.drawPixmap(32, 96, bumpmap, 32, 32, 64, 64);
		painter.drawPixmap(96, 96, bumpmap, 160, 416, 64, 64);
		painter.drawPixmap(32, 160, bumpmap, 32, 416, 64, 64);

		// Draw example highlighted piece
		painter.translate(160, 0);

		QPixmap highlight = coloredShadow(m_highlight->color());
		for (int i = 0; i < 3; ++i) {
			painter.drawPixmap(0, i * 64, highlight);
		}
		painter.drawPixmap(64, 64, highlight);
		painter.drawPixmap(32, 32, bumpmap, 288, 416, 64, 64);
		painter.drawPixmap(32, 96, bumpmap, 32, 32, 64, 64);
		painter.drawPixmap(96, 96, bumpmap, 160, 416, 64, 64);
		painter.drawPixmap(32, 160, bumpmap, 32, 416, 64, 64);
	}

	QPalette palette = m_preview->palette();
	palette.setColor(m_preview->backgroundRole(), m_background->color());
	m_preview->setPalette(palette);

	m_preview->setPixmap(pixmap);
}

//-----------------------------------------------------------------------------
