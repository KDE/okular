/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qpixmap.h>
#include <qstring.h>
#include <qmap.h>
#include <kdebug.h>

// local includes
#include "page.h"
#include "pagetransition.h"
#include "link.h"
#include "conf/settings.h"
#include "xpdf/TextOutputDev.h"


/** class KPDFPage **/

KPDFPage::KPDFPage( uint page, float w, float h, int r )
    : m_number( page ), m_rotation( r ), m_width( w ), m_height( h ),
    m_bookmarked( false ), m_text( 0 ), m_transition( 0 )
{
    // if landscape swap width <-> height (rotate 90deg CCW)
    if ( r == 90 || r == 270 )
    {
        m_width = h;
        m_height = w;
    }
    // avoid Division-By-Zero problems in the program
    if ( m_width <= 0 )
        m_width = 1;
    if ( m_height <= 0 )
        m_height = 1;
}

KPDFPage::~KPDFPage()
{
    deletePixmapsAndRects();
    deleteHighlights();
    delete m_text;
    delete m_transition;
}


bool KPDFPage::hasHighlights( int id ) const
{
    // simple case: have no highlights
    if ( m_highlights.isEmpty() )
        return false;
    // simple case: we have highlights and no id to match
    if ( id == -1 )
        return true;
    // iterate on the highlights list to find an entry by id
    QValueList< HighlightRect * >::const_iterator it = m_highlights.begin(), end = m_highlights.end();
    for ( ; it != end; ++it )
        if ( (*it)->id == id )
            return true;
    return false;
}

bool KPDFPage::hasPixmap( int id, int width, int height ) const
{
    if ( !m_pixmaps.contains( id ) )
        return false;
    if ( width == -1 || height == -1 )
        return true;
    QPixmap * p = m_pixmaps[ id ];
    return p ? ( p->width() == width && p->height() == height ) : false;
}

bool KPDFPage::hasSearchPage() const
{
    return m_text != 0;
}

bool KPDFPage::hasRect( int mouseX, int mouseY ) const
{
    if ( m_rects.count() < 1 )
        return false;
    QValueList< KPDFPageRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( mouseX, mouseY ) )
            return true;
    return false;
}

bool KPDFPage::hasLink( int mouseX, int mouseY ) const
{
    const KPDFPageRect *r;
    r = getRect( mouseX, mouseY);
    return r && r->pointerType() == KPDFPageRect::Link;
}

const KPDFPageRect * KPDFPage::getRect( int mouseX, int mouseY ) const
{
    QValueList< KPDFPageRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( mouseX, mouseY ) )
            return *it;
    return 0;
}

const KPDFPageTransition * KPDFPage::getTransition() const
{
    return m_transition;
}

const QString KPDFPage::getTextInRect( const QRect & rect, double zoom ) const
{
    if ( !m_text )
        return QString::null;
    int left = (int)((double)rect.left() / zoom),
        top = (int)((double)rect.top() / zoom),
        right = (int)((double)rect.right() / zoom),
        bottom = (int)((double)rect.bottom() / zoom);
    GString * text = m_text->getText( left, top, right, bottom );
    QString result = QString::fromUtf8( text->getCString() );
    delete text;
    return result; 
}


HighlightRect * KPDFPage::searchText( const QString & text, bool strictCase, HighlightRect * lastRect )
{
    if ( text.isEmpty() )
        return 0;

    // create a xpf's Unicode (unsigned int) array for the given text
    const QChar * str = text.unicode();
    int len = text.length();
    Unicode u[ len ];
    for (int i = 0; i < len; ++i)
        u[i] = str[i].unicode();

    // find out the direction of search
    enum SearchDir { FromTop, NextMatch, PrevMatch } dir = lastRect ? NextMatch : FromTop;
    double sLeft, sTop, sRight, sBottom;
    if ( dir == NextMatch )
    {
        sLeft = lastRect->left * m_width;
        sTop = lastRect->top * m_height;
        sRight = lastRect->right * m_width;
        sBottom = lastRect->bottom * m_height;
    }

    // this loop is only for 'bad case' matches
    bool found = false;
    while ( !found )
    {
        if ( dir == FromTop )
            found = m_text->findText( u, len, gTrue, gTrue, gFalse, gFalse, &sLeft, &sTop, &sRight, &sBottom );
        else if ( dir == NextMatch )
            found = m_text->findText( u, len, gFalse, gTrue, gTrue, gFalse, &sLeft, &sTop, &sRight, &sBottom );
        else if ( dir == PrevMatch )
            // FIXME: this doesn't work as expected (luckily backward search isn't yet used)
            found = m_text->findText( u, len, gTrue, gFalse, gFalse, gTrue, &sLeft, &sTop, &sRight, &sBottom );

        // if not found (even in case unsensitive search), terminate
        if ( !found )
            break;

        // check for case sensitivity
        if ( strictCase )
        {
            // since we're in 'Case sensitive' mode, check if words are identical
            GString * realText = m_text->getText( sLeft, sTop, sRight, sBottom );
            found = QString::fromUtf8( realText->getCString() ) == text;
            if ( !found && dir == FromTop )
                dir = NextMatch;
            delete realText;
        }
    }

    // if the page was found, return a new normalized HighlightRect
    if ( found )
        return new HighlightRect( sLeft / m_width, sTop / m_height, sRight / m_width, sBottom / m_height );
    return 0;
}

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

