/*
    SPDX-FileCopyrightText: 2017 Tobias Deiminger <haxtibal@t-online.de>
    SPDX-FileCopyrightText: 2004-2005 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2004-2006 Albert Astals Cid <aacid@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    With portions of code from kpdf/kpdf_pagewidget.cc by:
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2003 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2003 Dirk Mueller <mueller@kde.org>
    SPDX-FileCopyrightText: 2004 James Ots <kde@jamesots.com>
    SPDX-FileCopyrightText: 2011 Jiri Baum - NICTA <jiri@baum.com.au>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pageviewmouseannotation.h"

#include <qevent.h>
#include <qpainter.h>
#include <qtooltip.h>

#include "core/document.h"
#include "core/page.h"
#include "guiutils.h"
#include "pageview.h"
#include "videowidget.h"

static const int handleSize = 10;
static const int handleSizeHalf = handleSize / 2;

bool AnnotationDescription::isValid() const
{
    return (annotation != nullptr);
}

bool AnnotationDescription::isContainedInPage(const Okular::Document *document, int pageNumber) const
{
    if (AnnotationDescription::pageNumber == pageNumber) {
        /* Don't access page via pageViewItem here. pageViewItem might have been deleted. */
        const Okular::Page *page = document->page(pageNumber);
        if (page != nullptr) {
            if (page->annotations().contains(annotation)) {
                return true;
            }
        }
    }
    return false;
}

void AnnotationDescription::invalidate()
{
    annotation = nullptr;
    pageViewItem = nullptr;
    pageNumber = -1;
}

AnnotationDescription::AnnotationDescription(PageViewItem *newPageViewItem, const QPoint eventPos)
{
    const Okular::AnnotationObjectRect *annObjRect = nullptr;
    if (newPageViewItem) {
        const QRect &uncroppedPage = newPageViewItem->uncroppedGeometry();
        /* find out normalized mouse coords inside current item (nX and nY will be in the range of 0..1). */
        const double nX = newPageViewItem->absToPageX(eventPos.x());
        const double nY = newPageViewItem->absToPageY(eventPos.y());
        annObjRect = (Okular::AnnotationObjectRect *)newPageViewItem->page()->objectRect(Okular::ObjectRect::OAnnotation, nX, nY, uncroppedPage.width(), uncroppedPage.height());
    }

    if (annObjRect) {
        annotation = annObjRect->annotation();
        pageViewItem = newPageViewItem;
        pageNumber = pageViewItem->pageNumber();
    } else {
        invalidate();
    }
}

MouseAnnotation::MouseAnnotation(PageView *parent, Okular::Document *document)
    : QObject(parent)
    , m_document(document)
    , m_pageView(parent)
    , m_state(StateInactive)
    , m_handle(RH_None)
{
    m_resizeHandleList << RH_Left << RH_Right << RH_Top << RH_Bottom << RH_TopLeft << RH_TopRight << RH_BottomLeft << RH_BottomRight;
}

MouseAnnotation::~MouseAnnotation()
{
}

void MouseAnnotation::routeMousePressEvent(PageViewItem *pageViewItem, const QPoint eventPos)
{
    /* Is there a selected annotation? */
    if (m_focusedAnnotation.isValid()) {
        m_mousePosition = eventPos - pageViewItem->uncroppedGeometry().topLeft();
        m_handle = getHandleAt(m_mousePosition, m_focusedAnnotation);
        if (m_handle != RH_None) {
            /* Returning here means, the selection-rectangle gets control, unconditionally.
             * Even if it overlaps with another annotation. */
            return;
        }
    }

    AnnotationDescription ad(pageViewItem, eventPos);
    /* qDebug() << "routeMousePressEvent: eventPos = " << eventPos; */
    if (ad.isValid()) {
        if (ad.annotation->subType() == Okular::Annotation::AMovie || ad.annotation->subType() == Okular::Annotation::AScreen || ad.annotation->subType() == Okular::Annotation::AFileAttachment ||
            ad.annotation->subType() == Okular::Annotation::ARichMedia) {
            /* qDebug() << "routeMousePressEvent: trigger action for AMovie/AScreen/AFileAttachment"; */
            processAction(ad);
        } else {
            /* qDebug() << "routeMousePressEvent: select for modification"; */
            m_mousePosition = eventPos - pageViewItem->uncroppedGeometry().topLeft();
            m_handle = getHandleAt(m_mousePosition, ad);
            if (m_handle != RH_None) {
                setState(StateFocused, ad);
            }
        }
    } else {
        /* qDebug() << "routeMousePressEvent: no annotation under mouse, enter StateInactive"; */
        setState(StateInactive, ad);
    }
}

