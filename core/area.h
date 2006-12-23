/***************************************************************************
 *   Copyright (C) 2004-05 by Enrico Ros <eros.kde@email.it>               *
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_AREA_H_
#define _OKULAR_AREA_H_

#include <QtCore/QList>
#include <QtGui/QColor>
#include <QtGui/QPainterPath>

#include <kdebug.h>

#include "okular_export.h"

class QPolygonF;
class QRect;

namespace Okular {

class Annotation;
class Link;
class NormalizedShape;

/**
 * NormalizedPoint is a helper class which stores the coordinates
 * of a normalized point. Normalized means that the coordinates are
 * between 0 and 1 so that it is page size independent.
 *
 * Example:
 *    The normalized point is (0.5, 0.3)
 *
 *    If you want to draw it on a 800x600 page, just multiply the x coordinate (0.5) with
 *    the page width (800) and the y coordinate (0.3) with the page height (600), so
 *    the point will be drawn on the page at (400, 180).
 *
 *    That allows you to zoom the page by just multiplying the normalized points with the
 *    zoomed page size.
 */
class OKULAR_EXPORT NormalizedPoint
{
    public:
        /**
         * Creates a new empty normalized point.
         */
        NormalizedPoint();

        /**
         * Creates a new normalized point with the normalized coordinates (@p x, @p y ).
         */
        NormalizedPoint( double x, double y );

        /**
         * Creates a new normalized point with the coordinates (@p x, @p y) which are normalized
         * by the scaling factors @p xScale and @p yScale.
         */
        NormalizedPoint( int x, int y, int xScale, int yScale );

        NormalizedPoint& operator=( const NormalizedPoint& );

        /**
         * Transforms the normalized point with the operations defined by @p matrix.
         */
        void transform( const QMatrix &matrix );

        /**
         * The normalized x coordinate.
         */
        double x;

        /**
         * The normalized y coordinate.
         */
        double y;
};

/**
 * NormalizedRect is a helper class which stores the coordinates
 * of a normalized rect, which is a rectangle of @see NormalizedPoints.
 */
class OKULAR_EXPORT NormalizedRect
{
    public:
        /**
         * Creates a null normalized rectangle.
         * @see isNull()
         */
        NormalizedRect();

        /**
         * Creates a normalized rectangle with the normalized coordinates
         * @p left, @p top, @p right, @p bottom.
         *
         * If you need the x, y, width and height coordinates use the
         * following formulas:
         *
         * @li x = left
         * @li y = top
         * @li width = right - left
         * @li height = bottom - top
         */
        NormalizedRect( double left, double top, double right, double bottom );

        /**
         * Creates a normalized rectangle of the given @p rectangle which is normalized
         * by the scaling factors @p xScale and @p yScale.
         */
        NormalizedRect( const QRect &rectangle, double xScale, double yScale );

        NormalizedRect( const NormalizedRect& );
        NormalizedRect& operator=( const NormalizedRect &other );

        /**
         * Returns whether this normalized rectangle is a null normalized rect.
         */
        bool isNull() const;

        /**
         * Returns whether the normalized rectangle contains the normalized coordinates
         * @p x and @p y.
         */
        bool contains( double x, double y ) const;

        /**
         * Returns whether the normalized rectangle intersects the @p other normalized
         * rectangle.
         */
        bool intersects( const NormalizedRect &other ) const;

        /**
         * This is an overloaded member function, provided for convenience. It behaves essentially
         * like the above function.
         */
        bool intersects( const NormalizedRect *other ) const;

        /**
         * Returns whether the normalized rectangle intersects an other normalized
         * rectangle, which is defined by @p left, @p top, @p right and @p bottom.
         */
        bool intersects( double left, double top, double right, double bottom ) const;

        /**
         * Returns the rectangle that accrues when the normalized rectangle is multiplyed
         * with the scaling @p xScale and @p yScale.
         */
        QRect geometry( int xScale, int yScale ) const;

        /**
         * Returns the normalized bounding rectangle of the normalized rectangle
         * combined with the @p other normalized rectangle.
         */
        NormalizedRect operator|( const NormalizedRect &other ) const;

        /**
         * Sets the normalized rectangle to the normalized bounding rectangle
         * of itself combined with the @p other normalized rectangle.
         */
        NormalizedRect& operator|=( const NormalizedRect &other );

        /**
         * Returns whether the normalized rectangle is equal to the @p other
         * normalized rectangle.
         */
        bool operator==( const NormalizedRect &other ) const;

