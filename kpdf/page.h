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
class TextPage;
class LinkAction;
class LinkDest;
class KPDFLink;
class KPDFActiveRect;

/**
 * @short Collector for all the data belonging to a page.
 *
 * The KPDFPage class contains pixmaps (referenced using obsedvers id as key),
 * a search page (a class used internally for searching data), link classes
 * (that describe links in the current page) plus graphics overlays and more.
 *
 * Note: All objects passed to this class will be destoryed on class deletion.
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

        bool hasPixmap( int id, int width, int height ) const;
        bool hasSearchPage() const { return m_text != 0; }
        bool hasLink( int mouseX, int mouseY ) const;
        bool hasActiveRect( int mouseX, int mouseY ) const;
        const QString getTextInRect( const QRect & rect, double zoom = 1.0 ) const;
        const KPDFLink * getLink( int mouseX, int mouseY ) const;

        // operations (not const methods caled by KPDFDocument)
        inline void setAttribute( int att ) { m_attributes |= att; }
        inline void clearAttribute( int att ) { m_attributes &= ~att; }
        inline void toggleAttribute( int att ) { m_attributes ^= att; }
        bool hasText( const QString & text, bool strictCase, bool fromTop );

        // set contents (not const methods caled by KPDFDocument)
        void setPixmap( int id, QPixmap * pixmap );
        void setSearchPage( TextPage * text );
        void setLinks( const QValueList<KPDFLink *> links );
        void setActiveRects( const QValueList<KPDFActiveRect *> rects );

    private:
        friend class PagePainter;
        int m_number, m_rotation, m_attributes;
        float m_width, m_height;
        double m_sLeft, m_sTop, m_sRight, m_sBottom;

        QMap< int, QPixmap * > m_pixmaps;
        TextPage * m_text;
        QValueList< KPDFLink * > m_links;
        QValueList< KPDFActiveRect * > m_rects;
};


class PagePainter
{
    public:
        static void paintPageOnPainter( const KPDFPage * page, int id,
            QPainter * p, const QRect & limits, int width = -1, int height = -1 );
};

/**
 * @short Encapsulates data that describes a link.
 *
 * There are many types of PDF links, here we provide accessors to set the
 * link to be of the given type. Other functions are for asking if a point
 * is inside the link rect (in displayed page coordinates).
 * KPDFLinks are created by the KPDFOutputDevice then stored and deleted
 * inside the referring KPDFPage.
 * Note: this structure is similar to XPDF LinkAction and its hieracy, but
 * is needed for storing data inside pages, since XPDF's PDFDoc deletes
 * Links objects when changing page (and we need persistant storage).
 */
class KPDFLink
{
public:
    KPDFLink( LinkAction * PDFAction );
    ~KPDFLink();

    // set geometry (only links collected KPDFPage(s))
    void setGeometry( int left, int top, int right, int bottom );

    // query / others
    bool contains( int x, int y ) const;
    void copyString( char * &dest, const char * src ) const;

    // action queries
    enum LinkType { Goto, Execute, URI, Named, Movie, Unknown };
    LinkType type() const;
    const LinkDest * getDest() const;   //1
    const char * getNamedDest() const;  //1
    const char * getFileName() const;   //1,2
    const char * getParameters() const; //2
    const char * getName() const;       //3
    const char * getURI() const;        //4

private:
    // general
    LinkType m_type;
    float x_min, x_max, y_min, y_max;

    // actions related
    LinkDest * m_dest;
    char * m_destNamed;
    char * m_fileName;
    char * m_parameters;
    char * m_uri;
    int m_refNum, m_refGen;
};


/**
 * @short Describes an 'active' rectange on the page.
 *
 * ...
 */
class KPDFActiveRect
{
public:
    KPDFActiveRect(int left, int top, int width, int height);

    // query
    bool contains(int x, int y);

private:
    int m_left, m_top, m_right, m_bottom;
};

#endif
