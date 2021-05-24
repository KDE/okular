/*
    SPDX-FileCopyrightText: 2020 David Hurka <david.hurka@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CURSORWRAPHELPER_H
#define CURSORWRAPHELPER_H

#include <QPoint>
#include <QPointer>

class QScreen;

/**
 * Wrap the cursor around screen edges.
 *
 * Problem: When setting the cursor position,
 * the actual wrap operation may happen later or not at all.
 * Your application needs to observe the actual wrap operation,
 * and calculate movement offsets based on this operation.
 *
 * This class provides this functionality in a single static function.
 *
 * Example:
 * \code
 * MyWidget::mousePressEvent(QMouseEvent *me)
 * {
 *     CursorWrapHelper::startDrag();
 *     m_lastCursorPos = me->pos();
 * }
 *
 * MyWidget::mouseMoveEvent(QMouseEvent *me)
 * {
 *     cursorMovement = me->pos() - m_lastCursorPos;
 *     cursorMovement -= CursorWrapHelper::wrapCursor(me->pos(), Qt::TopEdge | Qt::BottomEdge);
 *
 *     ...
 *     processMovement(cursorMovement);
 *     ...
 * }
 * \endcode
 */
class CursorWrapHelper
{
public:
    /**
     * Wrap the QCursor around specified screen edges.
     *
     * Wrapping is performed using QCursor::pos().
     * You have to provide a cursor position, because QCursor::pos() is realtime,
     * which means it can not be used to calculate the resulting offset for you.
     * If you implement mousePressEvent() and mouseMoveEvent(),
     * you can simply pass event->pos().
     * @p eventPosition may have a constant offset.
     *
     * @param eventPosition The cursor position you are currently working with.
     * @param edges At which edges to wrap. (E. g. top -> bottom: use Qt::TopEdge)
     * @returns The actual distance the cursor was moved.
     */
    static QPoint wrapCursor(QPoint eventPosition, Qt::Edges edges);

    /**
     * Call this to avoid computing a wrap distance when a drag starts.
     *
     * This should be called every time you get e. g. a mousePressEvent().
     */
    static void startDrag();

protected:
    /** Returns the screen under the cursor */
    static QScreen *getScreen();
    /** Remember screen to speed up screen search */
    static QPointer<QScreen> s_lastScreen;

    /**
     * Actual wrapping of the cursor may happen later.
     * By comparing the magnitude of cursor movements to the last wrap operation,
     * we can catch the moment when wrapping actually happens,
     * and return the wrapping offset at that time.
     *
     * Vertical wrapping and horizontal wrapping may happen with little delay,
     * so they are handled strictly separately.
     */
    static QPoint s_lastCursorPosition;
    static QPoint s_lastWrapOperation;

    /**
     * If the user releases the mouse while it is being wrapped,
     * we don’t want the wrap to be subtracted from the next drag operation.
     * This timestamp allows to check whether the user possibly started a new drag.
     */
    static QPoint s_lastTimeStamp;
};

#endif // CURSORWRAPHELPER_H
