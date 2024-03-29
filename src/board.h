/*
	SPDX-FileCopyrightText: 2008-2021 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_BOARD_H
#define TETZLE_BOARD_H

#include "graphics_layer.h"
class AppearanceDialog;
class Message;
class Overview;
class Piece;
class Tile;

#include <QHash>
#include <QOpenGLWidget>
#include <QRandomGenerator>
class QOpenGLTexture;

class Board : public QOpenGLWidget
{
	Q_OBJECT

	struct Region
	{
		VertexArray fill;
		VertexArray border;
	};

public:
	explicit Board(QWidget* parent = nullptr);
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

public Q_SLOTS:
	void newGame(const QString& image, int difficulty);
	void openGame(int id);
	void saveGame();
	void retrievePieces();
	void zoomIn();
	void zoomOut();
	void zoomFit();
	void zoom(int level);
	void toggleOverview();

Q_SIGNALS:
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
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

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

	QRandomGenerator m_random;
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
	return m_random.bounded(max);
}

inline float Board::tileTextureSize() const
{
	return m_image_ts;
}

inline const QPointF* Board::corners(int rotation) const
{
	return m_corners[rotation];
}

#endif // TETZLE_BOARD_H
