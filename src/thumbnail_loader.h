/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef THUMBNAIL_LOADER_H
#define THUMBNAIL_LOADER_H

struct Thumbnail;

#include <QAtomicInt>
#include <QMutex>
#include <QThread>
class QListWidget;
class QListWidgetItem;

class ThumbnailLoader : public QThread
{
	Q_OBJECT

	ThumbnailLoader(QObject* parent = 0);
public:
	~ThumbnailLoader();

	static QListWidgetItem* createItem(const QString& image, const QString& text, QListWidget* list, qreal pixelratio);

signals:
	void loaded(const Thumbnail& details);

protected:
	virtual void run();

private slots:
	void imageLoaded(const Thumbnail& details);

private:
	bool m_done;
	QList<Thumbnail> m_details;
	QMutex m_mutex;
};

#endif
