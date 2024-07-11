/*
	SPDX-FileCopyrightText: 2011-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "choose_game_dialog.h"

#include "add_image.h"
#include "new_game_tab.h"
#include "open_game_tab.h"
#include "path.h"

#include <QDir>
#include <QSettings>
#include <QTabBar>
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
	m_tabs->setDocumentMode(true);
	m_tabs->tabBar()->setExpanding(true);

	// Layout dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_tabs);
	resize(QSettings().value("ChooseGame/Size", QSize(800, 600)).toSize());

	// Create tabs
	OpenGameTab* open_game_tab = new OpenGameTab(current_id, this);
	connect(open_game_tab, &OpenGameTab::openGame, this, &ChooseGameDialog::accept);
	connect(open_game_tab, &OpenGameTab::openGame, this, &ChooseGameDialog::openGame);
	m_tabs->addTab(open_game_tab, tr("Current Games"));

	m_new_game_tab = new NewGameTab(files, this);
	connect(m_new_game_tab, &NewGameTab::newGame, this, &ChooseGameDialog::accept);
	connect(m_new_game_tab, &NewGameTab::newGame, this, &ChooseGameDialog::newGame);
	m_tabs->addTab(m_new_game_tab, tr("New Game"));

	connect(m_new_game_tab, &NewGameTab::imageRenamed, open_game_tab, &OpenGameTab::imageRenamed);

	if (!files.isEmpty() || currentGames().count() <= (current_id != 0)) {
		m_tabs->setCurrentIndex(1);
	}
}

//-----------------------------------------------------------------------------

QStringList ChooseGameDialog::currentGames()
{
	QStringList result;

	const QStringList files = QDir(Path::saves(), "*.xml").entryList(QDir::Files, QDir::Time);
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
		const QXmlStreamAttributes attributes = xml.attributes();
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
