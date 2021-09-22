/*
	SPDX-FileCopyrightText: 2008-2011 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "tile.h"

#include "board.h"
#include "piece.h"

//-----------------------------------------------------------------------------

Tile::Tile(int column, int row)
	: m_parent(0)
	, m_column(column)
	, m_row(row)
	, m_pos(column * size, row * size)
	, m_bevel(0)
	, m_bevel_coords(-1,-1)
{
}

//-----------------------------------------------------------------------------

QPoint Tile::scenePos() const
{
	return m_pos + m_parent->scenePos();
}

//-----------------------------------------------------------------------------

void Tile::rotate()
{
	qSwap(m_pos.rx(), m_pos.ry());
	m_pos.setX(-m_pos.x() + m_parent->boundingRect().width() - Tile::size);
	m_bevel_coords.setX( (m_bevel_coords.x() > 0.1) ? (m_bevel_coords.x() - 0.25) : 0.8125 );
}

//-----------------------------------------------------------------------------

void Tile::setBevel(int bevel)
{
	m_bevel = qBound(0, bevel, 15);
	int col = m_bevel % 4;
	int row = m_bevel / 4;
	m_bevel_coords = QPointF(col * 0.25 + 0.0625, row * 0.25 + 0.0625);
}

//-----------------------------------------------------------------------------

void Tile::save(QXmlStreamWriter& xml) const
{
	QXmlStreamAttributes attributes;
	attributes.append("column", QString::number(m_column));
	attributes.append("row", QString::number(m_row));
	if (m_bevel) {
		attributes.append("bevel", QString::number(m_bevel));
	}
	xml.writeEmptyElement("tile");
	xml.writeAttributes(attributes);
}

//-----------------------------------------------------------------------------
