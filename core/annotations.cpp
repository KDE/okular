/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt / kde includes
#include <qrect.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <klocale.h>

// system includes
#include <math.h>

// local includes
#include "annotations.h"

Annotation::Annotation()
    : NormalizedRect(), m_state( NewBorn )
{
}

Annotation::~Annotation()
{
}
/*
void mousePressEvent( double x, double y, Qt::ButtonState b );
void mouseMoveEvent( double x, double y, Qt::ButtonState b );
void mouseReleaseEvent( double x, double y, Qt::ButtonState b );
void overlayPaint( QPainter * painter );
void finalPaint( QPixmap * pixmap, MouseState mouseState );
*/

HighlightAnnotation::HighlightAnnotation( Type type )
    : Annotation(), m_type( type )
{
}

QString HighlightAnnotation::usageTip() const
{
    return i18n( "Press and drag the mouse to highlight words." );
}

bool HighlightAnnotation::mouseEvent( MouseEvent e, double x, double y, Qt::ButtonState b )
{
    // only process events if left button pressed
    if ( b != Qt::LeftButton )
        return false;

    // start operation
    if ( e == MousePress && m_state == NewBorn )
    {
        FloatPoint newPoint;
        left = right = newPoint.x = x;
        top = bottom = newPoint.y = y;
        m_points.append( newPoint );
        m_state = Creating;
        return false;
    }
    // add a point to the path, TODO inline redundancy suppression
    else if ( e == MouseMove && m_state == Creating )
    {
        double dist = hypot( x - m_points.last().x, y - m_points.last().y );
        if ( dist < 0.02 )
            return false;
        left = QMIN( left, x );
        top = QMIN( y, top );
        right = QMAX( x, right );
        bottom = QMAX( y, bottom );
        FloatPoint newPoint;
        newPoint.x = x;
        newPoint.y = y;
        m_points.append( newPoint );
        return true;
    }
    // terminate precess
    else if ( e == MouseRelease && m_state == Creating )
    {
        if ( m_points.count() > 2 )
            m_state = Opened;
        else
            m_state = NewBorn;
        return false;
    }

    // nothing change -> don't update gfx
    return false;
}

void HighlightAnnotation::paintOverlay( QPainter * painter, int xScale, int yScale, const QRect & limits )
{
    // draw annotations whith at least 2 points
    if ( m_points.count() < 2 )
        return;

    // use basecolor
    painter->setPen( QPen(m_baseColor, 3) );

    // perform a rought paint drawing meanline only
    //const QRect myRect = geometry( xScale, yScale );

    QValueList<FloatPoint>::iterator pIt = m_points.begin(), pEnd = m_points.end();
    FloatPoint pA = *pIt;
    ++pIt;
    for ( ; pIt != pEnd; ++pIt )
    {
        FloatPoint pB = *pIt;
//        painter->setPen( QPen(Qt::black, 2) );
//        painter->drawLine( (int)(pA.x * (double)xScale)+1, (int)(pA.y * (double)yScale)+1,
//                           (int)(pB.x * (double)xScale)+1, (int)(pB.y * (double)yScale)+1 );
        painter->drawLine( (int)(pA.x * (double)xScale), (int)(pA.y * (double)yScale),
                           (int)(pB.x * (double)xScale), (int)(pB.y * (double)yScale) );
        pA = pB;
    }
}

void HighlightAnnotation::paintFinal( QImage & /*backImage*/, int /*xScale*/, int /*yScale*/, const QRect & /*limits*/ )
{
    // use m_type for painting
}
