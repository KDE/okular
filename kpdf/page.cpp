/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt includes
#include <qapplication.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qpainter.h>
#include <qmap.h>

// local includes
#include "TextOutputDev.h"
#include "page.h"

// TODO add painting effects (plus selection rectangle)
// TODO think about moving rendering ...

KPDFPage::KPDFPage( int page, float w, float h, int r )
    : m_number( page ), m_rotation( r ), m_width( w ), m_height( h ),
    m_hilighting( false ), m_bookmarking( false ), m_sLeft( 0 ),
    m_sTop( 0 ), m_sRight( 0 ), m_sBottom( 0 ), m_text( 0 )
{
}

KPDFPage::~KPDFPage()
{
    QMap<int,QPixmap *>::iterator it = m_pixmaps.begin(), end = m_pixmaps.end();
    for ( ; it != end; ++it )
        delete *it;
    delete m_text;
}


bool KPDFPage::hasPixmap( int id, int width, int height ) const
{
    if ( !m_pixmaps.contains( id ) )
        return false;
    QPixmap * p = m_pixmaps[ id ];
    return p ? ( p->width() == width && p->height() == height ) : false;
}

bool KPDFPage::hasSearchPage() const
{
    return (m_text != 0);
}

bool KPDFPage::hasLink( int mouseX, int mouseY ) const
{
    //TODO this.
    //Sample implementation using a small rect as 'active' link zone
    return QRect( 20,20, 100,50 ).contains( mouseX, mouseY );
}

// BEGIN commands (paint / search)
void KPDFPage::drawPixmap( int id, QPainter * p, const QRect & limits, int width, int height ) const
{
    QPixmap * pixmap = 0;

    // if a pixmap is present for given id, use it
    if ( m_pixmaps.contains( id ) )
        pixmap = m_pixmaps[ id ];

    // else find the closest match using pixmaps of other IDs (great optim!)
    else if ( !m_pixmaps.isEmpty() )
    {
        int minDistance = -1;
        QMap<int,QPixmap *>::const_iterator it = m_pixmaps.begin(), end = m_pixmaps.end();
        for ( ; it != end; ++it )
        {
            int pixWidth = (*it)->width(),
                distance = pixWidth > width ? pixWidth - width : width - pixWidth;
            if ( minDistance == -1 || distance < minDistance )
            {
                pixmap = *it;
                minDistance = distance;
            }
        }
    }

    // if we found a pixmap draw it
    if ( pixmap )
    {
        // fast blit the pixmap if it has the right size..
        if ( pixmap->width() == width && pixmap->height() == height )
            p->drawPixmap( limits.topLeft(), *pixmap, limits );
        // ..else set a scale matrix to the painter and paint a quick 'zoomed' pixmap
        else
        {
            p->save();
            p->scale( width / (double)pixmap->width(), height / (double)pixmap->height() );
            p->drawPixmap( 0,0, *pixmap, 0,0, pixmap->width(), pixmap->height() );
            p->restore();
            // draw a red cross (to hilight that the pixmap has not the right size)
            p->setPen( Qt::red );
            p->drawLine( 0, 0, width-1, height-1 );
            p->drawLine( 0, height-1, width-1, 0 );
        }
        // draw selection
        if ( m_hilighting )
        {
            int x = (int)( m_sLeft * width / m_width ),
                y = (int)( m_sTop * height / m_height ),
                w = (int)( m_sRight * width / m_width ) - x,
                h = (int)( m_sBottom * height / m_height ) - y;
            if ( w > 0 && h > 0 )
            {
                p->setBrush( Qt::SolidPattern );
                p->setPen( QPen( Qt::black, 1 ) ); // should not be necessary bug a Qt bug makes it necessary
                p->setRasterOp( Qt::NotROP );
                p->drawRect( x, y, w, h );
            }
        }
    }
    // else draw a blank area
    else
        p->fillRect( limits, Qt::white /*FIXME change to the page bg color*/ );
}

bool KPDFPage::hasText( const QString & text, bool strictCase, bool fromTop )
{
    if ( !m_text )
        return false;

    const char * str = text.latin1();
    int len = text.length();
    Unicode *u = (Unicode *)gmalloc(len * sizeof(Unicode));
    for (int i = 0; i < len; ++i)
        u[i] = (Unicode) str[i];

    bool found = m_text->findText( u, len, fromTop ? gTrue : gFalse, gTrue, fromTop ? gFalse : gTrue, gFalse, &m_sLeft, &m_sTop, &m_sRight, &m_sBottom );
    if( found && strictCase )
    {
        GString * orig = m_text->getText( m_sLeft, m_sTop, m_sRight, m_sBottom );
        found = !strcmp( text.latin1(), orig->getCString() );
    }
    return found;
}

void KPDFPage::hilightLastSearch( bool on )
{
    m_hilighting = on;
    //if ( !on ) -> invalidate search rect?
}

void KPDFPage::bookmark( bool on )
{
    m_bookmarking = on;
}
// END commands (paint / search)


void KPDFPage::setPixmap( int id, QPixmap * pixmap )
{
    if ( m_pixmaps.contains( id ) )
        delete m_pixmaps[id];
    m_pixmaps[id] = pixmap;
}

void KPDFPage::setSearchPage( TextPage * tp )
{
    delete m_text;
    m_text = tp;
}

/*
void KPDFPage::setLinks( ..SomeStruct.. )
{   //TODO this
}

void KPDFPage::setPixmapOverlayNotations( ..DOMdescription.. )
{   //TODO this
}
*/
