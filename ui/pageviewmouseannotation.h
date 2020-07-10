/***************************************************************************
 *   Copyright (C) 2017 by Tobias Deiminger <haxtibal@t-online.de>         *
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *   Copyright (C) 2004-2006 by Albert Astals Cid <aacid@kde.org>          *
 *   Copyright (C) 2017    Klarälvdalens Datakonsult AB, a KDAB Group      *
 *                         company, info@kdab.com. Work sponsored by the   *
 *                         LiMux project of the city of Munich             *
 *                                                                         *
 *   With portions of code from kpdf/kpdf_pagewidget.cc by:                *
 *     Copyright (C) 2002 by Wilco Greven <greven@kde.org>                 *
 *     Copyright (C) 2003 by Christophe Devriese                           *
 *                           <Christophe.Devriese@student.kuleuven.ac.be>  *
 *     Copyright (C) 2003 by Laurent Montel <montel@kde.org>               *
 *     Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                *
 *     Copyright (C) 2004 by James Ots <kde@jamesots.com>                  *
 *     Copyright (C) 2011 by Jiri Baum - NICTA <jiri@baum.com.au>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGEVIEWMOUSEANNOTATION_H_
#define _OKULAR_PAGEVIEWMOUSEANNOTATION_H_

#include <QObject>

#include "core/annotations.h"
#include "pageviewutils.h"

class QHelpEvent;
class QPainter;
class QPoint;
class PageView;
class PageViewItem;
class AnnotationDescription;

namespace Okular
{
class Document;
}

/* This class shall help to keep data for one annotation consistent. */
class AnnotationDescription
{
public:
    AnnotationDescription()
        : annotation(nullptr)
        , pageViewItem(nullptr)
        , pageNumber(-1)
    {
    }
    AnnotationDescription(PageViewItem *newPageViewItem, const QPoint eventPos);
    bool isValid() const;
    bool isContainedInPage(const Okular::Document *document, int pageNumber) const;
    void invalidate();
    bool operator==(const AnnotationDescription &rhs) const
    {
        return (annotation == rhs.annotation);
    }
    Okular::Annotation *annotation;
    PageViewItem *pageViewItem;
    int pageNumber;
};

/**
 * @short Handle UI for annotation interactions, like moving, resizing and triggering actions.
 *
 * An object of this class tracks which annotation is currently under the mouse cursor.
 * Some annotation types can be focused in order to move or resize them.
 * State is determined from mouse and keyboard events, which are forwarded from the parent PageView object.
 * Move and resize actions are dispatched to the Document object.
 */
class MouseAnnotation : public QObject
{
    Q_OBJECT

public:
    MouseAnnotation(PageView *parent, Okular::Document *document);
    ~MouseAnnotation() override;

    /* Process a mouse press event. eventPos: Mouse position in content area coordinates. */
    void routeMousePressEvent(PageViewItem *pageViewItem, const QPoint eventPos);

    /* Process a mouse release event. */
    void routeMouseReleaseEvent();

    /* Process a mouse move event. eventPos: Mouse position in content area coordinates. */
    void routeMouseMoveEvent(PageViewItem *pageViewItem, const QPoint eventPos, bool leftButtonPressed);

    /* Process a key event. */
    void routeKeyPressEvent(const QKeyEvent *e);

    /* Process a tooltip event. eventPos: Mouse position in content area coordinates. */
    void routeTooltipEvent(const QHelpEvent *helpEvent);

    /* Process a paint event. */
    void routePaint(QPainter *painter, const QRect paintRect);

    /* Cancel the current selection or action, if any. */
    void cancel();

    /* Reset to initial state. Cancel current action and relinquish references to PageViewItem widgets. */
    void reset();

    Okular::Annotation *annotation() const;

    /* Return true, if MouseAnnotation demands control for a mouse click on the current cursor position. */
    bool isMouseOver() const;

    bool isActive() const;

    bool isFocused() const;

    bool isMoved() const;

    bool isResized() const;

    bool isModified() const;

    Qt::CursorShape cursor() const;

    /* Forward DocumentObserver::notifyPageChanged to this method. */
    void notifyAnnotationChanged(int pageNumber);

    /* Forward DocumentObserver::notifySetup to this method. */
    void updateAnnotationPointers();

    enum MouseAnnotationState { StateInactive, StateFocused, StateMoving, StateResizing };

    enum ResizeHandleFlag {
        RH_None = 0,
        RH_Top = 1,
        RH_Right = 2,
        RH_Bottom = 4,
        RH_Left = 8,
        RH_TopLeft = RH_Top | RH_Left,
        RH_BottomLeft = RH_Bottom | RH_Left,
        RH_TopRight = RH_Top | RH_Right,
        RH_BottomRight = RH_Bottom | RH_Right,
        RH_Content = 16,
        RH_AllHandles = RH_Top | RH_Right | RH_Bottom | RH_Left
    };
    Q_DECLARE_FLAGS(ResizeHandle, ResizeHandleFlag)

private:
    void setState(MouseAnnotationState state, const AnnotationDescription &ad);
    QRect getFullBoundingRect(const AnnotationDescription &ad) const;
    void performCommand(const QPoint newPos);
    void finishCommand();
    void updateViewport(const AnnotationDescription &ad) const;
    ResizeHandle getHandleAt(const QPoint eventPos, const AnnotationDescription &ad) const;
    QRect getHandleRect(ResizeHandle handle, const AnnotationDescription &ad) const;
    static void handleToAdjust(const QPointF dIn, QPointF &dOut1, QPointF &dOut2, MouseAnnotation::ResizeHandle handle, Okular::Rotation rotation);
    static QPointF rotateInRect(const QPointF rotated, Okular::Rotation rotation);
    static ResizeHandle rotateHandle(ResizeHandle handle, Okular::Rotation rotation);
    void processAction(const AnnotationDescription &ad);

    /* We often have to delegate to the document model and our parent widget. */
    Okular::Document *m_document;
    PageView *m_pageView;

    /* Remember which annotation is currently focused/modified. */
    MouseAnnotationState m_state;
    MouseAnnotation::ResizeHandle m_handle;
    AnnotationDescription m_focusedAnnotation;

    /* Mouse tracking, always kept up to date with the latest mouse position and annotation under mouse cursor. */
    AnnotationDescription m_mouseOverAnnotation;
    QPoint m_mousePosition; // in page view item coordinates

    QList<ResizeHandle> m_resizeHandleList;
};

#endif
