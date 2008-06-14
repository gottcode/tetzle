/***********************************************************************
 *
 * Copyright (C) 2008 Graeme Gott <graeme@gottcode.org>
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

#include "thumbnail.h"

#include <QPainter>

/*****************************************************************************/

Thumbnail::Thumbnail(const QString& image, const QString& thumbnail)
:	m_image(image),
	m_thumbnail(thumbnail)
{
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

/*****************************************************************************/

void Thumbnail::run()
{
	QImage source(m_image);
	if (source.width() > 92 || source.height() > 92) {
		source = source.scaled(92, 92, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	QImage thumbnail(100, 100, QImage::Format_ARGB32);
	thumbnail.fill(0);
	{
		QPainter painter(&thumbnail);
		painter.translate(46 - (source.width() / 2), 46 - (source.height() / 2));
		painter.fillRect(0, 0, source.width() + 8, source.height() + 8, QColor(0, 0, 0, 50));
		painter.fillRect(1, 1, source.width() + 6, source.height() + 6, QColor(0, 0, 0, 75));
		painter.fillRect(2, 2, source.width() + 4, source.height() + 4, Qt::white);
		painter.drawImage(4, 4, source, 0, 0, -1, -1, Qt::AutoColor | Qt::AvoidDither);
	}
	thumbnail.save(m_thumbnail, 0, 0);

	emit generated(m_image);
}

/*****************************************************************************/