void KPDFPage::setRects( const QValueList< KPDFPageRect * > rects )
{
    QValueList< KPDFPageRect * >::iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        delete *it;
    m_rects = rects;
}

void KPDFPage::setHighlight( HighlightRect * hr, bool add )
{
    if ( !add )
        deleteHighlights( hr->id );
    m_highlights.append( hr );
}

void KPDFPage::setTransition( const KPDFPageTransition * transition )
{
    delete m_transition;
    m_transition = transition;
}

void KPDFPage::deletePixmap( int id )
{
    if ( m_pixmaps.contains( id ) )
    {
        delete m_pixmaps[ id ];
        m_pixmaps.remove( id );
    }
}

void KPDFPage::deletePixmapsAndRects()
{
    // delete all stored pixmaps
    QMap<int,QPixmap *>::iterator it = m_pixmaps.begin(), end = m_pixmaps.end();
    for ( ; it != end; ++it )
        delete *it;
    m_pixmaps.clear();
    // delete PageRects
    QValueList< KPDFPageRect * >::iterator rIt = m_rects.begin(), rEnd = m_rects.end();
    for ( ; rIt != rEnd; ++rIt )
        delete *rIt;
    m_rects.clear();
}

void KPDFPage::deleteHighlights( int id )
{
    // delete highlights by ID
    QValueList< HighlightRect * >::iterator it = m_highlights.begin(), end = m_highlights.end();
    while ( it != end )
    {
        HighlightRect * highlight = *it;
        if ( id == -1 || highlight->id == id )
        {
            it = m_highlights.remove( it );
            delete highlight;
        }
        else
            ++it;
    }
}


/** class KPDFPageRect **/

KPDFPageRect::KPDFPageRect( int l, int t, int r, int b )
    : m_pointerType( NoPointer ), m_pointer( 0 )
{
    // assign coordinates swapping them if negative width or height
    m_xMin = r > l ? l : r;
    m_xMax = r > l ? r : l;
    m_yMin = b > t ? t : b;
    m_yMax = b > t ? b : t;
}

KPDFPageRect::~KPDFPageRect()
{
    deletePointer();
}

bool KPDFPageRect::contains( int x, int y ) const
{
    return (x > m_xMin) && (x < m_xMax) && (y > m_yMin) && (y < m_yMax);
}

QRect KPDFPageRect::geometry() const
{
    return QRect( m_xMin, m_yMin, m_xMax - m_xMin, m_yMax - m_yMin );
}

void KPDFPageRect::setPointer( void * object, enum PointerType pType )
{
    deletePointer();
    m_pointer = object;
    m_pointerType = pType;
}

KPDFPageRect::PointerType KPDFPageRect::pointerType() const
{
    return m_pointerType;
}

const void * KPDFPageRect::pointer() const
{
    return m_pointer;
}

void KPDFPageRect::deletePointer()
{
    if ( !m_pointer )
        return;

    if ( m_pointerType == Link )
        delete static_cast<KPDFLink*>( m_pointer );
    else
        kdDebug() << "Object deletion not implemented for type '"
                  << m_pointerType << "' ." << endl;
}


/** class HighlightRect **/

HighlightRect::HighlightRect()
    : id( -1 ), left( 0.0 ), top( 0.0 ), right( 0.0 ), bottom( 0.0 )
{
}

HighlightRect::HighlightRect( double l, double t, double r, double b )
    : id( -1 ), left( l ), top( t ), right( r ), bottom( b )
{
}