        /**
         * Transforms the normalized rectangle with the operations defined by @p matrix.
         */
        void transform( const QMatrix &matrix );

        /**
         * The normalized left coordinate.
         */
        double left;

        /**
         * The normalized top coordinate.
         */
        double top;

        /**
         * The normalized right coordinate.
         */
        double right;

        /**
         * The normalized bottom coordinate.
         */
        double bottom;
};

/**
 * @short NormalizedRect that contains a reference to an object.
 *
 * These rects contains a pointer to a okular object (such as a link or something
 * like that). The pointer is read and stored as 'void pointer' so cast is
 * performed by accessors based on the value returned by objectType(). Objects
 * are reparented to this class.
 *
 * Type / Class correspondency tab:
 *  - Link      : class Link  : description of a link
 *  - Image     : class Image : description of an image (n/a)
 *  - Annotation: class Annotation: description of an annotation
 */
class OKULAR_EXPORT ObjectRect
{
    public:
        // definition of the types of storable objects
        enum ObjectType { Link, Image, OAnnotation, SourceRef };

        // default constructor: initialize all parameters
        ObjectRect( double l, double t, double r, double b, bool ellipse, ObjectType typ, void * obj );
        ObjectRect( const NormalizedRect& x, bool ellipse, ObjectType type, void * pnt ) ;
        ObjectRect( const QPolygonF &poly, ObjectType type, void * pnt ) ;
        virtual ~ObjectRect();

        // query type and get a const pointer to the stored object
        inline ObjectType objectType() const { return m_objectType; }
        inline const void * pointer() const { return m_pointer; }
        const QPainterPath &region() const;
        virtual QRect boundingRect( double xScale, double yScale ) const;
        virtual bool contains( double x, double y, double /*xScale*/, double /*yScale*/ ) const;
        virtual void transform( const QMatrix &matrix );

    protected:
        ObjectType m_objectType;
        void * m_pointer;
        QPainterPath m_path;
        QPainterPath m_transformed_path;
};

class OKULAR_EXPORT AnnotationObjectRect : public ObjectRect
{
    public:
        AnnotationObjectRect( Annotation * ann );
        virtual ~AnnotationObjectRect();

        virtual QRect boundingRect( double xScale, double yScale ) const;
        virtual bool contains( double x, double y, double xScale, double yScale ) const;
        inline Annotation * annotation() const { return m_ann; }
        virtual void transform( const QMatrix &matrix );

    private:
        Annotation * m_ann;
};

class OKULAR_EXPORT SourceRefObjectRect : public ObjectRect
{
    public:
        SourceRefObjectRect( const NormalizedPoint& point, void * scrRef );

        virtual QRect boundingRect( double xScale, double yScale ) const;
        virtual bool contains( double x, double y, double xScale, double yScale ) const;

    private:
        NormalizedPoint m_point;
};

/**
 * Internal Storage: normalized colored highlight owned by id
 */
struct HighlightRect : public NormalizedRect
{
    // searchID of the highlight owner
    int s_id;
    // color of the highlight
    QColor color;
};


/** @internal */
template <typename T>
class okularPtrUtils
{
public:
    static void doDelete( T& t )
    {
        (void)t;
    }

    static T* givePtr( T& t )
    {
        return &t;
    }
};

/** @internal */
template <typename T>
class okularPtrUtils<T*>
{
public:
    static void doDelete( T* t )
    {
        delete t;
    }

    static T* givePtr( T* t )
    {
        return t;
    }
};

/**
 * @short A regular area of NormalizedShape which normalizes a Shape
 *
 * Class NormalizedShape \b must have the following functions/operators defined:
 * - bool contains( double, double )
 * - bool intersects( NormalizedShape )
 * - bool isNull()
 * - Shape geometry( int, int )
 * - operator|=( NormalizedShape ) which unite two NormalizedShape's
 */
template <class NormalizedShape, class Shape> class RegularArea : public  QList<NormalizedShape>
{
    public:
        ~RegularArea();
        bool contains( double x, double y ) const;
        bool contains( const NormalizedShape& shape ) const;
        bool intersects (const RegularArea<NormalizedShape,Shape> * area) const;
        bool intersects (const NormalizedShape& shape) const;
        void appendArea (const RegularArea<NormalizedShape,Shape> *area);
        void appendShape( const NormalizedShape& shape );
        void simplify ();
        bool isNull() const;
        QList<Shape>* geometry( int xScale, int yScale, int dx=0,int dy=0 ) const;
};

