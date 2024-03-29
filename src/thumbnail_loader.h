/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_THUMBNAIL_LOADER_H
#define TETZLE_THUMBNAIL_LOADER_H

struct Thumbnail;

#include <QAtomicInt>
#include <QMutex>
#include <QThread>
class QListWidget;
class QListWidgetItem;

class ThumbnailLoader : public QThread
{
	Q_OBJECT

	explicit ThumbnailLoader(QObject* parent = nullptr);

public:
	~ThumbnailLoader();

	static QListWidgetItem* createItem(const QString& image, const QString& text, QListWidget* list, qreal pixelratio);

Q_SIGNALS:
	void loaded(const Thumbnail& details);

protected:
	void run() override;

private Q_SLOTS:
	void imageLoaded(const Thumbnail& details);

private:
	bool m_done;
	QList<Thumbnail> m_details;
	QMutex m_mutex;
};

#endif // TETZLE_THUMBNAIL_LOADER_H
