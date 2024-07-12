/*
	SPDX-FileCopyrightText: 2008-2024 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef TETZLE_BOARD_H
#define TETZLE_BOARD_H

class AppearanceDialog;
class EdgeScroller;
class Message;
class Overview;
class Piece;
class Tile;

#include <QHash>
#include <QWidget>
#include <QRandomGenerator>
class QTimeLine;

class Board : public QWidget
{
	Q_OBJECT

public:
	explicit Board(QWidget* parent = nullptr);
	~Board();

	Piece* findCollidingPiece(Piece* piece) const;
	void removePiece(Piece* piece);

	int id() const;
	int margin() const;
	QRect marginRect(const QRect& rect) const;
	int randomInt(int max);
	void setAppearance(const AppearanceDialog& dialog);
	void updateSceneRectangle(Piece* piece);

public Q_SLOTS:
	void newGame(const QString& image, int difficulty);
	void openGame(int id);
	void saveGame() const;
	void retrievePieces();
	void zoomIn();
	void zoomOut();
	void zoomFit();
	void zoom(int level, bool on_cursor);
	void toggleOverview();

Q_SIGNALS:
	void completionChanged(int value);
	void overviewToggled(bool visible);
	void retrievePiecesAvailable(bool available);
	void showMessage(const QString& message, int timeout = 0);
	void clearMessage();
	void zoomChanged(int level, float factor);
	void finished();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	bool event(QEvent* event) override;
#endif
	void paintEvent(QPaintEvent*) override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private Q_SLOTS:
	void edgeScroll(int horizontal, int vertical);

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

	void drawRect(QPainter& painter, const QRect& rect, const QColor& fill, const QColor& border) const;
	void loadImage();
	void updateCursor();
	QPoint mapCursorPosition() const;
	QPoint mapPosition(const QPoint& position) const;
	void updateCompleted();
	void updateSceneRectangle();
	void updateStatusMessage(const QString& message);
	void updateViewport();
	Piece* pieceUnderCursor() const;
	int pieceCount() const;
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

	QPixmap m_pixmap;
	QPixmap m_bevel_pixmap;
	QPixmap m_shadow_pixmap;
	QPixmap m_selected_shadow_pixmap;

	int m_columns;
	int m_rows;
	QList<Piece*> m_pieces;
	QList<Piece*> m_active_pieces;
	QList<Piece*> m_selected_pieces;
	QRect m_scene;
	QRect m_selection;
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

	QRect m_viewport;
	QTransform m_viewport_transform;

	EdgeScroller* m_scroll_left;
	EdgeScroller* m_scroll_right;
	EdgeScroller* m_scroll_up;
	EdgeScroller* m_scroll_down;

	QTimeLine* m_zoom_timer;
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

#endif // TETZLE_BOARD_H
