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

inline float roundUp(float value)
{
	return value >= 0.0f ? ceil(value) : floor(value);
}

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
	return m_rect.translated(m_pos).adjusted(-margin, -margin, margin, margin);
}

/*****************************************************************************/

bool Piece::collidesWith(const Piece * other) const
{
	return marginRect().intersects(other->boundingRect());
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
	m_board->removePiece(piece);
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
	QSet<Piece*> closest_pieces;
	QPoint delta;
	int row, column;
	foreach (Piece* piece, m_board->findCollidingPieces(this)) {
		if (piece->m_rotation != m_rotation)
			continue;

		foreach (Tile* child, m_children) {
			foreach (Tile* tile, piece->children()) {
				delta = tile->scenePos() - child->scenePos();

				// Determine which neighbor the child is of the tile
				column = tile->column() - child->column() + 2;
				row = tile->row() - child->row() + 2;
				switch ((column * 1000) + row) {
				case 1002:
					delta -= left;
					break;
				case 3002:
					delta -= right;
					break;
				case 2001:
					delta -= above;
					break;
				case 2003:
					delta -= below;
					break;
				default:
					continue;
				}

				if (delta.manhattanLength() <= margin) {
					closest_pieces.insert(piece);
					piece->moveBy(-delta);
					break;
				}
			}
		}
	}

	// Attach to closest pieces
	foreach (Piece* piece, closest_pieces) {
		attach(piece);
	}
}

/*****************************************************************************/

void Piece::pushNeighbors(const QPointF& inertia)
{
	while (Piece* neighbor = m_board->findCollidingPiece(this)) {
		// Determine which piece to move
		Piece *source, *target;
		if (m_rect.width() >= neighbor->m_rect.width() || m_rect.height() >= neighbor->m_rect.height()) {
			source = this;
			target = neighbor;
		} else {
			source = neighbor;
			target = this;
		}
		QRect source_rect = source->marginRect();

		// Calculate valid movement vector for target; preserve some motion from last move
		QPointF vector = target->boundingRect().center() - source_rect.center() + inertia;
		while (abs(vector.x()) + abs(vector.y()) < 1)
			vector = QPointF(rand() - (RAND_MAX/2), rand() - (RAND_MAX/2));

		// Scale movement vector so that the largest dimension is 1
		QPointF direction = vector / qMax(abs(vector.x()), abs(vector.y()));

		// Push target until it is clear from current source
		// We use a binary-search, pushing away if collision, retracting otherwise
		QPoint orig = target->position();
		QRect united = source_rect.united(target->boundingRect());
		float min = 0.0f;
		float max = united.width() * united.height();
		while (true) {
			float test = (min + max) / 2.0f;
			float x = orig.x() + roundUp(test * direction.x());
			float y = orig.y() + roundUp(test * direction.y());
			target->moveTo( + x, y);
			if (source->collidesWith(target))
				min = test;
			else {
				max = test;
				if (max - min < 1.0f)
					break;
				Q_ASSERT(max - min > 0.01f);
			}
		}
		Q_ASSERT(min < max);
		Q_ASSERT(!source->collidesWith(target));

		// Recurse, and keep inertia for stability.
		target->pushNeighbors(vector);
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
