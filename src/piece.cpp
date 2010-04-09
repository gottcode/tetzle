/***********************************************************************
 *
 * Copyright (C) 2008, 2010 Graeme Gott <graeme@gottcode.org>
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

#ifndef GL_CLAMP_TO_EDGE
 #define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace
{

//-----------------------------------------------------------------------------

// Lightwheight Piece-lookalike.
// Used for collision detection. Supports slicing in half to sub-pieces.
// Collision-detection works by slicing and bounds-checking halves until either
// no collision, or we've found single Tiles that actually collide
struct PieceHelper
{
	QRect m_rect;
	QList<Tile*> m_children;
	const Piece* m_piece;
	const Board* m_board;

	PieceHelper(const PieceHelper& p)
	{
		m_piece = p.m_piece;
		m_board = p.m_board;
	}

	PieceHelper(const Piece* piece, const Board* board)
	{
		m_piece = piece;
		m_board = board;
		m_children = m_piece->children();
		foreach (Tile* tile, m_children) {
			m_rect = m_rect.united(tile->rect().translated(tile->pos()).translated(m_piece->scenePos()));
		}
	}

	void split(PieceHelper& a, PieceHelper& b) const
	{
		Q_ASSERT(m_children.size() > 1);
		bool is_fat = m_rect.width() > m_rect.height();
		int splitline;
		if (is_fat) {
			splitline = m_rect.center().x() - m_piece->scenePos().x();
		} else {
			splitline = m_rect.center().y() - m_piece->scenePos().y();
		}

		foreach (Tile* tile, m_children) {
			PieceHelper* target = &a;
			if (is_fat) {
				if (tile->pos().x() >= splitline) {
					target = &b;
				}
			} else {
				if (tile->pos().y() >= splitline) {
					target = &b;
				}
			}
			target->m_children.append(tile);
			target->m_rect = target->m_rect.united(tile->rect().translated(tile->pos()).translated(m_piece->scenePos()));
		}
	}

	bool collidesWith(const PieceHelper& other) const
	{
		int margin = m_board->margin();
		QRect other_rect = other.m_rect.adjusted(-margin, -margin, margin, margin);
		if (m_children.size() <= 1) {
			if (other.m_children.size() <= 1) {
				return m_rect.intersects(other_rect);
			} else {
				return other.collidesWith(*this);
			}
		} else {
			PieceHelper a(*this), b(*this);
			split(a,b);
			if (a.m_rect.intersects(other_rect)) {
				if (other.collidesWith(a)) {
					return true;
				}
			}
			if (b.m_rect.intersects(other_rect)) {
				if (other.collidesWith(b)) {
					return true;
				}
			}
		}
		return false;
	}
};

//-----------------------------------------------------------------------------

inline float roundUp(float value)
{
	return value >= 0.0f ? ceil(value) : floor(value);
}

//-----------------------------------------------------------------------------

}

//-----------------------------------------------------------------------------

Piece::Piece(const QPoint& pos, int rotation, const QList<Tile*>& tiles, Board* board)
	: m_board(board),
	m_rotation(0),
	m_pos(pos),
	m_children(tiles),
	m_shadow(tiles)
{
	// Add tiles
	QPoint first_pos = m_children.first()->pos();
	foreach (Tile* tile, m_children) {
		tile->setParent(this);
		tile->setPos(tile->pos() - first_pos);
		m_rect = m_rect.united(tile->rect().translated(tile->pos()));
	}
	updateShadow();

	// Rotate
	Tile* tile = m_children.first();
	for (int i = 0; i < rotation; ++i) {
		rotateAround(tile);
	}
}

//-----------------------------------------------------------------------------

Piece::~Piece()
{
	qDeleteAll(m_children);
}

//-----------------------------------------------------------------------------

bool Piece::collidesWith(const Piece * other) const
{
	if (marginRect().intersects(other->boundingRect())) {
		PieceHelper a(this, this->m_board), b(other, other->m_board);
		bool retVal = a.collidesWith(b);
		return retVal;
	} else {
		return false;
	}
}

//-----------------------------------------------------------------------------

QRect Piece::marginRect() const
{
	int margin = m_board->margin();
	return m_rect.translated(m_pos).adjusted(-margin, -margin, margin, margin);
}

//-----------------------------------------------------------------------------

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

	// Update shadow
	m_shadow += piece->m_shadow;
	updateShadow();

	// Update bounding rectangle
	m_rect = m_rect.united(piece->m_rect.translated(delta));

	// Remove old parent
	m_board->removePiece(piece);
}

//-----------------------------------------------------------------------------

void Piece::attachNeighbors()
{
	int margin = m_board->margin();

	// Create offset vectors
	int cos_size = 0;
	int sin_size = 0;
	switch (m_rotation) {
	case 0:
		cos_size = Tile::size();
		break;

	case 1:
		sin_size = -Tile::size();
		break;

	case 2:
		cos_size = -Tile::size();
		break;

	case 3:
		sin_size = Tile::size();
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
		if (piece->m_rotation != m_rotation) {
			continue;
		}

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

//-----------------------------------------------------------------------------

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
		while (fabs(vector.x()) + fabs(vector.y()) < 1) {
			vector = QPointF(rand() - (RAND_MAX / 2), rand() - (RAND_MAX / 2));
		}

		// Scale movement vector so that the largest dimension is 1
		QPointF direction = vector / qMax(fabs(vector.x()), fabs(vector.y()));

		// Push target until it is clear from current source
		// We use a binary-search, pushing away if collision, retracting otherwise
		QPoint orig = target->scenePos();
		QRect united = source_rect.united(target->boundingRect());
		float min = 0.0f;
		float max = united.width() * united.height();
		while (true) {
			float test = (min + max) / 2.0f;
			float x = orig.x() + roundUp(test * direction.x());
			float y = orig.y() + roundUp(test * direction.y());
			target->moveTo(x, y);
			if (source->collidesWith(target)) {
				min = test;
			} else {
				max = test;
				if (max - min < 1.0f) {
					break;
				}
				Q_ASSERT(max - min > 0.01f);
			}
		}
		Q_ASSERT(min < max);
		Q_ASSERT(!source->collidesWith(target));

		// Recurse, and keep inertia for stability.
		target->pushNeighbors(vector);
	}
}

//-----------------------------------------------------------------------------

void Piece::rotateAround(Tile* tile)
{
	m_rect.setRect(-m_rect.bottom() - 1 + Tile::size(), m_rect.left(), m_rect.height(), m_rect.width());

	if (tile) {
		QPoint pos = tile->scenePos() - scenePos();
		m_pos += QPoint(pos.x() + pos.y(), pos.y() - pos.x());
	}

	m_rotation += 1;
	if (m_rotation > 3) {
		m_rotation = 0;
	}

	QPoint pos;
	foreach (Tile* tile, m_children) {
		pos = tile->pos();
		qSwap(pos.rx(), pos.ry());
		pos.setX(-pos.x());
		tile->setPos(pos);
	}
}

//-----------------------------------------------------------------------------

void Piece::draw() const
{
	int x1, y1, x2, y2;
	QPointF pos;
	Tile* tile;

	// Draw shadow
	glColor3f(0, 0, 0);
	glBindTexture(GL_TEXTURE_2D, m_board->shadowTexture());
	glBegin(GL_QUADS);
	for (int i = 0; i < m_shadow.count(); ++i) {
		pos = m_shadow.at(i)->scenePos();
		x1 = pos.x() - 16;
		y1 = pos.y() - 16;
		x2 = x1 + 64;
		y2 = y1 + 64;

		glTexCoord2i(0, 0);
		glVertex2i(x1, y1);

		glTexCoord2i(1, 0);
		glVertex2i(x2, y1);

		glTexCoord2i(1, 1);
		glVertex2i(x2, y2);

		glTexCoord2i(0, 1);
		glVertex2i(x1, y2);
	}
	glEnd();

	// Draw tiles
	glColor3f(1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, m_board->imageTexture());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBegin(GL_QUADS);
	for (int i = 0; i < m_children.count(); ++i) {
		tile = m_children.at(i);

		pos = tile->scenePos();
		x1 = pos.x();
		y1 = pos.y();
		x2 = x1 + Tile::size();
		y2 = y1 + Tile::size();

		const QPointF* corners = m_board->corners(rotation());
		float tx = tile->column() * m_board->tileTextureSize();
		float ty = tile->row() * m_board->tileTextureSize();

		glTexCoord2f(tx + corners[0].x(), ty + corners[0].y());
		glVertex2i(x1, y1);

		glTexCoord2f(tx + corners[1].x(), ty + corners[1].y());
		glVertex2i(x2, y1);

		glTexCoord2f(tx + corners[2].x(), ty + corners[2].y());
		glVertex2i(x2, y2);

		glTexCoord2f(tx + corners[3].x(), ty + corners[3].y());
		glVertex2i(x1, y2);
	}
	glEnd();
}

//-----------------------------------------------------------------------------

void Piece::save(QXmlStreamWriter& xml) const
{
	xml.writeStartElement("piece");
	xml.writeAttribute("x", QString::number(m_pos.x()));
	xml.writeAttribute("y", QString::number(m_pos.y()));
	xml.writeAttribute("rotation", QString::number(m_rotation));

	for (int i = 0; i < m_children.count(); ++i) {
		m_children.at(i)->save(xml);
	}

	xml.writeEndElement();
}

//-----------------------------------------------------------------------------

void Piece::updateShadow()
{
	QMutableListIterator<Tile*> i(m_shadow);
	while (i.hasNext()) {
		Tile* tile = i.next();
		if ( containsTile(tile->column() - 1, tile->row())
			&& containsTile(tile->column() + 1, tile->row())
			&& containsTile(tile->column(), tile->row() - 1)
			&& containsTile(tile->column(), tile->row() + 1) ) {
			i.remove();
		}
	}
}

//-----------------------------------------------------------------------------

bool Piece::containsTile(int column, int row)
{
	bool result = false;
	for (int i = 0; i < m_children.count(); ++i) {
		const Tile* tile = m_children.at(i);
		if (tile->column() == column && tile->row() == row) {
			result = true;
			break;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
