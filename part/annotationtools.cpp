/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "annotationtools.h"

// qt / kde includes
#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QMouseEvent>

// local includes
#include "core/annotations.h"

AnnotatorEngine::AnnotatorEngine(const QDomElement &engineElement)
    : m_engineElement(engineElement)
    , m_creationCompleted(false)
    , m_item(nullptr)
{
    // parse common engine attributes
    if (engineElement.hasAttribute(QStringLiteral("color"))) {
        m_engineColor = QColor(engineElement.attribute(QStringLiteral("color")));
    }

    // get the annotation element
    QDomElement annElement = m_engineElement.firstChild().toElement();
    if (!annElement.isNull() && annElement.tagName() == QLatin1String("annotation")) {
        m_annotElement = annElement;
    }
}

void AnnotatorEngine::decodeEvent(const QMouseEvent *mouseEvent, EventType *eventType, Button *button)
{
    *eventType = AnnotatorEngine::Press;
    if (mouseEvent->type() == QEvent::MouseMove) {
        *eventType = AnnotatorEngine::Move;
    } else if (mouseEvent->type() == QEvent::MouseButtonRelease) {
        *eventType = AnnotatorEngine::Release;
    }

    *button = AnnotatorEngine::None;
    const Qt::MouseButtons buttonState = (*eventType == AnnotatorEngine::Move) ? mouseEvent->buttons() : mouseEvent->button();
    if (buttonState == Qt::LeftButton) {
        *button = AnnotatorEngine::Left;
    } else if (buttonState == Qt::RightButton) {
        *button = AnnotatorEngine::Right;
    }
}

void AnnotatorEngine::decodeEvent(const QTabletEvent *tabletEvent, EventType *eventType, Button *button)
{
    switch (tabletEvent->type()) {
    case QEvent::TabletPress:
        // Tablet press event is equivalent to pressing the left mouse button
        *button = AnnotatorEngine::Left;
        *eventType = AnnotatorEngine::Press;
        break;
    case QEvent::TabletRelease:
        // Tablet release event is equivalent to releasing the left mouse button
        *button = AnnotatorEngine::Left;
        *eventType = AnnotatorEngine::Release;
        break;
    case QEvent::TabletMove:
        // Tablet events are only routed if the pen is down so
        // this is equivalent to the left mouse button being pressed
        *button = AnnotatorEngine::Left;
        *eventType = AnnotatorEngine::Move;
        break;
    default:
        Q_ASSERT(false);
        break;
    }
}

AnnotatorEngine::~AnnotatorEngine()
{
}

QCursor AnnotatorEngine::cursor() const
{
    return Qt::CrossCursor;
}

SmoothPath::SmoothPath(const QList<Okular::NormalizedPoint> &points, const QPen &pen, qreal opacity, QPainter::CompositionMode compositionMode)
    : points(points)
    , pen(pen)
    , opacity(opacity)
    , compositionMode(compositionMode)
{
}

/** SmoothPathEngine */
SmoothPathEngine::SmoothPathEngine(const QDomElement &engineElement)
    : AnnotatorEngine(engineElement)
    , compositionMode(QPainter::CompositionMode_SourceOver)
{
    // parse engine specific attributes
    if (engineElement.attribute(QStringLiteral("compositionMode"), QStringLiteral("sourceOver")) == QLatin1String("clear")) {
        compositionMode = QPainter::CompositionMode_Clear;
    }
}

QRect SmoothPathEngine::event(EventType type, Button button, Modifiers /*modifiers*/, double nX, double nY, double xScale, double yScale, const Okular::Page * /*page*/)
{
    // only proceed if pressing left button
    if (button != Left) {
        return QRect();
    }

    // start operation
    if (type == Press && points.isEmpty()) {
        lastPoint.x = nX;
        lastPoint.y = nY;
        totalRect.left = totalRect.right = lastPoint.x;
        totalRect.top = totalRect.bottom = lastPoint.y;
        points.append(lastPoint);
    }
    // add a point to the path
    else if (type == Move && points.count() > 0) {
        // double dist = hypot( nX - points.last().x, nY - points.last().y );
        // if ( dist > 0.0001 )
        //{
        // append mouse position (as normalized point) to the list
        Okular::NormalizedPoint nextPoint = Okular::NormalizedPoint(nX, nY);
        points.append(nextPoint);
        // update total rect
        totalRect.left = qMin(totalRect.left, nX);
        totalRect.top = qMin(totalRect.top, nY);
        totalRect.right = qMax(nX, totalRect.right);
        totalRect.bottom = qMax(nY, totalRect.bottom);
        // paint the difference to previous full rect
        Okular::NormalizedRect incrementalRect;
        incrementalRect.left = qMin(nextPoint.x, lastPoint.x);
        incrementalRect.right = qMax(nextPoint.x, lastPoint.x);
        incrementalRect.top = qMin(nextPoint.y, lastPoint.y);
        incrementalRect.bottom = qMax(nextPoint.y, lastPoint.y);
        lastPoint = nextPoint;
        return incrementalRect.geometry((int)xScale, (int)yScale);
        //}
    }
    // terminate process
    else if (type == Release && points.count() > 0) {
        if (points.count() < 2) {
            points.clear();
        } else {
            m_creationCompleted = true;
        }
        return totalRect.geometry((int)xScale, (int)yScale);
    }
    return QRect();
}

