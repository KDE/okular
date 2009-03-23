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

#include <okular/core/global.h>
#include <okular/core/okular_export.h>

class QPolygonF;
class QRect;

namespace Okular {

class Annotation;
class Action;
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

        /**
         * @internal
         */
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

        /**
         * @internal
         */
        NormalizedRect( const NormalizedRect& );

        /**
         * @internal
         */
        NormalizedRect& operator=( const NormalizedRect &other );

        /**
         * Build a normalized rect from a QRectF.
         */
        static NormalizedRect fromQRectF( const QRectF &rect );

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
         * Returns the intersection of this normalized rectangle with the specified
         * @p other. If the rects do not intersect then the result is null.
         *
         * @since 0.7 (KDE 4.1)
         */
        NormalizedRect operator&( const NormalizedRect &other ) const;

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
KDE_DUMMY_QHASH_FUNCTION(NormalizedRect)

/**
 * @short NormalizedRect that contains a reference to an object.
 *
 * These rects contains a pointer to a okular object (such as an action or something
 * like that). The pointer is read and stored as 'void pointer' so cast is
 * performed by accessors based on the value returned by objectType(). Objects
 * are reparented to this class.
 *
 * Type / Class correspondency tab:
 *  - Action    : class Action: description of an action
 *  - Image     : class Image : description of an image (n/a)
 *  - Annotation: class Annotation: description of an annotation
 */
class OKULAR_EXPORT ObjectRect
{
    public:
        /**
         * Describes the type of storable object.
         */
        enum ObjectType
        {
            Action,      ///< An action
            Image,       ///< An image
            OAnnotation, ///< An annotation
            SourceRef    ///< A source reference
        };

        /**
         * Creates a new object rectangle.
         *
         * @param left The left coordinate of the rectangle.
         * @param top The top coordinate of the rectangle.
         * @param right The right coordinate of the rectangle.
         * @param bottom The bottom coordinate of the rectangle.
         * @param ellipse If true the rectangle describes an ellipse.
         * @param type The type of the storable object @see ObjectType.
         * @param object The pointer to the storable object.
         */
        ObjectRect( double left, double top, double right, double bottom, bool ellipse, ObjectType type, void *object );

        /**
         * This is an overloaded member function, provided for convenience.
         */
        ObjectRect( const NormalizedRect &rect, bool ellipse, ObjectType type, void *object );

        /**
         * This is an overloaded member function, provided for convenience.
         */
        ObjectRect( const QPolygonF &poly, ObjectType type, void *object );

        /**
         * Destroys the object rectangle.
         */
        virtual ~ObjectRect();

        /**
         * Returns the object type of the object rectangle.
         * @see ObjectType
         */
        ObjectType objectType() const;

        /**
         * Returns the storable object of the object rectangle.
         */
        const void *object() const;

        /**
         * Returns the region that is covered by the object rectangle.
         */
        const QPainterPath &region() const;

        /**
         * Returns the bounding rect of the object rectangle for the
         * scaling factor @p xScale and @p yScale.
         */
        virtual QRect boundingRect( double xScale, double yScale ) const;

        /**
         * Returns whether the object rectangle contains the point @p x, @p y for the
         * scaling factor @p xScale and @p yScale.
         */
        virtual bool contains( double x, double y, double xScale, double yScale ) const;

        /**
         * Transforms the object rectangle with the operations defined by @p matrix.
         */
        virtual void transform( const QMatrix &matrix );

        /**
         * Returns the square of the distance between the object and the point @p x, @p y
         * for the scaling factor @p xScale and @p yScale.
         *
         * @since 0.8.2 (KDE 4.2.2)
         */
        // FIXME this should most probably be a virtual method
        double distanceSqr( double x, double y, double xScale, double yScale ) const;

    protected:
        ObjectType m_objectType;
        void * m_object;
        QPainterPath m_path;
        QPainterPath m_transformedPath;
};

/**
 * This class describes the object rectangle for an annotation.
 */
class OKULAR_EXPORT AnnotationObjectRect : public ObjectRect
{
    public:
        /**
         * Creates a new annotation object rectangle with the
         * given @p annotation.
         */
        AnnotationObjectRect( Annotation *annotation );

        /**
         * Destroys the annotation object rectangle.
         */
        virtual ~AnnotationObjectRect();

        /**
         * Returns the annotation object of the annotation object rectangle.
         */
        Annotation *annotation() const;

        /**
         * Returns the bounding rect of the annotation object rectangle for the
         * scaling factor @p xScale and @p yScale.
         */
        virtual QRect boundingRect( double xScale, double yScale ) const;

        /**
         * Returns whether the annotation object rectangle contains the point @p x, @p y for the
         * scaling factor @p xScale and @p yScale.
         */
        virtual bool contains( double x, double y, double xScale, double yScale ) const;

        /**
         * Transforms the annotation object rectangle with the operations defined by @p matrix.
         */
        virtual void transform( const QMatrix &matrix );

    private:
        Annotation * m_annotation;
};

/**
 * This class describes the object rectangle for a source reference.
 */
class OKULAR_EXPORT SourceRefObjectRect : public ObjectRect
{
    friend class ObjectRect;

