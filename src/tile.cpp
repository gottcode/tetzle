/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011 Graeme Gott <graeme@gottcode.org>
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

#include "tile.h"

#include "board.h"
#include "piece.h"

//-----------------------------------------------------------------------------

Tile::Tile(int column, int row)
	: m_parent(0),
	m_column(column),
	m_row(row),
	m_pos(column * size, row * size),
	m_bevel(0),
	m_bevel_coords(-1,-1)
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
