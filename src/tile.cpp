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

#include "tile.h"

#include "board.h"

#include <QMouseEvent>
#include <QSet>

#include <cmath>

/*****************************************************************************/

Tile::Tile(int column, int row, int rotation, const QPoint& pos, Tile* parent, Board* board)
:	m_column(column),
	m_row(row),
	m_rotation(rotation),
	m_pos(pos),
	m_rect(0, 0, board->tileSize(), board->tileSize()),
	m_parent(this),
	m_board(board)
{
	if (parent) {
		m_pos += parent->m_pos;
		parent->attach(this);
	}
}

/*****************************************************************************/

Tile::~Tile()
{
	qDeleteAll(m_children);
}

/*****************************************************************************/

QPoint Tile::scenePos() const
{
	if (parent() != this)
		return m_pos + parent()->m_pos;
	else
		return m_pos;
}

/*****************************************************************************/

QRect Tile::rect() const
{
	return QRect(0, 0, m_board->tileSize(), m_board->tileSize()).translated(scenePos());
}

/*****************************************************************************/

void Tile::rotateAround(Tile* tile)
{
	Q_ASSERT(parent() == this);

	m_rect.setRect(-m_rect.bottom() - 1 + m_board->tileSize(), m_rect.left(), m_rect.height(), m_rect.width());

	if (tile) {
		QPoint pos = tile->scenePos() - scenePos();
		m_pos += QPoint(pos.x() + pos.y(), pos.y() - pos.x());
	}

	m_rotation += 1;
	if (m_rotation > 3)
		m_rotation = 0;

	foreach (Tile* tile, m_children) {
		qSwap(tile->m_pos.rx(), tile->m_pos.ry());
		tile->m_pos.setX(-tile->m_pos.x());
		tile->m_rotation = m_rotation;
	}
}

/*****************************************************************************/

void Tile::attach(Tile* tile)
{
	Q_ASSERT(parent() == this);
	Q_ASSERT(tile->parent() == tile);
	Q_ASSERT(tile != this);

	// Change parent of tile's children
	QList<Tile*> tiles = tile->m_children;
	tiles.append(tile);
	QPoint pos;
	foreach (Tile* item, tiles) {
		item->m_pos = item->scenePos() - scenePos();
		m_board->reparent(item);
		item->m_parent = this;
	}
	m_children += tiles;
	tile->m_children.clear();

	// Update bounding rectangle
	m_rect = m_rect.united(tile->m_rect.translated(tile->m_pos));
}

/*****************************************************************************/

void Tile::attachNeighbors(int region)
{
	Q_ASSERT(parent() == this);

	// Create offset vectors
	int cos_size = 0;
	int sin_size = 0;
	switch (m_rotation) {
	case 0:
		cos_size = m_board->tileSize();
		break;
	case 1:
		sin_size = -m_board->tileSize();
		break;
	case 2:
		cos_size = -m_board->tileSize();
		break;
	case 3:
		sin_size = m_board->tileSize();
		break;
	}
	QPoint left(-cos_size, sin_size);
	QPoint right(cos_size, -sin_size);
	QPoint above(-sin_size, -cos_size);
	QPoint below(sin_size, cos_size);

	// Find closest tiles
	QSet<Tile*> closest_tiles;
	QPoint delta;
	QList<Tile*> tiles = m_children;
	tiles.append(this);
	foreach (Tile* child, tiles) {
		foreach (Tile* tile, m_board->collidingItems(child)) {
			if (tile->m_rotation != m_rotation)
				continue;

			// Determine which side the tile is on
			delta = tile->scenePos() - child->scenePos();
			if (tile->m_row == child->m_row) {
				if (tile->m_column == child->m_column - 1) {
					// Left
					delta -= left;
					if (fabs(delta.x()) <= region && fabs(delta.y()) <= region) {
						tile = tile->parent();
						closest_tiles.insert(tile);
						tile->m_pos -= delta;
					}
				} else if (tile->m_column == child->m_column + 1) {
					// Right
					delta -= right;
					if (fabs(delta.x()) <= region && fabs(delta.y()) <= region) {
						tile = tile->parent();
						closest_tiles.insert(tile);
						tile->m_pos -= delta;
					}
				}
			} else if (tile->m_column == child->m_column) {
				if (tile->m_row == child->m_row - 1) {
					// Above
					delta -= above;
					if (fabs(delta.x()) <= region && fabs(delta.y()) <= region) {
						tile = tile->parent();
						closest_tiles.insert(tile);
						tile->m_pos -= delta;
					}
				} else if (tile->m_row == child->m_row + 1) {
					// Below
					delta -= below;
					if (fabs(delta.x()) <= region && fabs(delta.y()) <= region) {
						tile = tile->parent();
						closest_tiles.insert(tile);
						tile->m_pos -= delta;
					}
				}
			}
		}
	}

	// Attach to closest tiles
	foreach (Tile* tile, closest_tiles) {
		attach(tile);
	}
}

/*****************************************************************************/

void Tile::pushNeighbors(Tile* immobile)
{
	Q_ASSERT(parent() == this);

	QRect bounds = boundingRect();
	QPoint vector;
	float angle;
	QRect region;
	QSet<Tile*> pieces;

	foreach (Tile* neighbor, m_board->collidingItems(this))
		pieces.insert(neighbor->parent());

	foreach (Tile* neighbor, pieces) {
		region = bounds.intersected(neighbor->boundingRect());
		if (neighbor != immobile) {
			vector = bounds.center() - neighbor->boundingRect().center();
		} else {
			neighbor = this;
			vector = immobile->boundingRect().center() - bounds.center();
		}
		angle = 180.f - ((atan2(vector.y(), vector.x()) * 180.f) / 3.141593f);

		if (angle < 45.0f || angle >= 315.0f) {
			neighbor->moveBy(QPoint(region.width() + 10, 0));
		} else if (angle < 135.0f) {
			neighbor->moveBy(QPoint(0, -region.height() - 10));
		} else if (angle < 225.0f) {
			neighbor->moveBy(QPoint(-region.width() - 10, 0));
		} else if (angle < 315.0f) {
			neighbor->moveBy(QPoint(0, region.height() + 10));
		}
		neighbor->pushNeighbors(immobile);
	}
}

/*****************************************************************************/

void Tile::save(QXmlStreamWriter& xml) const
{
	// Begin group
	if (parent() == this) {
		xml.writeStartElement("group");
		xml.writeAttribute("rotation", QString::number(m_rotation));
	}

	// Write tile details
	QXmlStreamAttributes attributes;
	attributes.append("column", QString::number(m_column));
	attributes.append("row", QString::number(m_row));
	attributes.append("x", QString::number(m_pos.x()));
	attributes.append("y", QString::number(m_pos.y()));
	xml.writeEmptyElement("tile");
	xml.writeAttributes(attributes);

	// End group
	if (parent() == this) {
		foreach (Tile* child, m_children)
			child->save(xml);
		xml.writeEndElement();
	}
}

/*****************************************************************************/
