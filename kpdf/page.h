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
class LinkDest;
class KPDFLink;

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
    KPDFPage( int number, float width, float height, int rotation );
    ~KPDFPage();

    // query properties and draw (const read-only methods)
    uint number() const { return m_number; }
    float width() const { return m_width; }
    float height() const { return m_height; }
    float ratio() const { return m_height / m_width; }
    float rotation() const { return m_rotation; }
    bool isHilighted() const { return m_hilighting; }
    bool isBookmarked() const { return m_bookmarking; }
    bool hasPixmap( int id, int width, int height ) const;
    bool hasSearchPage() const;
    bool hasLink( int mouseX, int mouseY ) const;
    void drawPixmap( int id, QPainter * p, const QRect & rect, int width, int height ) const;

    // commands (not const methods caled by KPDFDocument)
    bool hasText( const QString & text, bool strictCase, bool fromTop );
    void hilightLastSearch( bool enabled );
    void bookmark( bool enabled );

    // set page contents (not const methods caled by KPDFDocument)
    void setPixmap( int id, QPixmap * pixmap );
    void setSearchPage( TextPage * text );
    void setLinks( const QValueList<KPDFLink *> links );
    /*void setPixmapOverlayNotations( ..DOMdescription.. );*/

private:
    int m_number, m_rotation;
    float m_width, m_height;
    bool m_hilighting, m_bookmarking;
    double m_sLeft, m_sTop, m_sRight, m_sBottom;

    QMap< int, QPixmap * > m_pixmaps;
    TextPage * m_text;
    QValueList< KPDFLink * > m_links;
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
    KPDFLink( int left, int top, int right, int bottom );
    ~KPDFLink();

    // action types setup
    enum LinkType { Goto, Execute, Action, URI, Movie, Unknown };
    void setLinkGoto( LinkDest * dest, const char * namedDest, const char * fileName = 0 );
    void setLinkExecute( const char * file, const char * parameters );
    void setLinkNamed( const char * name );
    void setLinkURI( const char * uri );
    void setLinkMovie( int ref_num, int ref_gen, const char * title );

    // query
    LinkType type() const;
    bool contains( int x, int y ) const;

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

    void copyString( char * dest, const char * src );
};

#endif