void MouseAnnotation::routeMouseReleaseEvent()
{
    if (isModified()) {
        /* qDebug() << "routeMouseReleaseEvent: finish command"; */
        finishCommand();
        setState(StateFocused, m_focusedAnnotation);
    }
    /*
    else
    {
        qDebug() << "routeMouseReleaseEvent: ignore";
    }
    */
}

void MouseAnnotation::routeMouseMoveEvent(PageViewItem *pageViewItem, const QPoint eventPos, bool leftButtonPressed)
{
    if (!pageViewItem) {
        /* qDebug() << "routeMouseMoveEvent: no pageViewItem provided, ignore"; */
        return;
    }

    if (leftButtonPressed) {
        if (isFocused()) {
            /* On first move event after annotation is selected, enter modification state */
            if (m_handle == RH_Content) {
                /* qDebug() << "routeMouseMoveEvent: handle " << m_handle << ", enter StateMoving"; */
                setState(StateMoving, m_focusedAnnotation);
            } else if (m_handle != RH_None) {
                /* qDebug() << "routeMouseMoveEvent: handle " << m_handle << ", enter StateResizing"; */
                setState(StateResizing, m_focusedAnnotation);
            }
        }

        if (isModified()) {
            /* qDebug() << "routeMouseMoveEvent: perform command, delta " << eventPos - m_mousePosition; */
            updateViewport(m_focusedAnnotation);
            performCommand(eventPos);
            m_mousePosition = eventPos - pageViewItem->uncroppedGeometry().topLeft();
            updateViewport(m_focusedAnnotation);
        }
    } else {
        if (isFocused()) {
            /* qDebug() << "routeMouseMoveEvent: update cursor for focused annotation, new eventPos " << eventPos; */
            m_mousePosition = eventPos - pageViewItem->uncroppedGeometry().topLeft();
            m_handle = getHandleAt(m_mousePosition, m_focusedAnnotation);
            m_pageView->updateCursor();
        }

        /* We get here quite frequently. */
        const AnnotationDescription ad(pageViewItem, eventPos);
        m_mousePosition = eventPos - pageViewItem->uncroppedGeometry().topLeft();
        if (ad.isValid()) {
            if (!(m_mouseOverAnnotation == ad)) {
                /* qDebug() << "routeMouseMoveEvent: Annotation under mouse (subtype " << ad.annotation->subType() << ", flags " << ad.annotation->flags() << ")"; */
                m_mouseOverAnnotation = ad;
                m_pageView->updateCursor();
            }
        } else {
            if (!(m_mouseOverAnnotation == ad)) {
                /* qDebug() << "routeMouseMoveEvent: Annotation disappeared under mouse."; */
                m_mouseOverAnnotation.invalidate();
                m_pageView->updateCursor();
            }
        }
    }
}

void MouseAnnotation::routeKeyPressEvent(const QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        cancel();
        break;
    case Qt::Key_Delete:
        if (m_focusedAnnotation.isValid()) {
            AnnotationDescription adToBeDeleted = m_focusedAnnotation;
            cancel();
            m_document->removePageAnnotation(adToBeDeleted.pageNumber, adToBeDeleted.annotation);
        }
        break;
    }
}