template <class NormalizedShape, class Shape>
RegularArea<NormalizedShape, Shape>::~RegularArea<NormalizedShape, Shape>()
{
    int size = this->count();
    for ( int i = 0; i < size; ++i )
        okularPtrUtils<NormalizedShape>::doDelete( (*this)[i] );
}

template <class NormalizedShape, class Shape>
void RegularArea<NormalizedShape, Shape>::simplify()
{
#ifdef DEBUG_REGULARAREA
            int prev_end = this->count();
#endif
            int end = this->count() - 1, x = 0;
            for ( int i = 0; i < end; ++i )
            {
                    if ( (*this)[x]->intersects( (*this)[i+1] ) )
                    {
                        *((*this)[x]) |= *((*this)[i+1]);
                        NormalizedShape tobedeleted = (*this)[i+1];
                        this->removeAt( i + 1 );
                        okularPtrUtils<NormalizedShape>::doDelete( tobedeleted );
                        --end;
                        --i;
                    }
                    else
                    {
                        x=i+1;
                   }
            }
#ifdef DEBUG_REGULARAREA
            kDebug() << "from " << prev_end << " to " << this->count() << endl;
#endif
}

template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::isNull() const
{
    if ( !this )
        return false;

    if ( this->isEmpty() )
        return false;

    foreach ( const NormalizedShape& ns, *this )
        if ( !(ns->isNull()) )
            return false;

    return true;
}

template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::intersects( const NormalizedShape& rect ) const
{
    if ( !this )
        return false;

    if ( this->isEmpty() )
        return false;

    foreach ( const NormalizedShape& ns, *this )
        if ( !( ns->isNull() ) && ns->intersects( rect ) )
            return true;

    return false;
}

template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::intersects( const RegularArea<NormalizedShape,Shape> *area ) const
{
    if ( !this )
        return false;

    if ( this->isEmpty() )
        return false;

    foreach ( const NormalizedShape& ns, this )
    {
        foreach ( const Shape& shape, area )
        {
            if ( !(ns->isNull) && ns->intersects( shape ) )
                return true;
        }
    }

    return false;
}

template <class NormalizedShape, class Shape>
void RegularArea<NormalizedShape, Shape>::appendArea( const RegularArea<NormalizedShape, Shape> *area )
{
    if ( !this )
        return false;

    foreach( const Shape& shape, area )
        this->append( shape );
}


template <class NormalizedShape, class Shape>
void RegularArea<NormalizedShape, Shape>::appendShape( const NormalizedShape& shape )
{
    if ( !this )
        return;

    int size = this->count();
    // if the list is empty, adds the shape normally
    if ( size == 0 )
    {
        this->append( shape );
    }
    else
    {
        // if the new shape intersects with the last shape in the list, then
        // merge it with that and delete the shape
        if ( (*this)[size - 1]->intersects( shape ) )
        {
            *((*this)[size - 1]) |= *okularPtrUtils<NormalizedShape>::givePtr( shape );
            okularPtrUtils<NormalizedShape>::doDelete( shape );
        }
        else
            this->append( shape );
    }
}


template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::contains( double x, double y ) const
{
    if ( !this )
        return false;

    if ( this->isEmpty() )
        return false;

    foreach ( const NormalizedShape& ns, this )
        if ( ns->contains( x, y ) )
            return true;

    return false;
}

template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::contains( const NormalizedShape& shape ) const
{
    if ( !this )
        return false;

    if ( this->isEmpty() )
        return false;

    const QList<NormalizedShape*> * const list = dynamic_cast<const QList<NormalizedShape*> * const >( this );
    return list->contains( shape );
}

template <class NormalizedShape, class Shape>
QList<Shape> * RegularArea<NormalizedShape, Shape>::geometry( int xScale, int yScale, int dx, int dy ) const
{
    if ( !this )
        return false;

    if ( this->isEmpty() )
        return 0;

    QList<Shape>* ret = new QList<Shape>;
    Shape t;
    foreach( const NormalizedShape& ns, *this )
    {
        t = ns->geometry( xScale, yScale );
        t.translate( dx, dy );
        ret->append( t );
    }

    return ret;
}

typedef RegularArea<NormalizedRect*,QRect> RegularAreaRect;

class HighlightAreaRect : public RegularAreaRect
{
    public:
        HighlightAreaRect( const RegularAreaRect *area = 0 );

        // searchID of the highlight owner
        int s_id;
        // color of the highlight
        QColor color;
};

}

#endif