    public:
        /**
         * Creates a new source reference object rectangle.
         *
         * @param point The point of the source reference.
         * @param reference The storable source reference object.
         */
        SourceRefObjectRect( const NormalizedPoint& point, void *reference );

        /**
         * Returns the bounding rect of the source reference object rectangle for the
         * scaling factor @p xScale and @p yScale.
         */
        virtual QRect boundingRect( double xScale, double yScale ) const;

        /**
         * Returns whether the source reference object rectangle contains the point @p x, @p y for the
         * scaling factor @p xScale and @p yScale.
         */
        virtual bool contains( double x, double y, double xScale, double yScale ) const;

    private:
        NormalizedPoint m_point;
};

/// @cond PRIVATE
/** @internal */
template <typename T>
void doDelete( T& t )
{
    (void)t;
}

/** @internal */
template <typename T>
T* givePtr( T& t )
{
    return &t;
}

/** @internal */
template <typename T>
T& deref( T& t )
{
    return t;
}

/** @internal */
template <typename T>
static void doDelete( T* t )
{
    delete t;
}

/** @internal */
template <typename T>
static T* givePtr( T* t )
{
    return t;
}

/** @internal */
template <typename T>
static T& deref( T* t )
{
    return *t;
}
/// @endcond

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
        /**
         * Destroys a regular area.
         */
        ~RegularArea();

        /**
         * Returns whether the regular area contains the
         * normalized point @p x, @p y.
         */
        bool contains( double x, double y ) const;

        /**
         * Returns whether the regular area contains the
         * given @p shape.
         */
        bool contains( const NormalizedShape& shape ) const;

        /**
         * Returns whether the regular area intersects with the given @p area.
         */
        bool intersects( const RegularArea<NormalizedShape,Shape> *area ) const;

        /**
         * Returns whether the regular area intersects with the given @p shape.
         */
        bool intersects( const NormalizedShape& shape ) const;

        /**
         * Appends the given @p area to the regular area.
         */
        void appendArea( const RegularArea<NormalizedShape,Shape> *area );

        /**
         * Appends the given @p shape to the regular area.
         */
        void appendShape( const NormalizedShape& shape, MergeSide side = MergeAll );

        /**
         * Simplifies the regular area by merging its intersecting subareas.
         */
        void simplify();

        /**
         * Returns whether the regular area is a null area.
         */
        bool isNull() const;

        /**
         * Returns the subareas of the regular areas as shapes for the given scaling factor
         * @p xScale and @p yScale, translated by @p dx and @p dy.
         */
        QList<Shape> geometry( int xScale, int yScale, int dx = 0, int dy = 0 ) const;

        /**
         * Transforms the regular area with the operations defined by @p matrix.
         */
        void transform( const QMatrix &matrix );
};

template <class NormalizedShape, class Shape>
RegularArea<NormalizedShape, Shape>::~RegularArea()
{
    int size = this->count();
    for ( int i = 0; i < size; ++i )
        doDelete( (*this)[i] );
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
                    if ( givePtr( (*this)[x] )->intersects( deref( (*this)[i+1] ) ) )
                    {
                        deref((*this)[x]) |= deref((*this)[i+1]);
                        NormalizedShape& tobedeleted = (*this)[i+1];
                        this->removeAt( i + 1 );
                        doDelete( tobedeleted );
                        --end;
                        --i;
                    }
                    else
                    {
                        x=i+1;
                   }
            }
#ifdef DEBUG_REGULARAREA
    kDebug() << "from" << prev_end << "to" << this->count();
#endif
}

template <class NormalizedShape, class Shape>
bool RegularArea<NormalizedShape, Shape>::isNull() const
{
    if ( !this )
        return false;

    if ( this->isEmpty() )
        return false;

    typename QList<NormalizedShape>::const_iterator it = this->begin(), itEnd = this->end();
    for ( ; it != itEnd; ++it )
        if ( !givePtr( *it )->isNull() )
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

    typename QList<NormalizedShape>::const_iterator it = this->begin(), itEnd = this->end();
    for ( ; it != itEnd; ++it )
        if ( !givePtr( *it )->isNull() && givePtr( *it )->intersects( rect ) )
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

    typename QList<NormalizedShape>::const_iterator it = this->begin(), itEnd = this->end();
    for ( ; it != itEnd; ++it )
    {
        typename QList<NormalizedShape>::const_iterator areaIt = area->begin(), areaItEnd = area->end();
        for ( ; areaIt != areaItEnd; ++areaIt )
        {
            if ( !( *it ).isNull() && ( *it ).intersects( *areaIt ) )
                return true;
        }
    }

    return false;
}

template <class NormalizedShape, class Shape>
void RegularArea<NormalizedShape, Shape>::appendArea( const RegularArea<NormalizedShape, Shape> *area )
{
    if ( !this )
        return;

    typename QList<NormalizedShape>::const_iterator areaIt = area->begin(), areaItEnd = area->end();
    for ( ; areaIt != areaItEnd; ++areaIt )
        this->append( *areaIt );
}