void MouseAnnotation::routeTooltipEvent(const QHelpEvent *helpEvent)
{
    /* qDebug() << "MouseAnnotation::routeTooltipEvent, event " << helpEvent; */
    if (m_mouseOverAnnotation.isValid() && m_mouseOverAnnotation.annotation->subType() != Okular::Annotation::AWidget) {
        /* get boundingRect in uncropped page coordinates */
        QRect boundingRect = Okular::AnnotationUtils::annotationGeometry(m_mouseOverAnnotation.annotation, m_mouseOverAnnotation.pageViewItem->uncroppedWidth(), m_mouseOverAnnotation.pageViewItem->uncroppedHeight());

        /* uncropped page to content area */
        boundingRect.translate(m_mouseOverAnnotation.pageViewItem->uncroppedGeometry().topLeft());
        /* content area to viewport */
        boundingRect.translate(-m_pageView->contentAreaPosition());

        const QString tip = GuiUtils::prettyToolTip(m_mouseOverAnnotation.annotation);
        QToolTip::showText(helpEvent->globalPos(), tip, m_pageView->viewport(), boundingRect);
    }
}

void MouseAnnotation::routePaint(QPainter *painter, const QRect paintRect)
{
    /* QPainter draws relative to the origin of uncropped viewport. */
    static const QColor borderColor = QColor::fromHsvF(0, 0, 1.0);
    static const QColor fillColor = QColor::fromHsvF(0, 0, 0.75, 0.66);

    if (!isFocused())
        return;
    /*
     * Get annotation bounding rectangle in uncropped page coordinates.
     * Distinction between AnnotationUtils::annotationGeometry() and AnnotationObjectRect::boundingRect() is,
     * that boundingRect would enlarge the QRect to a minimum size of 14 x 14.
     * This is useful for getting focus an a very small annotation,
     * but for drawing and modification we want the real size.
     */
    const QRect boundingRect = Okular::AnnotationUtils::annotationGeometry(m_focusedAnnotation.annotation, m_focusedAnnotation.pageViewItem->uncroppedWidth(), m_focusedAnnotation.pageViewItem->uncroppedHeight());

    if (!paintRect.intersects(boundingRect.translated(m_focusedAnnotation.pageViewItem->uncroppedGeometry().topLeft()).adjusted(-handleSizeHalf, -handleSizeHalf, handleSizeHalf, handleSizeHalf))) {
        /* Our selection rectangle is not in a region that needs to be (re-)drawn. */
        return;
    }

    painter->save();
    painter->translate(m_focusedAnnotation.pageViewItem->uncroppedGeometry().topLeft());
    painter->setPen(QPen(fillColor, 2, Qt::SolidLine, Qt::SquareCap, Qt::BevelJoin));
    painter->drawRect(boundingRect);
    if (m_focusedAnnotation.annotation->canBeResized()) {
        painter->setPen(borderColor);
        painter->setBrush(fillColor);
        for (const ResizeHandle &handle : qAsConst(m_resizeHandleList)) {
            QRect rect = getHandleRect(handle, m_focusedAnnotation);
            painter->drawRect(rect);
        }
    }
    painter->restore();
}

Okular::Annotation *MouseAnnotation::annotation() const
{
    if (m_focusedAnnotation.isValid()) {
        return m_focusedAnnotation.annotation;
    }
    return nullptr;
}

bool MouseAnnotation::isActive() const
{
    return (m_state != StateInactive);
}

bool MouseAnnotation::isMouseOver() const
{
    return (m_mouseOverAnnotation.isValid() || m_handle != RH_None);
}

bool MouseAnnotation::isFocused() const
{
    return (m_state == StateFocused);
}

bool MouseAnnotation::isMoved() const
{
    return (m_state == StateMoving);
}

bool MouseAnnotation::isResized() const
{
    return (m_state == StateResizing);
}

bool MouseAnnotation::isModified() const
{
    return (m_state == StateMoving || m_state == StateResizing);
}

