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

class QPainter;
class QPixmap;
class QRect;
class TextPage;
class KPDFPageRect;
class KPDFPageTransition;

/**
 * @short Collector for all the data belonging to a page.
 *
 * The KPDFPage class contains pixmaps (referenced using observers id as key),
 * a search page (a class used internally for retrieving text), rect classes
 * (that describe links or other active areas in the current page) and more.
 *
 * Note: All objects are reparented to this class.
 */
class KPDFPage
{
    public:
        KPDFPage( uint number, float width, float height, int rotation );
        ~KPDFPage();

        enum KPDFPageAttributes { Highlight = 1, Bookmark = 2 };

        // query properties (const read-only methods)
        inline int number() const { return m_number; }
        inline int rotation() const { return m_rotation; }
        inline int attributes() const { return m_attributes; }
        inline float width() const { return m_width; }
        inline float height() const { return m_height; }
        inline float ratio() const { return m_height / m_width; }

        bool hasPixmap( int id, int width = -1, int height = -1 ) const;
        bool hasSearchPage() const;
        bool hasRect( int mouseX, int mouseY ) const;
        bool hasLink( int mouseX, int mouseY ) const;
        const KPDFPageRect * getRect( int mouseX, int mouseY ) const;
        const KPDFPageTransition * getTransition() const;
        const QPoint getLastSearchCenter() const;
        const QString getTextInRect( const QRect & rect, double zoom /*= 1.0*/ ) const;

        // operations (by KPDFDocument)
        inline void setAttribute( int att ) { m_attributes |= att; }
        inline void clearAttribute( int att ) { m_attributes &= ~att; }
        inline void toggleAttribute( int att ) { m_attributes ^= att; }
        bool hasText( const QString & text, bool strictCase, bool fromTop );

        // set/delete contents (by KPDFDocument)
        void setPixmap( int id, QPixmap * pixmap );
        void setSearchPage( TextPage * text );
        void setRects( const QValueList< KPDFPageRect * > rects );
        void setTransition( const KPDFPageTransition * transition );
        void deletePixmap( int id );
        void deletePixmapsAndRects();

    private:
        friend class PagePainter;
        int m_number, m_rotation, m_attributes;
        float m_width, m_height;
        double m_sLeft, m_sTop, m_sRight, m_sBottom;

        QMap< int, QPixmap * > m_pixmaps;
        TextPage * m_text;
        QValueList< KPDFPageRect * > m_rects;
        const KPDFPageTransition * m_transition;
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
 *
 */
class KPDFPageRect
{
    public:
        KPDFPageRect( int left, int top, int right, int bottom );
        ~KPDFPageRect();

        // query geometric properties
        bool contains( int x, int y ) const;
        QRect geometry() const;

        // set a pointer to data associated to this rect
        enum PointerType { NoPointer, Link, Image };
        void setPointer( void * object, enum PointerType pType );

        // query type and get a const pointer to the stored object
        PointerType pointerType() const;
        const void * pointer() const;

    private:
        void deletePointer();
        int m_xMin, m_xMax, m_yMin, m_yMax;
        PointerType m_pointerType;
        void * m_pointer;
};



/**
 * @short Paints a KPDFPage to an open painter using given flags.
 * TODO: move definition and implementation to pagepainter.* (..maybe..)
 */
class PagePainter
{
    public:
        // list of flags passed to the painting function. by OR-ing those flags
        // you can decide wether or not to permit drawing of a certain feature.
        enum PagePainterFlags { Accessibility = 1, EnhanceLinks = 2,
                                EnhanceImages = 4, Highlight = 8 };

        // draw (using painter 'p') the 'page' requested by 'id' using features
        // in 'flags'. 'limits' is the bounding rect of the paint operation,
        // 'width' and 'height' the expected size of page contents (used only
        // to pick up an alternative pixmap if the pixmap of 'id' is missing.
        static void paintPageOnPainter( const KPDFPage * page, int id, int flags,
            QPainter * p, const QRect & limits, int width = -1, int height = -1 );
};

#endif
