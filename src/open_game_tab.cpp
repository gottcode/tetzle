/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "open_game_tab.h"

#include "choose_game_dialog.h"
#include "path.h"
#include "thumbnail_delegate.h"
#include "thumbnail_item.h"
#include "thumbnail_loader.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>
#include <QXmlStreamReader>

//-----------------------------------------------------------------------------

OpenGameTab::OpenGameTab(int current_id, QDialog* parent)
	: QWidget(parent)
{
	// List saved games
	m_games = new QListWidget(this);
	m_games->setIconSize(QSize(74, 74));
	m_games->setMovement(QListView::Static);
	m_games->setResizeMode(QListView::Adjust);
	m_games->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_games->setItemDelegate(new ThumbnailDelegate(m_games));

	const qreal pixelratio = devicePixelRatioF();
	const QSettings image_details(Path::image("details"), QSettings::IniFormat);
	const QStringList files = ChooseGameDialog::currentGames();
	for (const QString& game : files) {
		// Skip currently opened game
		const int id = game.section(".", 0, 0).toInt();
		if (current_id == id) {
			continue;
		}

		// Open saved game file
		QFile file(Path::save(game));
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		QXmlStreamReader xml(&file);

		// Load details
		while (!xml.isStartElement()) {
			xml.readNext();
		}
		const QXmlStreamAttributes attributes = xml.attributes();
		const QString image = attributes.value("image").toString();
		if (!QFile::exists(Path::image(image))) {
			continue;
		}
		const QString image_name = image_details.value(image + "/Name", tr("Untitled")).toString();
		const QString pieces = attributes.value("pieces").toString();
		const QString complete = attributes.value("complete").toString();
		const QString details = tr("%L1 pieces %2 %3% complete").arg(pieces, QChar(8226), complete);
		QListWidgetItem* item = ThumbnailLoader::createItem(image, image_name, m_games, pixelratio);
		item->setData(ThumbnailItem::GameRole, id);
		item->setData(ThumbnailItem::DetailsRole, details);
	}
	m_games->setCurrentRow(0);

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &OpenGameTab::accept);
	connect(buttons, &QDialogButtonBox::rejected, parent, &QDialog::reject);

	m_accept_button = buttons->button(QDialogButtonBox::Ok);
	m_accept_button->setText(ChooseGameDialog::tr("Play Game"));
	m_accept_button->setEnabled(m_games->count() > 0);

	QPushButton* delete_button = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	if (delete_button->style()->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons)) {
		delete_button->setIcon(QIcon::fromTheme("edit-delete"));
	}
	delete_button->setEnabled(m_accept_button->isEnabled());
	connect(delete_button, &QPushButton::clicked, this, &OpenGameTab::deleteGame);

	// Arrange widgets
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_games);
	layout->addSpacing(12);
	layout->addWidget(buttons);
}

//-----------------------------------------------------------------------------

void OpenGameTab::imageRenamed(const QString& image, const QString& name)
{
	for (int i = 0, count = m_games->count(); i < count; ++i) {
		QListWidgetItem* item = m_games->item(i);
		if (item->data(ThumbnailItem::ImageRole).toString() == image) {
			item->setText(name);
		}
	}
}

//-----------------------------------------------------------------------------

void OpenGameTab::hideEvent(QHideEvent* event)
{
	m_accept_button->setDefault(false);
	QWidget::hideEvent(event);
}

//-----------------------------------------------------------------------------

void OpenGameTab::showEvent(QShowEvent* event)
{
	m_accept_button->setDefault(true);
	QWidget::showEvent(event);
}

//-----------------------------------------------------------------------------

void OpenGameTab::accept()
{
	const QListWidgetItem* item = m_games->currentItem();
	if (item) {
		Q_EMIT openGame(item->data(ThumbnailItem::GameRole).toInt());
	}
}

//-----------------------------------------------------------------------------

void OpenGameTab::deleteGame()
{
	const QListWidgetItem* item = m_games->currentItem();
	if (!item) {
		return;
	}

	if (QMessageBox::question(this, tr("Delete Game"), tr("Delete selected game?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		QFile::remove(Path::save(item->data(ThumbnailItem::GameRole).toInt()));
		delete item;
		m_accept_button->setEnabled(m_games->count() > 0);
	};
}

//-----------------------------------------------------------------------------