void SmoothPathEngine::paint(QPainter *painter, double xScale, double yScale, const QRect & /*clipRect*/)
{
    const double penWidth = m_annotElement.attribute(QStringLiteral("width"), QStringLiteral("1")).toInt();
    const qreal opacity = m_annotElement.attribute(QStringLiteral("opacity"), QStringLiteral("1.0")).toDouble();

    // use engine's color for painting
    const SmoothPath path(points, QPen(m_engineColor, penWidth), opacity, compositionMode);

    // draw the path
    path.paint(painter, xScale, yScale);
}

void SmoothPath::paint(QPainter *painter, double xScale, double yScale) const
{
    // draw SmoothPaths with at least 2 points
    if (points.count() > 1) {
        painter->setCompositionMode(compositionMode);
        painter->setPen(pen);
        painter->setOpacity(opacity);

        QPainterPath path;
        QList<Okular::NormalizedPoint>::const_iterator pIt = points.begin(), pEnd = points.end();
        path.moveTo(QPointF(pIt->x * xScale, pIt->y * yScale));
        ++pIt;
        for (; pIt != pEnd; ++pIt) {
            path.lineTo(QPointF(pIt->x * xScale, pIt->y * yScale));
        }
        painter->drawPath(path);
    }
}

QList<Okular::Annotation *> SmoothPathEngine::end()
{
    m_creationCompleted = false;

    // find out annotation's description node
    if (m_annotElement.isNull()) {
        return QList<Okular::Annotation *>();
    }

    // find out annotation's type
    Okular::Annotation *ann = nullptr;
    QString typeString = m_annotElement.attribute(QStringLiteral("type"));

    // create InkAnnotation from path
    if (typeString == QLatin1String("Ink")) {
        Okular::InkAnnotation *ia = new Okular::InkAnnotation();
        ann = ia;
        if (m_annotElement.hasAttribute(QStringLiteral("width"))) {
            ann->style().setWidth(m_annotElement.attribute(QStringLiteral("width")).toDouble());
        }
        // fill points
        QList<QList<Okular::NormalizedPoint>> list = ia->inkPaths();
        list.append(points);
        ia->setInkPaths(list);
        // set boundaries
        ia->setBoundingRectangle(totalRect);
    }

    // safety check
    if (!ann) {
        return QList<Okular::Annotation *>();
    }

    // set common attributes
    ann->style().setColor(m_annotElement.hasAttribute(QStringLiteral("color")) ? m_annotElement.attribute(QStringLiteral("color")) : m_engineColor);
    if (m_annotElement.hasAttribute(QStringLiteral("opacity"))) {
        ann->style().setOpacity(m_annotElement.attribute(QStringLiteral("opacity"), QStringLiteral("1.0")).toDouble());
    }

    // return annotation
    return QList<Okular::Annotation *>() << ann;
}

SmoothPath SmoothPathEngine::endSmoothPath()
{
    m_creationCompleted = false;

    QColor color(m_annotElement.hasAttribute(QStringLiteral("color")) ? m_annotElement.attribute(QStringLiteral("color")) : m_engineColor);

    const int width = m_annotElement.attribute(QStringLiteral("width"), QStringLiteral("2")).toInt();
    const qreal opacity = m_annotElement.attribute(QStringLiteral("opacity"), QStringLiteral("1.0")).toDouble();

    return SmoothPath(points, QPen(color, width), opacity, compositionMode);
}

/* kate: replace-tabs on; indent-width 4; */
