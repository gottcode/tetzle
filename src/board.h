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
#include <QRandomGenerator>
#include <QWidget>
class QTimeLine;

/**
 * Main class for a game. It manages the pieces, rendering, and player interaction.
 */
class Board : public QWidget
{
	Q_OBJECT

public:
	/**
	 * Construct a board widget.
	 *
	 * @param parent the parent widget of the board
	 */
	explicit Board(QWidget* parent = nullptr);

	/**
	 * Clean up board.
	 */
	~Board();

	/**
	 * Find a colliding piece.
	 *
	 * @param piece the piece to check for collisions
	 *
	 * @return the first piece to collide, or @c nullptr if there are none
	 */
	Piece* findCollidingPiece(const Piece* piece) const;

	/**
	 * Remove a piece from the board.
	 *
	 * @param piece the piece to remove
	 */
	void removePiece(Piece* piece);

	/**
	 * Fetch current game identifier.
	 *
	 * @return the current game identifier
	 */
	int id() const
	{
		return m_id;
	}

	/**
	 * Fetch the maximum distance between pieces before they attach.
	 *
	 * @return the maximum distance between pieces before they attach
	 */
	int margin() const
	{
		return 16;
	}

	/**
	 * Expand a rectangle by the margin for attaching.
	 *
	 * @param rect the rectangle to expand
	 *
	 * @return rectangle expanded by attach margin
	 */
	QRect marginRect(const QRect& rect) const
	{
		return rect.adjusted(-margin(), -margin(), margin(), margin());
	}

	/**
	 * Fetch a random integer.
	 *
	 * @param max the first integer outside the range of possible results
	 *
	 * @return random integer
	 */
	int randomInt(int max)
	{
		return m_random.bounded(max);
	}

	/**
	 * Set the appearance of the game board.
	 *
	 * @param dialog the dialog containing the new appearance settings
	 */
	void setAppearance(const AppearanceDialog& dialog);

	/**
	 * Expand the scene rectangle to encompass piece.
	 *
	 * @param piece the piece to include in scene rectangle
	 */
	void updateSceneRectangle(const Piece* piece);

public Q_SLOTS:
	/**
	 * Start a new game. This will reset the board and generate a new puzzle.
	 *
	 * @param image the image to use on the pieces
	 * @param difficulty how many pieces to generate
	 */
	void newGame(const QString& image, int difficulty);

	/**
	 * Start a previous game. This will reset the board and load the
	 * contents of the saved game.
	 *
	 * @param id the game to start
	 */
	void openGame(int id);

	/**
	 * Save the current game, if started and not already finished.
	 */
	void saveGame() const;

	/**
	 * Grab all pieces and drop them on top of each other,
	 * then push them apart to prevent overlapping pieces.
	 */
	void retrievePieces();

	/**
	 * Increase the visible size of the pieces.
	 */
	void zoomIn();

	/**
	 * Decrease the visible size of the pieces.
	 */
	void zoomOut();

	/**
	 * Find the zoom level which will show all pieces on the board,
	 * then animate to that zoom level.
	 */
	void zoomFit();

	/**
	 * Set the visible size of the pieces.
	 *
	 * @param level the zoom level to use
	 * @param on_cursor @c true if it should center on the mouse cursor
	 */
	void zoom(int level, bool on_cursor);

	/**
	 * Toggle if the overview is visible.
	 */
	void toggleOverview();

Q_SIGNALS:
	/**
	 * Signal that the player has completed more of the puzzle.
	 * It is represented as an integer in the range 0-100.
	 *
	 * @param value how far the player is through the puzzle
	 */
	void completionChanged(int value);

	/**
	 * Signal that the overview has been toggled.
	 *
	 * @param visible @c true if the overview is visible
	 */
	void overviewToggled(bool visible);

	/**
	 * Signal that the player is able to retrieve all pieces.
	 *
	 * @param available @c true if the player can retrieve all pieces
	 */
	void retrievePiecesAvailable(bool available);

