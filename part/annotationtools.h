/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_ANNOTATIONTOOLS_H_
#define _OKULAR_ANNOTATIONTOOLS_H_

#include <QLinkedList>
#include <QPainter>
#include <QPen>
#include <QRect>
#include <qdom.h>

#include "core/area.h"

class QMouseEvent;
class QTabletEvent;
class PageViewItem;
namespace Okular
{
class Annotation;
class Page;
}

/**
 * @short Engine: filter events to distill Annotations.
 */
class AnnotatorEngine
{
public:
    explicit AnnotatorEngine(const QDomElement &engineElement);
    virtual ~AnnotatorEngine();

    AnnotatorEngine(const AnnotatorEngine &) = delete;
    AnnotatorEngine &operator=(const AnnotatorEngine &) = delete;

    // enum definitions
    enum EventType { Press, Move, Release };
    enum Button { None, Left, Right };
    /** To tell the annotator engine about modifier keys and other special wishes */
    struct Modifiers {
        bool constrainRatioAndAngle; ///< Whether the engine shall snap to certain angles, if supported.
    };

    // perform operations
    virtual QRect event(EventType type, Button button, Modifiers modifiers, double nX, double nY, double xScale, double yScale, const Okular::Page *page) = 0;
    virtual void paint(QPainter *painter, double xScale, double yScale, const QRect &clipRect) = 0;
    virtual QList<Okular::Annotation *> end() = 0;

    // query creation state
    // PageViewItem * editingItem() const { return m_lockedItem; }
    bool creationCompleted() const
    {
        return m_creationCompleted;
    }

    void setItem(PageViewItem *item)
    {
        m_item = item;
    }

    static void decodeEvent(const QMouseEvent *mouseEvent, EventType *eventType, Button *button);
    static void decodeEvent(const QTabletEvent *tabletEvent, EventType *eventType, Button *button);

    virtual QCursor cursor() const;

protected:
    PageViewItem *item()
    {
        return m_item;
    }

    // common engine attributes (the element includes annotation desc)
    QDomElement m_engineElement;
    QDomElement m_annotElement;
    QColor m_engineColor;
    // other vars (remove this!)
    bool m_creationCompleted;

private:
    PageViewItem *m_item;
};

class SmoothPath
{
public:
    SmoothPath(const QLinkedList<Okular::NormalizedPoint> &points, const QPen &pen, qreal opacity = 1.0, QPainter::CompositionMode compositionMode = QPainter::CompositionMode_SourceOver);
    void paint(QPainter *painter, double xScale, double yScale) const;

private:
    const QLinkedList<Okular::NormalizedPoint> points;
    const QPen pen;
    const qreal opacity;
    const QPainter::CompositionMode compositionMode;
};

/** @short SmoothPathEngine */
class SmoothPathEngine : public AnnotatorEngine
{
public:
    explicit SmoothPathEngine(const QDomElement &engineElement);

    QRect event(EventType type, Button button, Modifiers modifiers, double nX, double nY, double xScale, double yScale, const Okular::Page * /*page*/) override;

    void paint(QPainter *painter, double xScale, double yScale, const QRect & /*clipRect*/) override;

    // These are two alternative ways to get the resulting path. Don't call them both!
    QList<Okular::Annotation *> end() override;
    SmoothPath endSmoothPath();

private:
    // data
    QLinkedList<Okular::NormalizedPoint> points;
    Okular::NormalizedRect totalRect;
    Okular::NormalizedPoint lastPoint;
    QPainter::CompositionMode compositionMode;
};

#endif

/* kate: replace-tabs on; indent-width 4; */