template <class NormalizedShape, class Shape>
void RegularArea<NormalizedShape, Shape>::appendShape( const NormalizedShape& shape, MergeSide side )
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
        bool intersection = false;
        NormalizedShape& last = (*this)[size - 1];
#define O_LAST givePtr( last )
#  define O_LAST_R O_LAST->right
#  define O_LAST_L O_LAST->left
#  define O_LAST_T O_LAST->top
#  define O_LAST_B O_LAST->bottom
#define O_NEW givePtr( shape )
#  define O_NEW_R O_NEW->right
#  define O_NEW_L O_NEW->left
#  define O_NEW_T O_NEW->top
#  define O_NEW_B O_NEW->bottom
        switch ( side )
        {
            case MergeRight:
                intersection = ( O_LAST_R >= O_NEW_L ) && ( O_LAST_L <= O_NEW_R )
                               && ( ( O_LAST_T <= O_NEW_T && O_LAST_B >= O_NEW_B )
                                  || ( O_LAST_T >= O_NEW_T && O_LAST_B <= O_NEW_B ) );
                break;
            case MergeBottom:
                intersection = ( O_LAST_B >= O_NEW_T ) && ( O_LAST_T <= O_NEW_B )
                               && ( ( O_LAST_R <= O_NEW_R && O_LAST_L >= O_NEW_L )
                                  || ( O_LAST_R >= O_NEW_R && O_LAST_L <= O_NEW_L ) );
                break;
            case MergeLeft:
                intersection = ( O_LAST_L <= O_NEW_R ) && ( O_LAST_R >= O_NEW_L )
                               && ( ( O_LAST_T <= O_NEW_T && O_LAST_B >= O_NEW_B )
                                  || ( O_LAST_T >= O_NEW_T && O_LAST_B <= O_NEW_B ) );
                break;
            case MergeTop:
                intersection = ( O_LAST_T <= O_NEW_B ) && ( O_LAST_B >= O_NEW_T )
                               && ( ( O_LAST_R <= O_NEW_R && O_LAST_L >= O_NEW_L )
                                  || ( O_LAST_R >= O_NEW_R && O_LAST_L <= O_NEW_L ) );
                break;
            case MergeAll:
                intersection = O_LAST->intersects( shape );
                break;
        }
#undef O_LAST
#  undef O_LAST_R
#  undef O_LAST_L
#  undef O_LAST_T
#  undef O_LAST_B
#undef O_NEW
#  undef O_NEW_R
#  undef O_NEW_L
#  undef O_NEW_T
#  undef O_NEW_B
        // if the new shape intersects with the last shape in the list, then
        // merge it with that and delete the shape
        if ( intersection )
        {
            deref((*this)[size - 1]) |= deref( shape );
            doDelete( const_cast<NormalizedShape&>( shape ) );
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

    typename QList<NormalizedShape>::const_iterator it = this->begin(), itEnd = this->end();
    for ( ; it != itEnd; ++it )
        if ( ( *it ).contains( x, y ) )
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

    return QList<NormalizedShape>::contains( shape );
}

template <class NormalizedShape, class Shape>
QList<Shape> RegularArea<NormalizedShape, Shape>::geometry( int xScale, int yScale, int dx, int dy ) const
{
    if ( !this || this->isEmpty() )
        return QList<Shape>();

    QList<Shape> ret;
    Shape t;
    typename QList<NormalizedShape>::const_iterator it = this->begin(), itEnd = this->end();
    for ( ; it != itEnd; ++it )
    {
        t = givePtr( *it )->geometry( xScale, yScale );
        t.translate( dx, dy );
        ret.append( t );
    }

    return ret;
}

template <class NormalizedShape, class Shape>
void RegularArea<NormalizedShape, Shape>::transform( const QMatrix &matrix )
{
    if ( !this )
        return;

    if ( this->isEmpty() )
        return;

    for ( int i = 0; i < this->count(); ++i )
        givePtr( (*this)[i] )->transform( matrix );
}

class OKULAR_EXPORT RegularAreaRect : public RegularArea< NormalizedRect, QRect >
{
    public:
        RegularAreaRect();
        RegularAreaRect( const RegularAreaRect& rar );
        ~RegularAreaRect();

        RegularAreaRect& operator=( const RegularAreaRect& rar );

    private:
        class Private;
        Private * const d;
};

/**
 * This class stores the coordinates of a highlighting area
 * together with the id of the highlight owner and the color.
 */
class HighlightAreaRect : public RegularAreaRect
{
    public:
        /**
         * Creates a new highlight area rect with the coordinates of
         * the given @p area.
         */
        HighlightAreaRect( const RegularAreaRect *area = 0 );

        /**
         * The search ID of the highlight owner.
         */
        int s_id;

        /**
         * The color of the highlight.
         */
        QColor color;
};

}

#ifndef QT_NO_DEBUG_STREAM
/**
 * Debug operator for normalized @p point.
 */
OKULAR_EXPORT QDebug operator<<( QDebug str, const Okular::NormalizedPoint &point );

/**
 * Debug operator for normalized @p rect.
 */
OKULAR_EXPORT QDebug operator<<( QDebug str, const Okular::NormalizedRect &rect );
#endif

#endif