	/**
	 * Signal to show a statusbar message. If @a timeout is @c 0 it does not hide.
	 *
	 * @param message the text to show the player
	 * @param timeout how long in milliseconds before the text hides
	 */
	void showMessage(const QString& message, int timeout = 0);

	/**
	 * Signal to clear the statusbar message.
	 */
	void clearMessage();

	/**
	 * Signal that the zoom level has changed.
	 *
	 * @param level the level of zoom
	 */
	void zoomChanged(int level);

	/**
	 * Signal that the player has completed the puzzle.
	 */
	void finished();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
	/**
	 * Handle device pixel change. Reloads the image and shadows,
	 * and reloads the piece fragment lists for drawing at a new resolution.
	 */
	bool event(QEvent* event) override;
#endif

	/**
	 * Draw the game board.
	 */
	void paintEvent(QPaintEvent*) override;

	/**
	 * Handle player input. Allows players to move game board,
	 * as well as select, pick up, drop, or rotate pieces.
	 */
	void keyPressEvent(QKeyEvent* event) override;

	/**
	 * Handle player input. This stops resizing the selection rectangle,
	 * now that the player has released a key.
	 */
	void keyReleaseEvent(QKeyEvent* event) override;

	/**
	 * Handle player input. Allows players to move game board,
	 * as well as select, pick up, drop, or rotate pieces.
	 */
	void mousePressEvent(QMouseEvent* event) override;

	/**
	 * Handle player input. Allows players to move game board,
	 * as well as select, pick up, drop, or rotate pieces.
	 */
	void mouseReleaseEvent(QMouseEvent* event) override;

	/**
	 * Handle player input. Allows players to move game board,
	 * move held pieces, or to resize a selection rectangle.
	 */
	void mouseMoveEvent(QMouseEvent* event) override;

	/**
	 * Update the viewport for the new window size.
	 */
	void resizeEvent(QResizeEvent* event) override;

	/**
	 * Handle player input. Allows players to zoom board by scrolling.
	 */
	void wheelEvent(QWheelEvent* event) override;

private Q_SLOTS:
	/**
	 * Handle player scrolling board by its edges.
	 *
	 * @param horizontal negative scrolls left, positive scrolls right
	 * @param vertical negative scrolls up, positive scrolls down
	 */
	void edgeScroll(int horizontal, int vertical);

private:
	/**
	 * Start scrolling the play area.
	 */
	void startScrolling();

	/**
	 * Stop scrolling the play area.
	 */
	void stopScrolling();

	/**
	 * Scroll the play area.
	 *
	 * @param delta how far to scroll
	 */
	void scroll(const QPoint& delta);

	/**
	 * Handle player interacting with pieces. It grabs, releases, or rotates the pieces.
	 */
	void togglePiecesUnderCursor();

	/**
	 * Move the mouse cursor.
	 *
	 * @param delta how far to move the mouse cursor
	 *
	 * @note Does not work in Wayland.
	 */
	void moveCursor(const QPoint& delta);

	/**
	 * Pick up the piece under the mouse cursor.
	 */
	void grabPiece();

	/**
	 * Put down the held pieces.
	 */
	void releasePieces();

	/**
	 * Rotate the held pieces.
	 */
	void rotatePiece();

	/**
	 * Select the pieces under the mouse cursor or in the selection rectangle.
	 */
	void selectPieces();

	/**
	 * Draw a rectangle.
	 *
	 * @param painter the painter to use for drawing
	 * @param rect the rectangle to draw
	 * @param fill the fill of the rectangle
	 * @param border the border of the rectangle
	 */
	void drawRect(QPainter& painter, const QRect& rect, const QColor& fill, const QColor& border) const;

	/**
	 * Load the image for the puzzle, as well as the bevel and shadows.
	 */
	void loadImage();

	/**
	 * Change the mouse cursor depending on if it is over a piece.
	 * If the player is already holding pieces, it uses a different
	 * cursor to show that they can pick up another piece.
	 */
	void updateCursor();

