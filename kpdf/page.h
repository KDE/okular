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

class QPainter;
class QPixmap;
//class QString;
//class QRect;
class TextPage;

/**
 * @short Collector for all the data belonging to a page.
 *
 * The KPDFPage class contains pixmaps (referenced using obsedvers id as key),
 * a search page (a class used internally for searching data), link classes
 * (that describe links in the current page) plus graphics overlays and more.
 *
 * Note: All objects passed to this class will be destoryed on class deletion.
 */

// ### HACK : this structure is under big changes ###
class KPDFPage
{
public:
    KPDFPage( int number, float width, float height, int rotation );
    ~KPDFPage();

    // query properties (const read-only methods)
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

    // commands
    void drawPixmap( int id, QPainter * p, const QRect & rect, int width, int height ) const;
    bool hasText( const QString & text, bool strictCase, bool fromTop );
    void hilightLastSearch( bool enabled );
    void bookmark( bool enabled );

    // set page contents
    void setPixmap( int id, QPixmap * pixmap );
    void setSearchPage( TextPage * text );
    /*void setLinks( ..SomeStruct.. );    or (better): */
    /*void setPixmapOverlayNotations( ..DOMdescription.. );*/

private:
    int m_number, m_rotation;
    float m_width, m_height;
    bool m_hilighting, m_bookmarking;
    double m_sLeft, m_sTop, m_sRight, m_sBottom;

    QMap<int,QPixmap *> m_pixmaps;
    TextPage * m_text;
};
/*
class KPDFLink
{
public:
	enum LinkType { Goto, Execute, Action, URI, Movie };

	KPDFLink( LinkType type ) : m_type( type ) {};

	void setType( LinkType type ) { m_type = type; }
	LinkType type() { return m_type; }

private:
	LinkType m_type;
	float x_min, x_max, y_min, y_max;
	// [Goto] type

	// []
};
*/
#endif
