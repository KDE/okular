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

class QPixmap;
class QPainter;
class QImage;
//class QString;
//class QRect;

class TextOutputDev;
class PageOverlay;

/**
 * @short Collector for all the data belonging to a page.
 *
 * The Page class contains its pixmap, the thumbnail, a search page (a class
 * used internally for searching data) the modifiers descriptors (for overlay
 * graphics) and more.
 * It provides accessor methods for all those operations too.
 * 
 * Note: add stuff this class contains is destroyed automatically when the
 * class is destroyed.
 */

/*
[19:52] *TSDgeos* is, for links to work i need to enable setMouseTracking in the widget
[19:53] *TSDgeos* that generates a mousemoveevent even if the user does not click the mouse so i can change the cursor when the user is over a link
[19:53] *TSDgeos* do you think page could have a "cache" of places where the link exists so i don't have to query xpdf every time?
[19:57] *eros* I'll add a cache.
*/
class KPDFPage
{
public:
    KPDFPage( uint number, float width, float height, int rotation );
    ~KPDFPage();

    // query properties (const read-only methods)
    uint number() const { return m_number; }
    float width() const { return m_width; }
    float height() const { return m_height; }
    float ratio() const { return m_height / m_width; }
    float rotation() const { return m_rotate; }
    bool hasPixmap( int width, int height ) const;
    bool hasThumbnail( int width, int height ) const;
    bool hasLink( int mouseX, int mouseY ) const;
    void drawPixmap( QPainter * p, const QRect & rect, int width, int height ) const;
    void drawThumbnail( QPainter * p, const QRect & rect, int width, int height ) const;

    // page contents setup
    void setPixmap( const QImage & image );
    /*void setPixmapOverlaySelection( const QRect & normalizedRect );*/
    /*void setPixmapOverlayNotations( ..DOMdescription.. );*/
    void setThumbnail( const QImage & image );
    /*void setTextPage( TextOutputDev * );*/
    /*void setLinks( ..SomeStruct.. );    or (better): */
    /*void addLink( QFloatRect( normalizedRect ), int destPage, int destPos ); */

    // FIND command
    //bool hasText( QString & text );
    //const QRect & textPosition();

private:
    uint m_number;
    float m_width, m_height;
    int m_rotate;
    QPixmap * m_pixmap;
    QPixmap * m_thumbnail;
    TextOutputDev * m_text;
    PageOverlay * m_overlay;
};

#endif
