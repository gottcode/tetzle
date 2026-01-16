/*
	SPDX-FileCopyrightText: 2008 Graeme Gott <graeme@gottcode.org>

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

/**
 * Thread to load thumbnails.
 */
class ThumbnailLoader : public QThread
{
	Q_OBJECT

	/**
	 * Construct a thumbnail loading thread.
	 *
	 * @param parent the parent object of the thread
	 */
	explicit ThumbnailLoader(QObject* parent = nullptr);

public:
	/**
	 * Clean up thumbnail loading thread.
	 */
	~ThumbnailLoader();

	/**
	 * Create thumbnail for an image. This will start the loading thread if it is not running.
	 *
	 * @param image the image to use
	 * @param text the text of the thumbnail item
	 * @param list the listwidget for the thumbnail item
	 * @param pixelratio the pixel ratio to render at
	 */
	static QListWidgetItem* createItem(const QString& image, const QString& text, QListWidget* list, qreal pixelratio);

Q_SIGNALS:
	/**
	 * Signal that a thumbnail has been created.
	 *
	 * @param details the details of the thumbnail
	 */
	void loaded(const Thumbnail& details);

protected:
	/**
	 * Create thumbnails.
	 */
	void run() override;

private Q_SLOTS:
	/**
	 * Set image for thumbnail item.
	 *
	 * @param details the details of the thumbnail
	 */
	void imageLoaded(const Thumbnail& details);

private:
	bool m_done; ///< track if cancelled
	QList<Thumbnail> m_details; ///< list of thumbnails to load
	QMutex m_mutex; ///< mutex to protect list of thumbnails
};

#endif // TETZLE_THUMBNAIL_LOADER_H
