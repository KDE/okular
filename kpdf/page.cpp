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

// system includes
#include <string.h>

// local includes
#include "Link.h"
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
    QValueList< KPDFLink * >::iterator lIt = m_links.begin(), lEnd = m_links.end();
    for ( ; lIt != lEnd; ++lIt )
        delete *lIt;
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
    if ( m_links.count() < 1 )
        return false;
    QValueList< KPDFLink * >::const_iterator it = m_links.begin(), end = m_links.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( mouseX, mouseY ) )
            return true;
    return false;
}

// BEGIN commands (paint / search)
const KPDFLink * KPDFPage::getLink( int mouseX, int mouseY ) const
{
    QValueList< KPDFLink * >::const_iterator it = m_links.begin(), end = m_links.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( mouseX, mouseY ) )
            return *it;
    return 0;
}

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
                // setRasterOp is no more on Qt4 find an alternative way of doing this
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

void KPDFPage::setLinks( const QValueList<KPDFLink *> links )
{
    QValueList< KPDFLink * >::iterator it = m_links.begin(), end = m_links.end();
    for ( ; it != end; ++it )
        delete *it;
    m_links = links;
}

/*
void KPDFPage::setPixmapOverlayNotations( ..DOMdescription.. )
{   //TODO this
}
*/



KPDFLink::KPDFLink( int l, int t, int r, int b )
    : m_type( Unknown ), m_dest( 0 ), m_destNamed( 0 ),
    m_fileName( 0 ), m_parameters( 0 ), m_uri( 0 )
{
    // assign coordinates swapping them if negative width or height
    x_min = r > l ? l : r;
    x_max = r > l ? r : l;
    y_min = b > t ? t : b;
    y_max = b > t ? b : t;
}

KPDFLink::~KPDFLink()
{
    delete m_dest;
    delete [] m_destNamed;
    delete [] m_fileName;
    delete [] m_parameters;
    delete [] m_uri;
}

void KPDFLink::setLinkGoto( LinkDest * d, const char * n, const char * file )
{
    m_type = Goto;
    delete m_dest;
    m_dest = d;
    copyString( m_destNamed, n );
    copyString( m_fileName, file );
}

void KPDFLink::setLinkExecute( const char * file, const char * par )
{
    m_type = Execute;
    copyString( m_fileName, file );
    copyString( m_parameters, par );
}

void KPDFLink::setLinkNamed( const char * name )
{
    m_type = Named;
    copyString( m_uri, name );
}

void KPDFLink::setLinkURI( const char * uri )
{
    m_type = URI;
    copyString( m_uri, uri );
}

void KPDFLink::setLinkMovie( int ref_num, int ref_gen, const char * title )
{
    m_type = Movie;
    m_refNum = ref_num;
    m_refGen = ref_gen;
    copyString( m_uri, title );
}


const LinkDest * KPDFLink::getDest() const
{
    return m_dest;
}

const char * KPDFLink::getNamedDest() const
{
    return m_destNamed;
}

const char * KPDFLink::getFileName() const
{
    return m_fileName;
}

const char * KPDFLink::getParameters() const
{
    return m_parameters;
}

const char * KPDFLink::getName() const
{
    return m_uri;
}

const char * KPDFLink::getURI() const
{
    return m_uri;
}


KPDFLink::LinkType KPDFLink::type() const
{
    return m_type;
}

bool KPDFLink::contains( int x, int  y ) const
{
    return (x > x_min) && (x < x_max) && (y > y_min) && (y < y_max);
}

void KPDFLink::copyString( char * &dest, const char * src ) const
{
    if ( dest )
        delete [] dest;
    dest = 0;
    if ( src )
    {
        dest = new char[ strlen(src) + 1 ];
        strcpy( &dest[0], src );
    }
}
