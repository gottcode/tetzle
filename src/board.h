/***********************************************************************
 *
 * Copyright (C) 2008, 2010, 2011, 2014, 2016 Graeme Gott <graeme@gottcode.org>
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

#ifndef BOARD_H
#define BOARD_H

#include "graphics_layer.h"
class AppearanceDialog;
class Message;
class Overview;
class Piece;
class Tile;

#include <QHash>
#if (QT_VERSION >= QT_VERSION_CHECK(5,4,0))
#include <QOpenGLWidget>
typedef QOpenGLWidget GLWidget;
#else
#include <QGLWidget>
typedef QGLWidget GLWidget;
#endif
class QOpenGLTexture;

#include <random>

class Board : public GLWidget
{
	Q_OBJECT

	struct Region
	{
		VertexArray fill;
		VertexArray border;
	};

public:
	Board(QWidget* parent = 0);
	~Board();

	Piece* findCollidingPiece(Piece* piece) const;
	void removePiece(Piece* piece);

	int id() const;
	int margin() const;
	QRect marginRect(const QRect& rect) const;
	int randomInt(int max);
	float tileTextureSize() const;
	const QPointF* corners(int rotation) const;
	void setAppearance(const AppearanceDialog& dialog);
	void updateSceneRectangle(Piece* piece);

public slots:
	void newGame(const QString& image, int difficulty);
	void openGame(int id);
	void saveGame();
	void retrievePieces();
	void zoomIn();
	void zoomOut();
	void zoomFit();
	void zoom(int level);
	void toggleOverview();

signals:
	void completionChanged(int value);
	void overviewToggled(bool visible);
	void retrievePiecesAvailable(bool available);
	void showMessage(const QString& message, int timeout = 0);
	void clearMessage();
	void zoomInAvailable(bool available);
	void zoomOutAvailable(bool available);
	void zoomChanged(int level, float factor);
	void finished();

protected:
	virtual void initializeGL();
	virtual void resizeGL(int w, int h);
	virtual void paintGL();
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

private:
	void startScrolling();
	void stopScrolling();
	void scroll(const QPoint& delta);
	void togglePiecesUnderCursor();
	void moveCursor(const QPoint& delta);
	void grabPiece();
	void releasePieces();
	void rotatePiece();
	void selectPieces();

	void drawArray(const Region& region, const QColor& fill, const QColor& border);
	void loadImage();
	void updateCursor();
	QPoint mapCursorPosition() const;
	QPoint mapPosition(const QPoint& position) const;
	void updateCompleted();
	void updateArray(Region& region, const QRect& rect, int z);
	void updateSceneRectangle();
	void updateStatusMessage(const QString& message);
	Piece* pieceUnderCursor();
	int pieceCount();
	void finishGame();
	void cleanup();

private:
	int m_id;
	bool m_load_bevels;
	QString m_image_path;
	Overview* m_overview;
	Message* m_message;
	bool m_has_bevels;
	bool m_has_shadows;

	QOpenGLTexture* m_image;
	QOpenGLTexture* m_bumpmap_image;
	QOpenGLTexture* m_shadow_image;
	float m_image_ts;
	QPointF m_corners[4][4];
	Region m_scene_array;
	Region m_selection_array;

	int m_columns;
	int m_rows;
	QList<Piece*> m_pieces;
	QList<Piece*> m_active_pieces;
	QList<Piece*> m_selected_pieces;
	QRect m_scene;
	int m_total_pieces;
	int m_completed;

	QPoint m_pos;
	QPoint m_cursor_pos;
	QPoint m_select_pos;
	int m_scale_level;
	float m_scale;
	bool m_scrolling;
	bool m_selecting;
	bool m_finished;

	int m_action_key;
	Qt::MouseButton m_action_button;

	std::mt19937 m_random;
};


inline int Board::id() const
{
	return m_id;
}

inline int Board::margin() const
{
	return 16;
}

inline QRect Board::marginRect(const QRect& rect) const
{
	return rect.adjusted(-margin(), -margin(), margin(), margin());
}

inline int Board::randomInt(int max)
{
	std::uniform_int_distribution<int> dis(0, max - 1);
	return dis(m_random);
}

inline float Board::tileTextureSize() const
{
	return m_image_ts;
}

inline const QPointF* Board::corners(int rotation) const
{
	return m_corners[rotation];
}

#endif
