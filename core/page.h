/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_PAGE_H_
#define _KPDF_PAGE_H_

#include <qmap.h>
#include <qvaluelist.h>

class QPixmap;
class QRect;
class TextPage;
class KPDFPageTransition;
class HighlightRect;
class Annotation;

/**
 * @short A rect in normalized [0,1] coordinates.
 */
class NormalizedRect
{
    public:
        double left, top, right, bottom;

        NormalizedRect();
        NormalizedRect( double l, double t, double r, double b );
        NormalizedRect( const QRect & r, double xScale, double yScale );

        bool contains( double x, double y ) const;
        bool intersects( const NormalizedRect & normRect ) const;
        bool intersects( double l, double t, double r, double b ) const;

        QRect geometry( int xScale, int yScale ) const;
};

/**
 * @short NormalizedRect that contains a reference to an object.
 *
 * These rects contains a pointer to a kpdf object (such as a link or something
 * like that). The pointer is read and stored as 'void pointer' so cast is
 * performed by accessors based on the value returned by objectType(). Objects
 * are reparented to this class.
 *
 * Type / Class correspondency tab:
 *  - Link      : class KPDFLink  : description of a link
 *  - Image     : class KPDFImage : description of an image (n/a)
 */
class ObjectRect : public NormalizedRect
{
    public:
        // definition of the types of storable objects
        enum ObjectType { Link, Image };

        // default constructor: initialize all parameters
        ObjectRect( double l, double t, double r, double b, ObjectType typ, void * obj );
        ~ObjectRect();

        // query type and get a const pointer to the stored object
        inline ObjectType objectType() const { return m_objectType; }
        inline const void * pointer() const { return m_pointer; }

    private:
        ObjectType m_objectType;
        void * m_pointer;
};

/**
 * @short Collector for all the data belonging to a page.
 *
 * The KPDFPage class contains pixmaps (referenced using observers id as key),
 * a search page (a class used internally for retrieving text), rect classes
 * (that describe links or other active areas in the current page) and more.
 *
 * All coordinates are normalized to the page, so {x,y} are valid in [0,1]
 * range as long as NormalizedRect components.
 *
 * Note: The class takes ownership of all objects.
 */
class KPDFPage
{
    public:
        KPDFPage( uint number, float width, float height, int rotation );
        ~KPDFPage();

        // query properties (const read-only methods)
        inline int number() const { return m_number; }
        inline int rotation() const { return m_rotation; }
        inline float width() const { return m_width; }
        inline float height() const { return m_height; }
        inline float ratio() const { return m_height / m_width; }
        bool hasPixmap( int p_id, int width = -1, int height = -1 ) const;
        bool hasSearchPage() const;
        bool hasBookmark() const;
        bool hasObjectRect( double x, double y ) const;
        bool hasHighlights( int s_id = -1 ) const;
        //bool hasAnnotation( double x, double y ) const;
        bool hasTransition() const;

        NormalizedRect * findText( const QString & text, bool keepCase, NormalizedRect * last = 0 ) const;
        const QString getText( const NormalizedRect & rect ) const;
        const ObjectRect * hasObject( ObjectRect::ObjectType type, double x, double y ) const;
        //const Annotation * getAnnotation( double x, double y ) const;
        const KPDFPageTransition * getTransition() const;

        // operations: set/delete contents (by KPDFDocument)
        void setPixmap( int p_id, QPixmap * pixmap );
        void setSearchPage( TextPage * text );
        void setBookmark( bool state );
        void setObjectRects( const QValueList< ObjectRect * > rects );
        void setHighlight( int s_id, NormalizedRect * &r, const QColor & color );
        //void setAnnotation( Annotation * annotation );
        void setTransition( KPDFPageTransition * transition );
        void deletePixmap( int p_id );
        void deletePixmapsAndRects();
        void deleteHighlights( int s_id = -1 );

    private:
        friend class PagePainter;
        int m_number, m_rotation;
        float m_width, m_height;
        bool m_bookmarked;

        QMap< int, QPixmap * > m_pixmaps;
        TextPage * m_text;
        QValueList< ObjectRect * > m_rects;
        QValueList< HighlightRect * > m_highlights;
        //QValueList< Annotation * > m_annotations;
        KPDFPageTransition * m_transition;
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

#endif