	/**
	 * Map the cursor position to scene coordinates.
	 *
	 * @return QPoint of mapped cursor position
	 */
	QPoint mapCursorPosition() const;

	/**
	 * Map a position to scene coordinates.
	 *
	 * @param position the position to map
	 *
	 * @return QPoint of mapped position
	 */
	QPoint mapPosition(const QPoint& position) const;

	/**
	 * Determine how far the player is through the puzzle.
	 */
	void updateCompleted();

	/**
	 * Find the smallest scene rectangle which encompasses all pieces.
	 */
	void updateSceneRectangle();

	/**
	 * Change the statusbar message.
	 *
	 * @param message the text to show the player
	 */
	void updateStatusMessage(const QString& message);

	/**
	 * Calculate the transformation of the play area by the current
	 * zoom scale factor. Also handles enabling or disabling edge
	 * scrolling based on the scene rectangle.
	 */
	void updateViewport();

	/**
	 * Return piece under mouse cursor.
	 *
	 * @return piece under mouse cursor, or @c nullptr if there are none
	 */
	Piece* pieceUnderCursor() const;

	/**
	 * Fetch how many pieces there are total (decreases when pieces are attached).
	 *
	 * @return how many pieces there are total
	 */
	int pieceCount() const;

	/**
	 * Process finishing a game. Drops the currently held piece,
	 * rotates it to be upright, and then zooms in on it.
	 */
	void finishGame();

	/**
	 * Reset the board to an empty state. Deletes all pieces.
	 */
	void cleanup();

private:
	int m_id; ///< the current save game identifier
	bool m_load_bevels; ///< does the current game support bevels
	QString m_image_path; ///< the location of the image for the current puzzle
	Overview* m_overview; ///< the overview widget
	Message* m_message; ///< the message overlay widget
	bool m_has_bevels; ///< are bevels shown
	bool m_has_shadows; ///< are shadows shown

	QPixmap m_pixmap; ///< the current puzzle image
	QPixmap m_bevel_pixmap; ///< the bevel image
	QPixmap m_shadow_pixmap; ///< the shadow image
	QPixmap m_selected_shadow_pixmap; ///< the shadow image for selected pieces

	int m_columns; ///< how many columns there are
	int m_rows; ///< how many rows there are
	QList<Piece*> m_pieces; ///< the pieces that make up the puzzle
	QList<Piece*> m_active_pieces; ///< the pieces held by the player
	QList<Piece*> m_selected_pieces; ///< the pieces in the selection rectangle
	QRect m_scene; ///< the scene rectangle
	QRect m_selection; ///< the selection rectangle
	int m_total_pieces; ///< the total pieces (does not decrease when pieces are attached)
	int m_completed; ///< how much the player has completed

	QPoint m_pos; ///< the center of the view
	QPoint m_cursor_pos; ///< the mouse cursor position
	QPoint m_select_pos; ///< the start of the selection rectangle as it is being made
	int m_scale_level; ///< the zoom level
	float m_scale; ///< the scaling factor
	bool m_scrolling; ///< is the player scrolling the play area
	bool m_selecting; ///< is the player selecting pieces
	bool m_finished; ///< is the game over

	int m_action_key; ///< the currently pressed key
	Qt::MouseButton m_action_button; ///< the currently pressed mouse button

	QRandomGenerator m_random; ///< random number generator

	QRect m_viewport; ///< the size of the view
	QTransform m_viewport_transform; ///< the transformation matrix to view coordinates

	EdgeScroller* m_scroll_left; ///< the left edge scroller
	EdgeScroller* m_scroll_right; ///< the right edge scroller
	EdgeScroller* m_scroll_up; ///< the top edge scroller
	EdgeScroller* m_scroll_down; ///< the bottom edge scroller

	QTimeLine* m_zoom_timer; ///< timeline to animate zooming to fit
};

#endif // TETZLE_BOARD_H