Qt::CursorShape MouseAnnotation::cursor() const
{
    if (m_handle != RH_None) {
        if (isMoved()) {
            return Qt::SizeAllCursor;
        } else if (isFocused() || isResized()) {
            switch (m_handle) {
            case RH_Top:
                return Qt::SizeVerCursor;
            case RH_TopRight:
                return Qt::SizeBDiagCursor;
            case RH_Right:
                return Qt::SizeHorCursor;
            case RH_BottomRight:
                return Qt::SizeFDiagCursor;
            case RH_Bottom:
                return Qt::SizeVerCursor;
            case RH_BottomLeft:
                return Qt::SizeBDiagCursor;
            case RH_Left:
                return Qt::SizeHorCursor;
            case RH_TopLeft:
                return Qt::SizeFDiagCursor;
            case RH_Content:
                return Qt::SizeAllCursor;
            default:
                return Qt::OpenHandCursor;
            }
        }
    } else if (m_mouseOverAnnotation.isValid()) {
        /* Mouse is over annotation, but the annotation is not yet selected. */
        if (m_mouseOverAnnotation.annotation->subType() == Okular::Annotation::AMovie) {
            return Qt::PointingHandCursor;
        } else if (m_mouseOverAnnotation.annotation->subType() == Okular::Annotation::ARichMedia) {
            return Qt::PointingHandCursor;
        } else if (m_mouseOverAnnotation.annotation->subType() == Okular::Annotation::AScreen) {
            if (GuiUtils::renditionMovieFromScreenAnnotation(static_cast<const Okular::ScreenAnnotation *>(m_mouseOverAnnotation.annotation)) != nullptr) {
                return Qt::PointingHandCursor;
            }
        } else if (m_mouseOverAnnotation.annotation->subType() == Okular::Annotation::AFileAttachment) {
            return Qt::PointingHandCursor;
        } else {
            return Qt::ArrowCursor;
        }
    }

    /* There's no none cursor, so we still have to return something. */
    return Qt::ArrowCursor;
}

void MouseAnnotation::notifyAnnotationChanged(int pageNumber)
{
    const AnnotationDescription emptyAd;

    if (m_focusedAnnotation.isValid() && !m_focusedAnnotation.isContainedInPage(m_document, pageNumber)) {
        setState(StateInactive, emptyAd);
    }

    if (m_mouseOverAnnotation.isValid() && !m_mouseOverAnnotation.isContainedInPage(m_document, pageNumber)) {
        m_mouseOverAnnotation = emptyAd;
        m_pageView->updateCursor();
    }
}

void MouseAnnotation::updateAnnotationPointers()
{
    if (m_focusedAnnotation.annotation) {
        m_focusedAnnotation.annotation = m_document->page(m_focusedAnnotation.pageNumber)->annotation(m_focusedAnnotation.annotation->uniqueName());
    }

    if (m_mouseOverAnnotation.annotation) {
        m_mouseOverAnnotation.annotation = m_document->page(m_mouseOverAnnotation.pageNumber)->annotation(m_mouseOverAnnotation.annotation->uniqueName());
    }
}

void MouseAnnotation::cancel()
{
    if (isActive()) {
        finishCommand();
        setState(StateInactive, m_focusedAnnotation);
    }
}

void MouseAnnotation::reset()
{
    cancel();
    m_focusedAnnotation.invalidate();
    m_mouseOverAnnotation.invalidate();
}

