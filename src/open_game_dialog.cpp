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

#include "open_game_dialog.h"

#include "add_image.h"
#include "path.h"
#include "thumbnail_list.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>
#include <QXmlStreamReader>

//-----------------------------------------------------------------------------

OpenGameDialog::OpenGameDialog(int current_id, QWidget* parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setWindowTitle(tr("Open Game"));
	setAcceptDrops(true);

	// List saved games
	m_games = new ThumbnailList(this);
	m_games->setSpacing(2);
	QXmlStreamReader xml;
	QXmlStreamAttributes attributes;
	QStringList files = games();
	foreach (QString game, files) {
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
		if (xml.name() == QLatin1String("tetzle") && attributes.value("version").toString().toUInt() == 5) {
			QString image = attributes.value("image").toString();
			if (!QFileInfo(Path::image(image)).exists()) {
				continue;
			}
			int pieces = attributes.value("pieces").toString().toInt();
			int complete = attributes.value("complete").toString().toInt();
			QListWidgetItem* item = m_games->addImage(Path::image(image));
			item->setText(tr("%L1 pieces\n%2% complete").arg(pieces).arg(complete));
			item->setData(Qt::UserRole, id);
		}
	}
	m_games->setCurrentRow(0);

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(this);
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QPushButton* cancel_button = 0;
	if (current_id == 0) {
		cancel_button = new QPushButton(tr("New"), this);
		connect(cancel_button, SIGNAL(clicked()), this, SLOT(hide()));
		connect(cancel_button, SIGNAL(clicked()), this, SIGNAL(newGame()));
	} else {
		cancel_button = new QPushButton(tr("Cancel"), this);
	}
	cancel_button->setAutoDefault(false);
	buttons->addButton(cancel_button, QDialogButtonBox::RejectRole);

	m_accept_button = new QPushButton(tr("Open"), this);
	m_accept_button->setDefault(true);
	m_accept_button->setEnabled(m_games->count() > 0);
	buttons->addButton(m_accept_button, QDialogButtonBox::AcceptRole);

	QPushButton* delete_button = new QPushButton(tr("Delete"), this);
	delete_button->setAutoDefault(false);
	connect(delete_button, SIGNAL(clicked()), this, SLOT(deleteGame()));
	buttons->addButton(delete_button, QDialogButtonBox::ActionRole);

	// Arrange dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSpacing(24);
	layout->addWidget(m_games);
	layout->addWidget(buttons);

	// Resize dialog
	resize(QSettings().value("OpenGame/Size", sizeHint()).toSize());
}

//-----------------------------------------------------------------------------

QStringList OpenGameDialog::games()
{
	QStringList result;

	QXmlStreamReader xml;
	QXmlStreamAttributes attributes;

	QStringList files = QDir(Path::saves(), "*.xml").entryList(QDir::Files, QDir::Time);
	foreach (QString game, files) {
		// Load XML file
		QFile file(Path::save(game));
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		xml.setDevice(&file);

		// Load version
		while (!xml.isStartElement()) {
			xml.readNext();
		}
		attributes = xml.attributes();
		if (xml.name() == QLatin1String("tetzle") && attributes.value("version").toString().toUInt() == 5) {
			if (QFileInfo(Path::image(attributes.value("image").toString())).exists()) {
				result.append(game);
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

void OpenGameDialog::accept()
{
	QDialog::accept();

	QListWidgetItem* item = m_games->currentItem();
	if (item) {
		emit openGame(item->data(Qt::UserRole).toInt());
	}
}

//-----------------------------------------------------------------------------

void OpenGameDialog::dragEnterEvent(QDragEnterEvent* event)
{
	AddImage::dragEnterEvent(event);
}

//-----------------------------------------------------------------------------

void OpenGameDialog::dropEvent(QDropEvent* event)
{
	QStringList files = AddImage::dropEvent(event);
	if (!files.isEmpty()) {
		reject();
		emit newGame(files);
	}
}

//-----------------------------------------------------------------------------

void OpenGameDialog::hideEvent(QHideEvent* event)
{
	QSettings().setValue("OpenGame/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------

void OpenGameDialog::deleteGame()
{
	QListWidgetItem* item = m_games->currentItem();
	if (!item) {
		return;
	}

	if (QMessageBox::question(this, tr("Delete Game"), tr("Delete selected game?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
		QFile::remove(Path::save(item->data(Qt::UserRole).toInt()));
		delete item;
		m_accept_button->setEnabled(m_games->count() > 0);
	};
}

//-----------------------------------------------------------------------------
