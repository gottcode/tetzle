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

#include "piece.h"

#include "board.h"
#include "opengl.h"
#include "tile.h"

#include <QSet>

#include <cmath>

//-----------------------------------------------------------------------------

Piece::Piece(const QPoint& pos, int rotation, const QList<Tile*>& tiles, Board* board)
	: m_board(board),
	m_pos(pos),
	m_tiles(tiles),
	m_shadow(tiles),
	m_rotation(0),
	m_selected(true),
	m_changed(false)
{
	updateTiles();
	updateShadow();

	// Add bevels to tiles
	if (m_tiles.first()->bevel().y() == -1) {
		int count = m_tiles.count();
		for (int i = 0; i < count; ++i) {
			Tile* tile = m_tiles.at(i);
			int sides = 0;
			sides |= containsTile(tile->column() - 1, tile->row());
			sides |= containsTile(tile->column() + 1, tile->row()) << 1;
			sides |= containsTile(tile->column(), tile->row() - 1) << 2;
			sides |= containsTile(tile->column(), tile->row() + 1) << 3;
			static const int bevels[14] = {13, 15, 11, 12, 5, 4, 1, 14, 6, 7, 3, 8, 2, 0};
			tile->setBevel(bevels[sides - 1]);
		}
	}

	// Rotate
	if (rotation) {
		for (int i = 0; i < rotation; ++i) {
			rotate();
		}
	} else {
		updateVerts();
	}

	setSelected(false);
}

//-----------------------------------------------------------------------------

Piece::~Piece()
{
	qDeleteAll(m_tiles);
}

//-----------------------------------------------------------------------------

bool Piece::collidesWith(const Piece* other) const
{
	if (m_board->marginRect(boundingRect()).intersects(other->boundingRect())) {
		return m_collision_region_expanded.intersects(other->m_collision_region);
	} else {
		return false;
	}
}

//-----------------------------------------------------------------------------