/* Handle state changes for the focused annotation. */
void MouseAnnotation::setState(MouseAnnotationState state, const AnnotationDescription &ad)
{
    /* qDebug() << "setState: requested " << state; */
    if (m_focusedAnnotation.isValid()) {
        /* If there was a annotation before, request also repaint for the previous area. */
        updateViewport(m_focusedAnnotation);
    }

    if (!ad.isValid()) {
        /* qDebug() << "No annotation provided, forcing state inactive." << state; */
        state = StateInactive;
    } else if ((state == StateMoving && !ad.annotation->canBeMoved()) || (state == StateResizing && !ad.annotation->canBeResized())) {
        /* qDebug() << "Annotation does not support requested state, forcing state selected." << state; */
        state = StateInactive;
    }

    switch (state) {
    case StateMoving:
        m_focusedAnnotation = ad;
        m_focusedAnnotation.annotation->setFlags(m_focusedAnnotation.annotation->flags() | Okular::Annotation::BeingMoved);
        updateViewport(m_focusedAnnotation);
        break;
    case StateResizing:
        m_focusedAnnotation = ad;
        m_focusedAnnotation.annotation->setFlags(m_focusedAnnotation.annotation->flags() | Okular::Annotation::BeingResized);
        updateViewport(m_focusedAnnotation);
        break;
    case StateFocused:
        m_focusedAnnotation = ad;
        m_focusedAnnotation.annotation->setFlags(m_focusedAnnotation.annotation->flags() & ~(Okular::Annotation::BeingMoved | Okular::Annotation::BeingResized));
        updateViewport(m_focusedAnnotation);
        break;
    case StateInactive:
    default:
        if (m_focusedAnnotation.isValid()) {
            m_focusedAnnotation.annotation->setFlags(m_focusedAnnotation.annotation->flags() & ~(Okular::Annotation::BeingMoved | Okular::Annotation::BeingResized));
        }
        m_focusedAnnotation.invalidate();
        m_handle = RH_None;
    }

    /* qDebug() << "setState: enter " << state; */
    m_state = state;
    m_pageView->updateCursor();
}

/* Get the rectangular boundary of the given annotation, enlarged for space needed by resize handles.
 * Returns a QRect in page view item coordinates. */
QRect MouseAnnotation::getFullBoundingRect(const AnnotationDescription &ad) const
{
    QRect boundingRect;
    if (ad.isValid()) {
        boundingRect = Okular::AnnotationUtils::annotationGeometry(ad.annotation, ad.pageViewItem->uncroppedWidth(), ad.pageViewItem->uncroppedHeight());
        boundingRect = boundingRect.adjusted(-handleSizeHalf, -handleSizeHalf, handleSizeHalf, handleSizeHalf);
    }
    return boundingRect;
}

/* Apply the command determined by m_state to the currently focused annotation. */
void MouseAnnotation::performCommand(const QPoint newPos)
{
    const QRect &pageViewItemRect = m_focusedAnnotation.pageViewItem->uncroppedGeometry();
    QPointF mouseDelta(newPos - pageViewItemRect.topLeft() - m_mousePosition);
    QPointF normalizedRotatedMouseDelta(rotateInRect(QPointF(mouseDelta.x() / pageViewItemRect.width(), mouseDelta.y() / pageViewItemRect.height()), m_focusedAnnotation.pageViewItem->page()->rotation()));

    if (isMoved()) {
        m_document->translatePageAnnotation(m_focusedAnnotation.pageNumber, m_focusedAnnotation.annotation, Okular::NormalizedPoint(normalizedRotatedMouseDelta.x(), normalizedRotatedMouseDelta.y()));
    } else if (isResized()) {
        QPointF delta1, delta2;
        handleToAdjust(normalizedRotatedMouseDelta, delta1, delta2, m_handle, m_focusedAnnotation.pageViewItem->page()->rotation());
        m_document->adjustPageAnnotation(m_focusedAnnotation.pageNumber, m_focusedAnnotation.annotation, Okular::NormalizedPoint(delta1.x(), delta1.y()), Okular::NormalizedPoint(delta2.x(), delta2.y()));
    }
}

