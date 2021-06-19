/*
    SPDX-FileCopyrightText: 2020 David Hurka <david.hurka@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cursorwraphelper.h"

#include <QCursor>
#include <QGuiApplication>
#include <QRect>
#include <QScreen>

QPointer<QScreen> CursorWrapHelper::s_lastScreen;
QPoint CursorWrapHelper::s_lastCursorPosition;
QPoint CursorWrapHelper::s_lastWrapOperation;

QPoint CursorWrapHelper::wrapCursor(QPoint eventPosition, Qt::Edges edges)
{
    QScreen *screen = getScreen();
    if (!screen) {
        return QPoint();
    }

    // Step 1: Generate wrap operations.
    // Assuming screen->geometry() is larger than 10x10.
    const QRect screenGeometry = screen->geometry();
    const QPoint screenCursorPos = QCursor::pos(screen);

    if (edges & Qt::LeftEdge && screenCursorPos.x() < screenGeometry.left() + 4) {
        QCursor::setPos(screen, screenCursorPos + QPoint(screenGeometry.width() - 10, 0));
        s_lastWrapOperation.setX(screenGeometry.width() - 10);
    } else if (edges & Qt::RightEdge && screenCursorPos.x() > screenGeometry.right() - 4) {
        QCursor::setPos(screen, screenCursorPos + QPoint(-screenGeometry.width() + 10, 0));
        s_lastWrapOperation.setX(-screenGeometry.width() + 10);
    }

    if (edges & Qt::TopEdge && screenCursorPos.y() < screenGeometry.top() + 4) {
        QCursor::setPos(screen, screenCursorPos + QPoint(0, screenGeometry.height() - 10));
        s_lastWrapOperation.setY(screenGeometry.height() - 10);
    } else if (edges & Qt::BottomEdge && screenCursorPos.y() > screenGeometry.bottom() - 4) {
        QCursor::setPos(screen, screenCursorPos + QPoint(0, -screenGeometry.height() + 10));
        s_lastWrapOperation.setY(-screenGeometry.height() + 10);
    }

    // Step 2: Catch wrap movements.
    // We observe the cursor movement since the last call of wrapCursor().
    // If the cursor moves in the same magnitude as the last wrap operation,
    // we return the value of this wrap operation with appropriate sign.
    const QPoint cursorMovement = eventPosition - s_lastCursorPosition;
    s_lastCursorPosition = eventPosition;

    QPoint ret_wrapDistance;

    qreal horizontalMagnitude = qAbs(qreal(s_lastWrapOperation.x()) / qreal(cursorMovement.x()));
    int horizontalSign = cursorMovement.x() > 0 ? 1 : -1;
    if (0.5 < horizontalMagnitude && horizontalMagnitude < 2.0) {
        ret_wrapDistance.setX(qAbs(s_lastWrapOperation.x()) * horizontalSign);
    }

    qreal verticalMagnitude = qAbs(qreal(s_lastWrapOperation.y()) / qreal(cursorMovement.y()));
    int verticalSign = cursorMovement.y() > 0 ? 1 : -1;
    if (0.5 < verticalMagnitude && verticalMagnitude < 2.0) {
        ret_wrapDistance.setY(qAbs(s_lastWrapOperation.y()) * verticalSign);
    }

    return ret_wrapDistance;
}

void CursorWrapHelper::startDrag()
{
    s_lastWrapOperation.setX(0);
    s_lastWrapOperation.setY(0);
}

QScreen *CursorWrapHelper::getScreen()
{
    const QPoint cursorPos = QCursor::pos();

    if (s_lastScreen && s_lastScreen->geometry().contains(cursorPos)) {
        return s_lastScreen;
    }

    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (screen->geometry().contains(cursorPos)) {
            s_lastScreen = screen;
            return screen;
        }
    }
    // Corner case: cursor already pushed against an edge.
    for (QScreen *screen : screens) {
        if (screen->geometry().adjusted(-5, -5, 5, 5).contains(cursorPos)) {
            s_lastScreen = screen;
            return screen;
        }
    }

    return nullptr;
}
