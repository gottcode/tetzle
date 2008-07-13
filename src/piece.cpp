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

#include "piece.h"

#include "board.h"
#include "tile.h"

#include <QSet>

#include <cmath>

/*****************************************************************************/

Piece::Piece(int rotation, const QPoint& pos, Board* board)
:	m_rotation(rotation),
	m_pos(pos),
	m_rect(0, 0, 0, 0),
	m_board(board)
{
}

/*****************************************************************************/

Piece::~Piece()
{
	qDeleteAll(m_children);
}

/*****************************************************************************/

QPoint Piece::scenePos() const
{
	return m_pos;
}

/*****************************************************************************/

QRect Piece::marginRect() const
{
	int margin = m_board->margin();
	return boundingRect().adjusted(-margin, -margin, margin, margin);
}

/*****************************************************************************/

void Piece::rotateAround(Tile* tile)
{
	m_rect.setRect(-m_rect.bottom() - 1 + m_board->tileSize(), m_rect.left(), m_rect.height(), m_rect.width());

	if (tile) {
		QPoint pos = tile->scenePos() - scenePos();
		m_pos += QPoint(pos.x() + pos.y(), pos.y() - pos.x());
	}

	m_rotation += 1;
	if (m_rotation > 3)
		m_rotation = 0;

	QPoint pos;
	foreach (Tile* tile, m_children) {
		pos = tile->pos();
		qSwap(pos.rx(), pos.ry());
		pos.setX(-pos.x());
		tile->setPos(pos);
	}
}

/*****************************************************************************/

void Piece::attach(Tile* tile)
{
	tile->setParent(this);
	m_children.append(tile);
	m_rect = m_rect.united(tile->rect().translated(tile->pos()));
}

/*****************************************************************************/

void Piece::attach(Piece* piece)
{
	Q_ASSERT(piece != this);

	QPoint delta = piece->m_pos - m_pos;

	// Change parent of piece's children
	QList<Tile*> tiles = piece->m_children;
	foreach (Tile* tile, tiles) {
		tile->setPos(tile->pos() + delta);
		tile->setParent(this);
	}
	m_children += tiles;
	piece->m_children.clear();

	// Update bounding rectangle
	m_rect = m_rect.united(piece->m_rect.translated(delta));

	// Remove old parent
	m_board->reparent(piece);
}

/*****************************************************************************/

void Piece::attachNeighbors()
{
	int margin = m_board->margin();

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
	QSet<Piece*> closest_tiles;
	QPoint delta;
	foreach (Tile* child, m_children) {
	foreach (Piece* piece, m_board->findCollidingPieces(child->parent())) {
		if (piece->m_rotation != m_rotation)
			continue;

		foreach (Tile* tile, piece->children()) {
			// Determine which side the tile is on
			delta = tile->scenePos() - child->scenePos();
			if (tile->row() == child->row()) {
				if (tile->column() == child->column() - 1) {
					// Left
					delta -= left;
					if (fabs(delta.x()) <= margin && fabs(delta.y()) <= margin) {
						closest_tiles.insert(piece);
						piece->moveBy(-delta);
					}
				} else if (tile->column() == child->column() + 1) {
					// Right
					delta -= right;
					if (fabs(delta.x()) <= margin && fabs(delta.y()) <= margin) {
						closest_tiles.insert(piece);
						piece->moveBy(-delta);
					}
				}
			} else if (tile->column() == child->column()) {
				if (tile->row() == child->row() - 1) {
					// Above
					delta -= above;
					if (fabs(delta.x()) <= margin && fabs(delta.y()) <= margin) {
						closest_tiles.insert(piece);
						piece->moveBy(-delta);
					}
				} else if (tile->row() == child->row() + 1) {
					// Below
					delta -= below;
					if (fabs(delta.x()) <= margin && fabs(delta.y()) <= margin) {
						closest_tiles.insert(piece);
						piece->moveBy(-delta);
					}
				}
			}
		}
	}
	}

	// Attach to closest tiles
	foreach (Piece* piece, closest_tiles) {
		attach(piece);
	}
}

/*****************************************************************************/

void Piece::pushNeighbors(Piece* immobile)
{
	QRect bounds = boundingRect();
	QPoint vector;
	float angle;
	QRect region;
	while (Piece* neighbor = m_board->findCollidingPiece(this)) {
		if (!m_rect.intersects(neighbor->m_rect))
			continue;

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

void Piece::save(QXmlStreamWriter& xml) const
{
	xml.writeStartElement("group");
	xml.writeAttribute("rotation", QString::number(m_rotation));

	m_children.at(0)->save(xml, true);
	for (int i = 1; i < m_children.count(); ++i)
		m_children.at(i)->save(xml);

	xml.writeEndElement();
}

/*****************************************************************************/