/* Finalize a command in progress for the currently focused annotation. */
void MouseAnnotation::finishCommand()
{
    /*
     * Note:
     * Translate-/resizePageAnnotation causes PopplerAnnotationProxy::notifyModification,
     * where modify flag needs to be already cleared. So it is important to call
     * setFlags before translatePageAnnotation-/adjustPageAnnotation.
     */
    if (isMoved()) {
        m_focusedAnnotation.annotation->setFlags(m_focusedAnnotation.annotation->flags() & ~Okular::Annotation::BeingMoved);
        m_document->translatePageAnnotation(m_focusedAnnotation.pageNumber, m_focusedAnnotation.annotation, Okular::NormalizedPoint(0.0, 0.0));
    } else if (isResized()) {
        m_focusedAnnotation.annotation->setFlags(m_focusedAnnotation.annotation->flags() & ~Okular::Annotation::BeingResized);
        m_document->adjustPageAnnotation(m_focusedAnnotation.pageNumber, m_focusedAnnotation.annotation, Okular::NormalizedPoint(0.0, 0.0), Okular::NormalizedPoint(0.0, 0.0));
    }
}

/* Tell viewport widget that the rectangular of the given annotation needs to be repainted. */
void MouseAnnotation::updateViewport(const AnnotationDescription &ad) const
{
    const QRect &changedPageViewItemRect = getFullBoundingRect(ad);
    if (changedPageViewItemRect.isValid()) {
        m_pageView->viewport()->update(changedPageViewItemRect.translated(ad.pageViewItem->uncroppedGeometry().topLeft()).translated(-m_pageView->contentAreaPosition()));
    }
}

/* eventPos: Mouse position in uncropped page coordinates.
   ad: The annotation to get the handle for. */
MouseAnnotation::ResizeHandle MouseAnnotation::getHandleAt(const QPoint eventPos, const AnnotationDescription &ad) const
{
    ResizeHandle selected = RH_None;

    if (ad.annotation->canBeResized()) {
        for (const ResizeHandle &handle : m_resizeHandleList) {
            const QRect rect = getHandleRect(handle, ad);
            if (rect.contains(eventPos)) {
                selected |= handle;
            }
        }

        /*
         * Handles may overlap when selection is very small.
         * Then it can happen that cursor is over more than one handles,
         * and therefore maybe more than two flags are set.
         * Favor one handle in that case.
         */
        if ((selected & RH_BottomRight) == RH_BottomRight)
            return RH_BottomRight;
        if ((selected & RH_TopRight) == RH_TopRight)
            return RH_TopRight;
        if ((selected & RH_TopLeft) == RH_TopLeft)
            return RH_TopLeft;
        if ((selected & RH_BottomLeft) == RH_BottomLeft)
            return RH_BottomLeft;
    }

    if (selected == RH_None && ad.annotation->canBeMoved()) {
        const QRect boundingRect = Okular::AnnotationUtils::annotationGeometry(ad.annotation, ad.pageViewItem->uncroppedWidth(), ad.pageViewItem->uncroppedHeight());
        if (boundingRect.contains(eventPos)) {
            return RH_Content;
        }
    }

    return selected;
}

/* Get the rectangle for a specified resizie handle. */
QRect MouseAnnotation::getHandleRect(ResizeHandle handle, const AnnotationDescription &ad) const
{
    const QRect boundingRect = Okular::AnnotationUtils::annotationGeometry(ad.annotation, ad.pageViewItem->uncroppedWidth(), ad.pageViewItem->uncroppedHeight());
    int left, top;

    if (handle & RH_Top) {
        top = boundingRect.top() - handleSizeHalf;
    } else if (handle & RH_Bottom) {
        top = boundingRect.bottom() - handleSizeHalf;
    } else {
        top = boundingRect.top() + boundingRect.height() / 2 - handleSizeHalf;
    }

    if (handle & RH_Left) {
        left = boundingRect.left() - handleSizeHalf;
    } else if (handle & RH_Right) {
        left = boundingRect.right() - handleSizeHalf;
    } else {
        left = boundingRect.left() + boundingRect.width() / 2 - handleSizeHalf;
    }

    return QRect(left, top, handleSize, handleSize);
}

