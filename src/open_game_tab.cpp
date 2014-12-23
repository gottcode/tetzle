/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011, 2014 Graeme Gott <graeme@gottcode.org>
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

#include "open_game_tab.h"

#include "choose_game_dialog.h"
#include "path.h"
#include "thumbnail_delegate.h"
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

namespace
{
	enum ItemRoles
	{
		DetailsRole = Qt::UserRole,
		ImageRole,
		GameRole
	};
}

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

	QSettings details(Path::image("details"), QSettings::IniFormat);
	QXmlStreamReader xml;
	QXmlStreamAttributes attributes;
	QStringList files = ChooseGameDialog::currentGames();
	for (const QString& game : files) {
		QFile file(Path::save(game));
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		xml.setDevice(&file);

		int id = game.section(".", 0, 0).toInt();
		if (current_id == id) {
			continue;
		}

		// Load details
		while (!xml.isStartElement()) {
			xml.readNext();
		}
		attributes = xml.attributes();
		QString image = attributes.value("image").toString();
		if (!QFile::exists(Path::image(image))) {
			continue;
		}
		QString image_name = details.value(image + "/Name", tr("Untitled")).toString();
		QString pieces = attributes.value("pieces").toString();
		QString complete = attributes.value("complete").toString();
		QString details = tr("%L1 pieces %2 %3% complete").arg(pieces, QChar(8226), complete);
		QListWidgetItem* item = ThumbnailLoader::createItem(Path::image(image), image_name, m_games);
		item->setData(GameRole, id);
		item->setData(ImageRole, image);
		item->setData(DetailsRole, details);
	}
	m_games->setCurrentRow(0);

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &OpenGameTab::accept);
	connect(buttons, &QDialogButtonBox::rejected, parent, &QDialog::reject);

	m_accept_button = buttons->button(QDialogButtonBox::Open);
	m_accept_button->setEnabled(m_games->count() > 0);

	QPushButton* delete_button = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
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
	int count = m_games->count();
	for (int i = 0; i < count; ++i) {
		QListWidgetItem* item = m_games->item(i);
		if (item->data(ImageRole).toString() == image) {
			item->setText(name);
		}
	}
}

//-----------------------------------------------------------------------------

void OpenGameTab::accept()
{
	QListWidgetItem* item = m_games->currentItem();
	if (item) {
		emit openGame(item->data(GameRole).toInt());
	}
}

//-----------------------------------------------------------------------------

void OpenGameTab::deleteGame()
{
	QListWidgetItem* item = m_games->currentItem();
	if (!item) {
		return;
	}

	if (QMessageBox::question(this, tr("Delete Game"), tr("Delete selected game?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		QFile::remove(Path::save(item->data(GameRole).toInt()));
		delete item;
		m_accept_button->setEnabled(m_games->count() > 0);
	};
}

//-----------------------------------------------------------------------------
