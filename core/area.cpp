/***************************************************************************
 *   Copyright (C) 2004-05 by Enrico Ros <eros.kde@email.it>               *
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include <qrect.h>
#include <kdebug.h>
#include "link.h"
#include "area.h"

/** class NormalizedPoint **/
NormalizedPoint::NormalizedPoint()
    : x( 0.0 ), y( 0.0 ) {}

NormalizedPoint::NormalizedPoint( double dX, double dY )
    : x( dX ), y( dY ) {}

NormalizedPoint::NormalizedPoint( int iX, int iY, int xScale, int yScale )
    : x( (double)iX / (double)xScale ), y( (double)iY / (double)yScale ) {}


/** class NormalizedRect **/

NormalizedRect::NormalizedRect()
    : left( 0.0 ), top( 0.0 ), right( 0.0 ), bottom( 0.0 ) {}

NormalizedRect::NormalizedRect( double l, double t, double r, double b )
    // note: check for swapping coords?
    : left( l ), top( t ), right( r ), bottom( b ) {}

NormalizedRect::NormalizedRect( const QRect & r, double xScale, double yScale )
    : left( (double)r.left() / xScale ), top( (double)r.top() / yScale ),
    right( (double)r.right() / xScale ), bottom( (double)r.bottom() / yScale ) {}

bool NormalizedRect::isNull() const
{
    return left == 0 && top == 0 && right == 0 && bottom == 0;
}

bool NormalizedRect::contains( double x, double y ) const
{
    return x >= left && x <= right && y >= top && y <= bottom;
}

bool NormalizedRect::intersects( const NormalizedRect & r ) const
{
    return (r.left <= right) && (r.right >= left) && (r.top <= bottom) && (r.bottom >= top);
}

bool NormalizedRect::intersects( const NormalizedRect * r ) const
{
    return (r->left <= right) && (r->right >= left) && (r->top <= bottom) && (r->bottom >= top);
}

bool NormalizedRect::intersects( double l, double t, double r, double b ) const
{
    return (l <= right) && (r >= left) && (t <= bottom) && (b >= top);
}

QRect NormalizedRect::geometry( int xScale, int yScale ) const
{
    int l = (int)( left * xScale ),
        t = (int)( top * yScale ),
        r = (int)( right * xScale ),
        b = (int)( bottom * yScale );
    return QRect( l, t, r - l + 1, b - t + 1 );
}

HighlightAreaRect::HighlightAreaRect(RegularAreaRect *area)
{
	RegularAreaRect::Iterator i;
	for (i=area->begin();i!=area->end();++i)
	{
		append(*i);
	}
	s_id=-1;
	color=QColor();
}

/** class ObjectRect **/

ObjectRect::ObjectRect( double l, double t, double r, double b, ObjectType type, void * pnt )
    // assign coordinates swapping them if negative width or height    
    : NormalizedRect( r > l ? l : r, b > t ? t : b, r > l ? r : l, b > t ? b : t ),
    m_objectType( type ), m_pointer( pnt )
{
}

ObjectRect::~ObjectRect()
{
    if ( !m_pointer )
        return;

    if ( m_objectType == Link )
        delete static_cast<KPDFLink*>( m_pointer );
    else
        kdDebug() << "Object deletion not implemented for type '" << m_objectType << "' ." << endl;
}