void Piece::attachNeighbors()
{
	// Create offset vectors
	int cos_size = 0;
	int sin_size = 0;
	switch (m_rotation) {
	case 0:
		cos_size = Tile::size;
		break;

	case 1:
		sin_size = -Tile::size;
		break;

	case 2:
		cos_size = -Tile::size;
		break;

	case 3:
		sin_size = Tile::size;
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
	foreach (Piece* piece, m_neighbors) {
		if (piece->m_rotation != m_rotation) {
			continue;
		}

		foreach (Tile* child, m_shadow) {
			foreach (Tile* tile, piece->m_shadow) {
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

				if (delta.manhattanLength() <= m_board->margin()) {
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

void Piece::findNeighbors(const QList<Piece*>& pieces)
{
	// Find neighbor tiles
	static QList<QPoint> deltas = QList<QPoint>() << QPoint(-1,0) << QPoint(1,0) << QPoint(0,-1) << QPoint(0,1);
	QList<QPoint> tiles;
	foreach (Tile* tile, m_shadow) {
		QPoint pos(tile->column(), tile->row());
		foreach (const QPoint& delta, deltas) {
			QPoint neighbor = pos + delta;
			if (!containsTile(neighbor.x(), neighbor.y()) && !tiles.contains(neighbor)) {
				tiles.append(neighbor);
			}
		}
	}

	// Find neighbor pieces
	foreach (Piece* piece, pieces) {
		foreach (const QPoint& tile, tiles) {
			if (piece->containsTile(tile.x(), tile.y())) {
				m_neighbors.insert(piece);
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------

void Piece::pushNeighbors(const QPointF& inertia)
{
	while (Piece* neighbor = m_board->findCollidingPiece(this)) {
		// Determine which piece to move
		Piece *source, *target;
		if (m_tiles.count() >= neighbor->m_tiles.count()) {
			source = this;
			target = neighbor;
		} else {
			source = neighbor;
			target = this;
		}
		QRect source_rect = m_board->marginRect(source->boundingRect());

		// Calculate valid movement vector for target; preserve some motion from last move
		QPointF vector = target->boundingRect().center() - source_rect.center() + inertia;
		while (vector.manhattanLength() < 1.0) {
			vector.setX( (rand() % Tile::size) - (Tile::size / 2) );
			vector.setY( (rand() % Tile::size) - (Tile::size / 2) );
		}

		// Scale movement vector so that the largest dimension is 1
		QPointF direction = vector / qMax(fabs(vector.x()), fabs(vector.y()));

		// Push target until it is clear from current source
		// We use a binary-search, pushing away if collision, retracting otherwise
		QPoint orig = target->m_pos;
		QRect united = source_rect.united(target->boundingRect());
		float min = 0.0f;
		float max = united.width() * united.height();
		while (true) {
			float test = (min + max) / 2.0f;
			target->m_pos = orig + (test * direction).toPoint();
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
		target->updateVerts();
		Q_ASSERT(min < max);
		Q_ASSERT(!source->collidesWith(target));

		// Recurse, and keep inertia for stability.
		target->pushNeighbors(vector);
	}
}

//-----------------------------------------------------------------------------

void Piece::rotate(const QPoint& origin)
{
	// Rotate 90 degrees counter-clockwise around origin
	if (!origin.isNull()) {
		QPoint pos = m_pos;
		pos.ry() += m_rect.height();
		m_pos.setX( origin.y() + origin.x() - pos.y() );
		m_pos.setY( origin.y() - origin.x() + pos.x() );
	}
	m_rect.setRect(0, 0, m_rect.height(), m_rect.width());

	// Rotate tiles 90 degrees counter-clockwise
	int count = m_tiles.count();
	for (int i = 0; i < count; ++i) {
		m_tiles.at(i)->rotate();
	}

	// Track how many rotations have occured
	m_rotation += 1;
	if (m_rotation > 3) {
		m_rotation = 0;
	}

	updateVerts();
}

//-----------------------------------------------------------------------------

void Piece::setSelected(bool selected)
{
	m_selected = selected;
	if (!m_selected && m_changed) {
		updateCollisionRegions();
	}
}

//-----------------------------------------------------------------------------

void Piece::drawTiles() const
{
	glVertexPointer(2, GL_INT, sizeof(TileVertex), &m_verts.first().x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(TileVertex), &m_verts.first().s1);
	GL::clientActiveTexture(GL_TEXTURE1);
	glTexCoordPointer(2, GL_FLOAT, sizeof(TileVertex), &m_verts.first().s2);
	GL::clientActiveTexture(GL_TEXTURE0);
	glDrawArrays(GL_QUADS, 0, m_verts.count());
}

//-----------------------------------------------------------------------------

void Piece::drawShadow() const
{
	glVertexPointer(2, GL_INT, sizeof(ShadowVertex), &m_shadow_verts.first().x);
	glTexCoordPointer(2, GL_INT, sizeof(ShadowVertex), &m_shadow_verts.first().s);
	glDrawArrays(GL_QUADS, 0, m_shadow_verts.count());
}

//-----------------------------------------------------------------------------

void Piece::save(QXmlStreamWriter& xml) const
{
	xml.writeStartElement("piece");
	xml.writeAttribute("x", QString::number(m_pos.x()));
	xml.writeAttribute("y", QString::number(m_pos.y()));
	xml.writeAttribute("rotation", QString::number(m_rotation));

	for (int i = 0; i < m_tiles.count(); ++i) {
		m_tiles.at(i)->save(xml);
	}

	xml.writeEndElement();
}

//-----------------------------------------------------------------------------

void Piece::attach(Piece* piece)
{
	Q_ASSERT(piece != this);

	// Update position
	QPoint delta = piece->m_pos - m_pos;
	if (delta.x() < 0) {
		m_pos.rx() += delta.x();
	}
	if (delta.y() < 0) {
		m_pos.ry() += delta.y();
	}

	// Update position of attached tiles
	QList<Tile*> tiles = piece->m_tiles;
	int count = tiles.count();
	for (int i = 0; i < count; ++i) {
		tiles.at(i)->setPos(tiles.at(i)->pos() + delta);
	}
	m_tiles += tiles;
	piece->m_tiles.clear();
	updateTiles();

	// Update shadow
	m_shadow += piece->m_shadow;
	updateShadow();

	// Update neighbors
	m_neighbors += piece->m_neighbors;
	m_neighbors.remove(piece);
	m_neighbors.remove(this);
	foreach (Piece* neighbor, m_neighbors) {
		neighbor->m_neighbors.remove(piece);
		neighbor->m_neighbors.insert(this);
	}

	// Remove attached piece
	m_board->removePiece(piece);

	updateVerts();
}

//-----------------------------------------------------------------------------

bool Piece::containsTile(int column, int row)
{
	bool result = false;
	for (int i = 0; i < m_tiles.count(); ++i) {
		const Tile* tile = m_tiles.at(i);
		if (tile->column() == column && tile->row() == row) {
			result = true;
			break;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------

void Piece::updateCollisionRegions()
{
	m_changed = false;
	m_collision_region = QRegion();
	m_collision_region_expanded = QRegion();
	QRect rect(0,0, Tile::size, Tile::size);
	for (int i = 0; i < m_tiles.count(); ++i) {
		rect.moveTo(m_tiles.at(i)->scenePos());
		m_collision_region += rect;
		m_collision_region_expanded += m_board->marginRect(rect);
	}
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

void Piece::updateTiles()
{
	int count = m_tiles.count();

	// Find bounding rectangle
	QPoint top_left = m_tiles.first()->pos();
	QPoint bottom_right = m_tiles.first()->pos() + QPoint(Tile::size, Tile::size);
	QPoint pos;
	for (int i = 0; i < count; ++i) {
		pos = m_tiles.at(i)->pos();
		top_left.setX( qMin(pos.x(), top_left.x()) );
		top_left.setY( qMin(pos.y(), top_left.y()) );
		bottom_right.setX( qMax(pos.x() + Tile::size, bottom_right.x()) );
		bottom_right.setY( qMax(pos.y() + Tile::size, bottom_right.y()) );
	}
	m_rect.setRect(0, 0, bottom_right.x() - top_left.x(), bottom_right.y() - top_left.y());

	// Shift tiles to be inside rectangle
	Tile* tile;
	for (int i = 0; i < count; ++i) {
		tile = m_tiles.at(i);
		tile->setParent(this);
		tile->setPos(tile->pos() - top_left);
	}
}

//-----------------------------------------------------------------------------

void Piece::updateVerts()
{
	m_changed = true;
	if (!m_selected) {
		updateCollisionRegions();
	}

	// Update tile verts
	m_verts.clear();
	m_verts.reserve(m_tiles.count() * 4);
	for (int i = 0; i < m_tiles.count(); ++i) {
		Tile* tile = m_tiles.at(i);

		QPoint pos = tile->scenePos();
		int x1 = pos.x();
		int y1 = pos.y();
		int x2 = x1 + Tile::size;
		int y2 = y1 + Tile::size;

		const QPointF* corners = m_board->corners(rotation());
		float tx = tile->column() * m_board->tileTextureSize();
		float ty = tile->row() * m_board->tileTextureSize();

		float bx1 = tile->bevel().x();
		float by1 = tile->bevel().y();
		float bx2 = bx1 + 0.125;
		float by2 = by1 + 0.125;

		m_verts.append( TileVertex(x1,y1, tx + corners[0].x(),ty + corners[0].y(), bx1,by1) );
		m_verts.append( TileVertex(x1,y2, tx + corners[1].x(),ty + corners[1].y(), bx1,by2) );
		m_verts.append( TileVertex(x2,y2, tx + corners[2].x(),ty + corners[2].y(), bx2,by2) );
		m_verts.append( TileVertex(x2,y1, tx + corners[3].x(),ty + corners[3].y(), bx2,by1) );
	}

	// Update shadow verts
	static const int offset = Tile::size / 2;
	static const int size = Tile::size * 2;
	m_shadow_verts.clear();
	m_shadow_verts.reserve(m_shadow.count() * 4);
	for (int i = 0; i < m_shadow.count(); ++i) {
		QPoint pos = m_shadow.at(i)->scenePos();
		int x1 = pos.x() - offset;
		int y1 = pos.y() - offset;
		int x2 = x1 + size;
		int y2 = y1 + size;

		m_shadow_verts.append( ShadowVertex(x1,y1, 0,0) );
		m_shadow_verts.append( ShadowVertex(x1,y2, 0,1) );
		m_shadow_verts.append( ShadowVertex(x2,y2, 1,1) );
		m_shadow_verts.append( ShadowVertex(x2,y1, 1,0) );
	}

	// Update scene rectangle
	m_board->updateSceneRectangle(this);
}

//-----------------------------------------------------------------------------
