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
class KPDFPageRect;
class KPDFPageTransition;
class NormalizedRect;
class HighlightRect;

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
        bool hasRect( double x, double y ) const;
        bool hasHighlights( int s_id = -1 ) const;
        bool hasTransition() const;

        NormalizedRect * findText( const QString & text, bool keepCase, NormalizedRect * last = 0 ) const;
        const QString getText( const NormalizedRect & rect ) const;
        const KPDFPageRect * getRect( double x, double y ) const;
        const KPDFPageTransition * getTransition() const;

        // oprations: set/delete contents (by KPDFDocument)
        void setPixmap( int p_id, QPixmap * pixmap );
        void setSearchPage( TextPage * text );
        void setBookmark( bool state );
        void setRects( const QValueList< KPDFPageRect * > rects );
        void setHighlight( int s_id, NormalizedRect * &r, const QColor & color );
        void setTransition( const KPDFPageTransition * transition );
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
        QValueList< KPDFPageRect * > m_rects;
        QValueList< HighlightRect * > m_highlights;
        const KPDFPageTransition * m_transition;
};


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
};

/**
 * @short A rect on the page that may contain an object.
 *
 * This class describes a rect (geometrical coordinates) and may hold a
 * pointer to an associated object. An object is reparented to this class
 * and deleted when this class is deleted.
 *
 * Objects are stored and read as 'void pointers' so you have to perform
 * the cast on the code that handles the object using information provided
 * by pointerType().
 *
 * Type / Class correspondency tab:
 *  - NoPointer : ''              : no object is stored
 *  - Link      : class KPDFLink  : description of a link
 *  - Image     : class KPDFImage : description of an image
 */
class KPDFPageRect : public NormalizedRect
{
    public:
        KPDFPageRect( double left, double top, double right, double bottom );
        ~KPDFPageRect();

        // expand NormalizedRect to given width and height
        QRect geometry( int width, int height ) const;

        // set a pointer to data associated to this rect
        enum PointerType { NoPointer, Link, Image };
        void setPointer( void * object, enum PointerType pType );

        // query type and get a const pointer to the stored object
        PointerType pointerType() const;
        const void * pointer() const;

    private:
        void deletePointer();
        PointerType m_pointerType;
        void * m_pointer;
};

/**
 * Internal Storage: normalized colored highlight owned by id
 */
struct HighlightRect : public NormalizedRect
{
    // more publicly accessed fields
    int s_id;
    QColor color;
};

#endif
