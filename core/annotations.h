/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_ANNOTATIONS_H_
#define _KPDF_ANNOTATIONS_H_

#include <qstring.h>
#include <qdatetime.h>
#include "page.h"

/**
 * @short Base options for an annotation (highlight, stamp, boxes, ..).
 *
 * From PDFreferece v.1.6:
 *  An annotation associates an object such as a note, sound, or movie with a
 *  location on a page of a PDF document ...
 *
 * Inherited classes must modify protected variables as appropriate.
 * Other fields in pdf reference we dropped here:
 *   -subtype, rectangle(we are a rect), border stuff
 */
class Annotation : public NormalizedRect
{
    public:
        Annotation();
        virtual ~Annotation();

        enum State { Creating, Modifying, Closed, Opened };
        enum MouseState { Normal, Hovered, Pressed };
        enum Flags { Hidden, NoOpenable, Print, Locked, ReadOnly };

        State state() const { return m_state; }
        const QString & text() const { return m_text; }
        const QString & uniqueName() const { return m_uniqueName; }
        const QDateTime & creationDate() const { return m_creationDate; }
        const QDateTime & modifyDate() const { return m_modifyDate; }
        const QColor & baseColor() const { return m_baseColor; }

        // event handlers (must update state)
        virtual void mousePressEvent( double x, double y, Qt::ButtonState b ) = 0;
        virtual void mouseMoveEvent( double x, double y, Qt::ButtonState b ) = 0;
        virtual void mouseReleaseEvent( double x, double y, Qt::ButtonState b ) = 0;

        // paint roughtly over a cleared area
        virtual void overlayPaint( QPainter * painter ) = 0;
        // cool-paint over a pixmap
        virtual void finalPaint( QPixmap * pixmap, MouseState mouseState ) = 0;

    protected:
        State m_state;
        MouseState m_mouseState;
        QString m_text;
        QString m_uniqueName;
        QDateTime m_modifyDate;
        QColor m_baseColor;

    private:
        QDateTime m_creationDate;
};

class TextAnnotation : public Annotation
{
    //Text (post-it like)
    //FreeText (direct on page)
    enum Type { InPlace, Popup };
};

class LineAnnotation : public Annotation
{
    //Line (arrows too)
};

class GeomAnnotation : public Annotation
{
    //Square, Circle
};

class PathAnnotation : public Annotation
{
    //Ink (one or more disjoints paths)
    //Polygon, PolyLine
};

class HighlightAnnotation : public Annotation
{
    //Highlight, Underline, Squiggly, StrikeOut, BLOCK
    enum BrushHor { Horizontal, Vertical };
};

class StampAnnotation : public Annotation
{
    // (14 default symbols + ours)
};

class MediaAnnotation : public Annotation
{
    //FileAttachment, Sound, Movie
    enum Type { FileAttachment, Sound, Movie };
};

#endif