/* Convert a resize handle delta into two adjust delta coordinates. */
void MouseAnnotation::handleToAdjust(const QPointF dIn, QPointF &dOut1, QPointF &dOut2, MouseAnnotation::ResizeHandle handle, Okular::Rotation rotation)
{
    const MouseAnnotation::ResizeHandle rotatedHandle = MouseAnnotation::rotateHandle(handle, rotation);
    dOut1.rx() = (rotatedHandle & MouseAnnotation::RH_Left) ? dIn.x() : 0;
    dOut1.ry() = (rotatedHandle & MouseAnnotation::RH_Top) ? dIn.y() : 0;
    dOut2.rx() = (rotatedHandle & MouseAnnotation::RH_Right) ? dIn.x() : 0;
    dOut2.ry() = (rotatedHandle & MouseAnnotation::RH_Bottom) ? dIn.y() : 0;
}

QPointF MouseAnnotation::rotateInRect(const QPointF rotated, Okular::Rotation rotation)
{
    QPointF ret;

    switch (rotation) {
    case Okular::Rotation90:
        ret = QPointF(rotated.y(), -rotated.x());
        break;
    case Okular::Rotation180:
        ret = QPointF(-rotated.x(), -rotated.y());
        break;
    case Okular::Rotation270:
        ret = QPointF(-rotated.y(), rotated.x());
        break;
    case Okular::Rotation0: /* no modifications */
    default:                /* other cases */
        ret = rotated;
    }

    return ret;
}

MouseAnnotation::ResizeHandle MouseAnnotation::rotateHandle(MouseAnnotation::ResizeHandle handle, Okular::Rotation rotation)
{
    unsigned int rotatedHandle = 0;
    switch (rotation) {
    case Okular::Rotation90:
        /* bit rotation: #1 => #4, #2 => #1, #3 => #2, #4 => #3 */
        rotatedHandle = (handle << 3 | handle >> (4 - 3)) & RH_AllHandles;
        break;
    case Okular::Rotation180:
        /* bit rotation: #1 => #3, #2 => #4, #3 => #1, #4 => #2 */
        rotatedHandle = (handle << 2 | handle >> (4 - 2)) & RH_AllHandles;
        break;
    case Okular::Rotation270:
        /* bit rotation: #1 => #2, #2 => #3, #3 => #4, #4 => #1 */
        rotatedHandle = (handle << 1 | handle >> (4 - 1)) & RH_AllHandles;
        break;
    case Okular::Rotation0: /* no modifications */
    default:                /* other cases */
        rotatedHandle = handle;
        break;
    }
    return (MouseAnnotation::ResizeHandle)rotatedHandle;
}

/* Start according action for AMovie/ARichMedia/AScreen/AFileAttachment.
 * It was formerly (before mouse annotation refactoring) called on mouse release event.
 * Now it's called on mouse press. Should we keep the former behavior? */
void MouseAnnotation::processAction(const AnnotationDescription &ad)
{
    if (ad.isValid()) {
        Okular::Annotation *ann = ad.annotation;
        PageViewItem *pageItem = ad.pageViewItem;

        if (ann->subType() == Okular::Annotation::AMovie) {
            VideoWidget *vw = pageItem->videoWidgets().value(static_cast<Okular::MovieAnnotation *>(ann)->movie());
            vw->show();
            vw->play();
        } else if (ann->subType() == Okular::Annotation::ARichMedia) {
            VideoWidget *vw = pageItem->videoWidgets().value(static_cast<Okular::RichMediaAnnotation *>(ann)->movie());
            vw->show();
            vw->play();
        } else if (ann->subType() == Okular::Annotation::AScreen) {
            m_document->processAction(static_cast<Okular::ScreenAnnotation *>(ann)->action());
        } else if (ann->subType() == Okular::Annotation::AFileAttachment) {
            const Okular::FileAttachmentAnnotation *fileAttachAnnot = static_cast<Okular::FileAttachmentAnnotation *>(ann);
            GuiUtils::saveEmbeddedFile(fileAttachAnnot->embeddedFile(), m_pageView);
        }
    }
}
