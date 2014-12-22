/***********************************************************************
 *
 * Copyright (C) 2011, 2014 Graeme Gott <graeme@gottcode.org>
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

#include "choose_game_dialog.h"

#include "add_image.h"
#include "new_game_tab.h"
#include "open_game_tab.h"
#include "path.h"

#include <QDir>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QXmlStreamReader>

//-----------------------------------------------------------------------------

ChooseGameDialog::ChooseGameDialog(const QStringList& files, int current_id, QWidget* parent)
	: QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setWindowTitle(tr("Choose Game"));
	setAcceptDrops(true);

	m_tabs = new QTabWidget(this);

	// Layout dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_tabs);
	resize(QSettings().value("ChooseGame/Size", QSize(800, 600)).toSize());

	// Create tabs
	OpenGameTab* open_game_tab = new OpenGameTab(current_id, this);
	connect(open_game_tab, SIGNAL(openGame(int)), this, SLOT(accept()));
	connect(open_game_tab, SIGNAL(openGame(int)), this, SIGNAL(openGame(int)));
	m_tabs->addTab(open_game_tab, tr("Current Games"));

	m_new_game_tab = new NewGameTab(files, this);
	connect(m_new_game_tab, SIGNAL(newGame(const QString&, int)), this, SLOT(accept()));
	connect(m_new_game_tab, SIGNAL(newGame(const QString&, int)), this, SIGNAL(newGame(const QString&, int)));
	m_tabs->addTab(m_new_game_tab, tr("New Game"));

	connect(m_new_game_tab, SIGNAL(imageRenamed(const QString&, const QString&)), open_game_tab, SLOT(imageRenamed(const QString&, const QString&)));

	if (!files.isEmpty() || currentGames().count() <= (current_id != 0)) {
		m_tabs->setCurrentIndex(1);
	}
}

//-----------------------------------------------------------------------------

QStringList ChooseGameDialog::currentGames()
{
	QStringList result;

	QStringList files = QDir(Path::saves(), "*.xml").entryList(QDir::Files, QDir::Time);
	for (const QString& game : files) {
		// Load XML file
		QFile file(Path::save(game));
		if (!file.open(QIODevice::ReadOnly)) {
			continue;
		}
		QXmlStreamReader xml(&file);

		// Load version
		while (!xml.isStartElement()) {
			xml.readNext();
		}
		QXmlStreamAttributes attributes = xml.attributes();
		if (xml.name() == QLatin1String("tetzle") && attributes.value("version").toString().toUInt() <= 5) {
			if (QFile::exists(Path::image(attributes.value("image").toString()))) {
				result.append(game);
			}
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

void ChooseGameDialog::dragEnterEvent(QDragEnterEvent* event)
{
	AddImage::dragEnterEvent(event);
}

//-----------------------------------------------------------------------------

void ChooseGameDialog::dropEvent(QDropEvent* event)
{
	m_tabs->setCurrentIndex(1);
	m_new_game_tab->addImages(AddImage::dropEvent(event));
}

//-----------------------------------------------------------------------------

void ChooseGameDialog::hideEvent(QHideEvent* event)
{
	QSettings().setValue("ChooseGame/Size", size());
	QDialog::hideEvent(event);
}

//-----------------------------------------------------------------------------
