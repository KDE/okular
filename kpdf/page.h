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
class TextOutputDev;

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
    bool hasPixmap( int id, int width, int height ) const;
    bool hasLink( int mouseX, int mouseY ) const;
    void drawPixmap( int id, QPainter * p, const QRect & rect, int width, int height ) const;

    // page contents setup *NOTE changes in progress*
    void setPixmap( int id, QPixmap * pixmap );
    /*void setTextPage( TextOutputDev * );*/
    /*void setLinks( ..SomeStruct.. );    or (better): */
    /*void setPixmapOverlaySelection( const QRect & normalizedRect );*/
    /*void setPixmapOverlayNotations( ..DOMdescription.. );*/

    // FIND command
    //bool hasText( QString & text );
    //const QRect & textPosition();

private:
    int m_number, m_rotation;
    float m_width, m_height;

    QMap<int,QPixmap *> m_pixmaps;
    TextOutputDev * m_text;
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
