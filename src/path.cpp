/*
	SPDX-FileCopyrightText: 2010 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "path.h"

#include "board.h"

#include <QGuiApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QStandardPaths>
#include <QStringList>

//-----------------------------------------------------------------------------

QString Path::m_path;

//-----------------------------------------------------------------------------

void Path::load(const QString& userdir)
{
	m_path = userdir;

	// Find user data dir if not in portable mode
	if (m_path.isEmpty()) {
		m_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

		// Migrate data from old location
		if (!QFileInfo::exists(m_path)) {
			const QString oldpath = oldDataPath();
			if (!oldpath.isEmpty()) {
				QDir dir(m_path + "/../");
				dir.mkpath(dir.absolutePath());
				dir.rename(oldpath, m_path);
			}
		}
	}

	// Create data location
	QDir dir(m_path);
	if (!dir.exists()) {
		dir.mkpath(dir.absolutePath());
	}
	dir.mkpath(m_path + "/saves/");

	// Update thumbnails
	if (dir.exists("previews")) {
		QLabel label(Board::tr("Please Wait"), nullptr, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
		label.setWindowTitle(QGuiApplication::applicationDisplayName());
		label.setAlignment(Qt::AlignCenter);
		label.setContentsMargins(10, 10, 10, 10);
		label.setFixedSize(label.sizeHint());
		label.show();
		QCoreApplication::processEvents();

		const QStringList old_dirs{
			m_path + "/previews/",
			m_path + "/images/thumbnails/",
		};
		for (const QString& old_dir : old_dirs) {
			dir.setPath(old_dir);
			dir.removeRecursively();
		}
	}
	dir.mkpath(m_path + "/images/thumbnails/");
}

//-----------------------------------------------------------------------------

QString Path::image(const QString& file)
{
	return images() + file;
}

//-----------------------------------------------------------------------------

QString Path::thumbnail(const QString& image, qreal pixelratio)
{
	QString pixel;
	if (pixelratio > 1.0) {
		pixel = QString("@%1x").arg(pixelratio);
	}
	return thumbnails() + image + pixel + ".png";
}

//-----------------------------------------------------------------------------

QString Path::save(const QString& file)
{
	return saves() + file;
}

//-----------------------------------------------------------------------------

QString Path::save(int game)
{
	return save(QString::number(game) + ".xml");
}

//-----------------------------------------------------------------------------

QString Path::images()
{
	return m_path + "/images/";
}

//-----------------------------------------------------------------------------

QString Path::thumbnails()
{
	return m_path + "/images/thumbnails/";
}

//-----------------------------------------------------------------------------

QString Path::saves()
{
	return m_path + "/saves/";
}

//-----------------------------------------------------------------------------

QString Path::oldDataPath()
{
	QStringList oldpaths;
	QString oldpath;

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	// Data path from Qt 4 version of 2.0
	oldpaths.append(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/data/GottCode/Tetzle");
#endif

	// Data path from 1.0
#if defined(Q_OS_MAC)
	oldpath = QDir::homePath() + "/Library/Application Support/GottCode/Tetzle/";
#elif defined(Q_OS_UNIX)
	oldpath = QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME"));
	if (oldpath.isEmpty()) {
		oldpath = QDir::homePath() + "/.local/share/";
	}
	oldpath += "/games/tetzle/";
#elif defined(Q_OS_WIN32)
	oldpath = QDir::homePath() + "/Application Data/GottCode/Tetzle/";
#endif
	if (!oldpaths.contains(oldpath)) {
		oldpaths.append(oldpath);
	}

	// Check if an old data location exists
	oldpath.clear();
	for (const QString& testpath : std::as_const(oldpaths)) {
		if (QFileInfo::exists(testpath)) {
			oldpath = testpath;
			break;
		}
	}

	return oldpath;
}

//-----------------------------------------------------------------------------
