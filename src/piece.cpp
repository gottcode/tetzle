/*
	SPDX-FileCopyrightText: 2008-2016 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "piece.h"

#include "board.h"
#include "tile.h"

#include <algorithm>
#include <cmath>

//-----------------------------------------------------------------------------

Piece::Piece(const QPoint& pos, int rotation, const QList<Tile*>& tiles, Board* board)
	: m_board(board)
	, m_pos(pos)
	, m_tiles(tiles)
	, m_shadow(tiles)
	, m_rotation(0)
	, m_depth(2)
	, m_selected(true)
	, m_changed(false)
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
	rotate(rotation);

	setSelected(false);
}

//-----------------------------------------------------------------------------

Piece::~Piece()
{
	graphics_layer->removeArray(m_tile_array);
	graphics_layer->removeArray(m_shadow_array);

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

QPoint Piece::randomPoint() const
{
	Tile* tile = m_tiles.at(m_board->randomInt(m_tiles.count()));
	return tile->scenePos() + QPoint(m_board->randomInt(Tile::size), m_board->randomInt(Tile::size));
}

//-----------------------------------------------------------------------------

void Piece::attachNeighbors()
{
	const auto neighbors = m_neighbors;
	for (Piece* piece : neighbors) {
		if (piece->m_rotation != m_rotation) {
			continue;
		}

		Tile* tile = m_tiles.first();
		Tile* piece_tile = piece->m_tiles.first();
		QPoint grid_delta = piece_tile->gridPos() - tile->gridPos();
		for (int i = 0; i < m_rotation; ++i) {
			QPoint pos = grid_delta;
			grid_delta.setX( -pos.y() );
			grid_delta.setY( pos.x() );
		}
		QPoint top_left = tile->scenePos() + grid_delta;
		QPoint delta = top_left - piece_tile->scenePos();

		if (delta.manhattanLength() <= m_board->margin()) {
			piece->moveBy(delta);
			attach(piece);
		}
	}
}

//-----------------------------------------------------------------------------

void Piece::findNeighbors(const QList<Piece*>& pieces)
{
	// Find neighbor tiles
	static const QList<QPoint> deltas = QList<QPoint>() << QPoint(-1,0) << QPoint(1,0) << QPoint(0,-1) << QPoint(0,1);
	QList<QPoint> tiles;
	for (Tile* tile : qAsConst(m_shadow)) {
		QPoint pos(tile->column(), tile->row());
		for (const QPoint& delta : deltas) {
			QPoint neighbor = pos + delta;
			if (!containsTile(neighbor.x(), neighbor.y()) && !tiles.contains(neighbor)) {
				tiles.append(neighbor);
			}
		}
	}

	// Find neighbor pieces
	for (Piece* piece : pieces) {
		for (const QPoint& tile : qAsConst(tiles)) {
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
			vector.setX( (m_board->randomInt(Tile::size)) - (Tile::size / 2) );
			vector.setY( (m_board->randomInt(Tile::size)) - (Tile::size / 2) );
		}

		// Scale movement vector so that the largest dimension is 1
		QPointF direction = vector / std::max(fabs(vector.x()), fabs(vector.y()));

		// Push target until it is clear from current source
		// We use a binary-search, pushing away if collision, retracting otherwise
		QPoint orig = target->m_pos;
		QRect united = source_rect.united(target->boundingRect());
		float min = 0.0f;
		float max = united.width() * united.height();
		Q_FOREVER {
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

void Piece::rotate(int rotations)
{
	if (rotations) {
		for (int i = 0; i < rotations; ++i) {
			rotate();
		}
	} else {
		updateVerts();
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

void Piece::setDepth(int depth)
{
	m_depth = (depth + 1) * 2;
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
	m_pos.setX(std::min(m_pos.x(), piece->m_pos.x()));
	m_pos.setY(std::min(m_pos.y(), piece->m_pos.y()));

	// Update shadow
	m_shadow += piece->m_shadow;
	updateShadow();

	// Update position of attached tiles
	int rotation = m_rotation;
	for (int i = rotation; i < 4; ++i) {
		rotate();
		piece->rotate();
	}
	m_tiles += piece->m_tiles;
	piece->m_tiles.clear();
	updateTiles();
	rotate(rotation);

	// Update neighbors
	m_neighbors += piece->m_neighbors;
	m_neighbors.remove(piece);
	m_neighbors.remove(this);
	for (Piece* neighbor : qAsConst(m_neighbors)) {
		neighbor->m_neighbors.remove(piece);
		neighbor->m_neighbors.insert(this);
	}

	// Remove attached piece
	m_board->removePiece(piece);
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
	QPoint top_left = m_tiles.first()->gridPos();
	QPoint bottom_right = top_left + QPoint(Tile::size, Tile::size);
	QPoint pos;
	for (int i = 0; i < count; ++i) {
		pos = m_tiles.at(i)->gridPos();
		top_left.setX( std::min(pos.x(), top_left.x()) );
		top_left.setY( std::min(pos.y(), top_left.y()) );
		bottom_right.setX( std::max(pos.x() + Tile::size, bottom_right.x()) );
		bottom_right.setY( std::max(pos.y() + Tile::size, bottom_right.y()) );
	}
	m_rect.setRect(0, 0, bottom_right.x() - top_left.x(), bottom_right.y() - top_left.y());

	// Shift tiles to be inside rectangle
	Tile* tile;
	for (int i = 0; i < count; ++i) {
		tile = m_tiles.at(i);
		tile->setParent(this);
		tile->setPos(tile->gridPos() - top_left);
	}
}

//-----------------------------------------------------------------------------

void Piece::updateVerts()
{
	m_changed = true;
	if (!m_selected) {
		updateCollisionRegions();
	}

	QList<Vertex> verts;
	int z = m_depth;

	// Update tile verts
	verts.reserve(m_tiles.count() * 6);
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

		verts.append( Vertex::init(x1,y1,z, tx + corners[0].x(),ty + corners[0].y(), bx1,by1) );
		verts.append( Vertex::init(x1,y2,z, tx + corners[1].x(),ty + corners[1].y(), bx1,by2) );
		verts.append( Vertex::init(x2,y1,z, tx + corners[3].x(),ty + corners[3].y(), bx2,by1) );
		verts.append( Vertex::init(x2,y1,z, tx + corners[3].x(),ty + corners[3].y(), bx2,by1) );
		verts.append( Vertex::init(x1,y2,z, tx + corners[1].x(),ty + corners[1].y(), bx1,by2) );
		verts.append( Vertex::init(x2,y2,z, tx + corners[2].x(),ty + corners[2].y(), bx2,by2) );
	}
	graphics_layer->updateArray(m_tile_array, verts);

	// Update shadow verts
	z--;
	static const int offset = Tile::size / 2;
	static const int size = Tile::size * 2;
	verts.clear();
	verts.reserve(m_shadow.count() * 6);
	for (int i = 0; i < m_shadow.count(); ++i) {
		QPoint pos = m_shadow.at(i)->scenePos();
		int x1 = pos.x() - offset;
		int y1 = pos.y() - offset;
		int x2 = x1 + size;
		int y2 = y1 + size;

		verts.append( Vertex::init(x1,y1,z, 0,0) );
		verts.append( Vertex::init(x1,y2,z, 0,1) );
		verts.append( Vertex::init(x2,y1,z, 1,0) );
		verts.append( Vertex::init(x2,y1,z, 1,0) );
		verts.append( Vertex::init(x1,y2,z, 0,1) );
		verts.append( Vertex::init(x2,y2,z, 1,1) );
	}
	graphics_layer->updateArray(m_shadow_array, verts);

	// Update scene rectangle
	m_board->updateSceneRectangle(this);
}

//-----------------------------------------------------------------------------
