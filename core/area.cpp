/***************************************************************************
 *   Copyright (C) 2004-05 by Enrico Ros <eros.kde@email.it>               *
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QRect>

#include <math.h>

#include "annotations.h"
#include "area.h"
#include "link.h"

using namespace Okular;

/** class NormalizedPoint **/
NormalizedPoint::NormalizedPoint()
    : x( 0.0 ), y( 0.0 ) {}

NormalizedPoint::NormalizedPoint( double dX, double dY )
    : x( dX ), y( dY ) {}

NormalizedPoint::NormalizedPoint( int iX, int iY, int xScale, int yScale )
    : x( (double)iX / (double)xScale ), y( (double)iY / (double)yScale ) {}

NormalizedPoint& NormalizedPoint::operator=( const NormalizedPoint & p )
{
    x = p.x;
    y = p.y;
    return *this;
}


/** class NormalizedRect **/

NormalizedRect::NormalizedRect()
    : left( 0.0 ), top( 0.0 ), right( 0.0 ), bottom( 0.0 ) {}

NormalizedRect::NormalizedRect( double l, double t, double r, double b )
    // note: check for swapping coords?
    : left( l ), top( t ), right( r ), bottom( b ) {}

NormalizedRect::NormalizedRect( const QRect & r, double xScale, double yScale )
    : left( (double)r.left() / xScale ), top( (double)r.top() / yScale ),
    right( (double)r.right() / xScale ), bottom( (double)r.bottom() / yScale ) {}

NormalizedRect::NormalizedRect( const NormalizedRect & rect )
    : left( rect.left ), top( rect.top ), right( rect.right ), bottom( rect.bottom ) {}

bool NormalizedRect::isNull() const
{
    return left == 0 && top== 0 && right == 0 && bottom == 0;
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

NormalizedRect NormalizedRect::operator| (const NormalizedRect & r) const
{
	NormalizedRect ret;
 // todo !       
	ret.left=qMin(left,r.left);
        ret.top=qMin(top,r.top);
        ret.bottom=qMax(bottom,r.bottom);
        ret.right=qMax(right,r.right);
	return ret;
}

NormalizedRect& NormalizedRect::operator|= (const NormalizedRect & r)
{
	return ((*this) = (*this) | r );
}

NormalizedRect & NormalizedRect::operator=( const NormalizedRect & r )
{
    left = r.left;
    right = r.right;
    top = r.top;
    bottom = r.bottom;
    return *this;
}

bool NormalizedRect::operator==( const NormalizedRect & r ) const
{
    return ( isNull() && r.isNull() ) ||
       ( fabs( left - r.left ) < 1e-4 &&
         fabs( right - r.right ) < 1e-4 &&
         fabs( top - r.top ) < 1e-4 &&
         fabs( bottom - r.bottom ) < 1e-4 );
}

/*
kdbgstream& operator << (kdbgstream& str , const NormalizedRect &r)
{
    str << "[" <<r.left() << "," << r.top() << "] x "<< "[" <<r.right() << "," << r.bottom() << "]";
    return str;
}*/

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
		append(new NormalizedRect(*(*i)));
	}
	s_id=-1;
	color=QColor();
}

/** class ObjectRect **/

ObjectRect::ObjectRect( double l, double t, double r, double b, bool ellipse, ObjectType type, void * pnt )
    : m_objectType( type ), m_pointer( pnt )
{
    // assign coordinates swapping them if negative width or height
    QRectF rect( r > l ? l : r, b > t ? t : b, fabs( r - l ), fabs( b - t ) );
    if ( ellipse )
        m_path.addEllipse( rect );
    else
        m_path.addRect( rect );
}

ObjectRect::ObjectRect( NormalizedRect x, bool ellipse, ObjectType type, void * pnt )
    : m_objectType( type ), m_pointer( pnt )
{
    QRectF rect( x.left, x.top, fabs( x.right - x.left ), fabs( x.bottom - x.top ) );
    if ( ellipse )
        m_path.addEllipse( rect );
    else
        m_path.addRect( rect );
}

ObjectRect::ObjectRect( const QPolygonF &poly, ObjectType type, void * pnt )
    : m_objectType( type ), m_pointer( pnt )
{
    m_path.addPolygon( poly );
}

QRect ObjectRect::boundingRect( double xScale, double yScale ) const
{
    const QRectF &br = m_path.boundingRect();
    return QRect( (int)( br.left() * xScale ), (int)( br.top() * yScale ),
                  (int)( br.width() * xScale ), (int)( br.height() * yScale ) );
}

ObjectRect::~ObjectRect()
{
    if ( !m_pointer )
        return;

    if ( m_objectType == Link )
        delete static_cast<Okular::Link*>( m_pointer );
    else
        kDebug() << "Object deletion not implemented for type '" << m_objectType << "' ." << endl;
}

/** class AnnotationObjectRect **/

AnnotationObjectRect::AnnotationObjectRect( Annotation * ann )
    : ObjectRect( QPolygonF(), OAnnotation, ann ), m_ann( ann )
{
}

QRect AnnotationObjectRect::boundingRect( double xScale, double yScale ) const
{
    if ( m_ann->subType() == Annotation::AText && ( ( (TextAnnotation*)m_ann )->textType == TextAnnotation::Linked ) )
    {
        return QRect( (int)( m_ann->boundary.left * xScale ), (int)( m_ann->boundary.top * yScale ), 24, 24 );
    }
    return m_ann->boundary.geometry( (int)xScale, (int)yScale );
}

bool AnnotationObjectRect::contains( double x, double y, double xScale, double yScale ) const
{
    return boundingRect( xScale, yScale ).contains( (int)( x * xScale ), (int)( y * yScale ), false );
}

AnnotationObjectRect::~AnnotationObjectRect()
{
    // the annotation pointer is kept elsewehere (in Page, most probably),
    // so just release its pointer
    m_pointer = 0;
}

