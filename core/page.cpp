/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qapplication.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qstring.h>
#include <qpainter.h>
#include <qmap.h>
#include <kimageeffect.h>
#include <kdebug.h>

// system includes
#include <string.h>

// local includes
#include "page.h"
#include "link.h"
#include "conf/settings.h"
#include "xpdf/TextOutputDev.h"


/** class KPDFPage **/

KPDFPage::KPDFPage( uint page, float w, float h, int r )
    : m_number( page ), m_rotation( r ), m_attributes( 0 ),
    m_width( w ), m_height( h ), m_sLeft( 0 ), m_sTop( 0 ),
    m_sRight( 0 ), m_sBottom( 0 ), m_text( 0 )
{
    // if landscape swap width <-> height (rotate 90deg CCW)
    if ( r == 90 || r == 270 )
    {
        m_width = h;
        m_height = w;
    }
}

KPDFPage::~KPDFPage()
{
    deletePixmapsAndRects();
    delete m_text;
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

const QString KPDFPage::getTextInRect( const QRect & rect, double zoom ) const
{
    if ( !m_text )
        return QString::null;
    int left = (int)((double)rect.left() / zoom),
        top = (int)((double)rect.top() / zoom),
        right = (int)((double)rect.right() / zoom),
        bottom = (int)((double)rect.bottom() / zoom);
    GString * text = m_text->getText( left, top, right, bottom );
    return QString( text->getCString() );
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
    gfree(u);
    return found;
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


/** class PagePainter **/

void PagePainter::paintPageOnPainter( const KPDFPage * page, int id, int flags,
    QPainter * destPainter, const QRect & limits, int width, int height )
{
    QPixmap * pixmap = 0;

    // if a pixmap is present for given id, use it
    if ( page->m_pixmaps.contains( id ) )
        pixmap = page->m_pixmaps[ id ];

    // else find the closest match using pixmaps of other IDs (great optim!)
    else if ( !page->m_pixmaps.isEmpty() && width != -1 )
    {
        int minDistance = -1;
        QMap< int,QPixmap * >::const_iterator it = page->m_pixmaps.begin(), end = page->m_pixmaps.end();
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

    if ( !pixmap )
    {
        if ( Settings::changeColors() &&
             Settings::renderMode() == Settings::EnumRenderMode::Paper )
            destPainter->fillRect( limits, Settings::paperColor() );
        else
            destPainter->fillRect( limits, Qt::white );

        // draw a cross (to  that the pixmap as not yet been loaded)
        // helps a lot on pages that take much to render
        destPainter->setPen( Qt::gray );
        destPainter->drawLine( 0, 0, width-1, height-1 );
        destPainter->drawLine( 0, height-1, width-1, 0 );
        return;
    }

    // we have a pixmap to paint, now let's paint it using a direct or buffered painter
    bool backBuffer = Settings::changeColors() &&
                      Settings::renderMode() != Settings::EnumRenderMode::Paper;
    // if PagePainter::Accessibility is not in 'flags', disable backBuffer
    backBuffer = backBuffer && (flags & Accessibility);
    QPixmap * backPixmap = 0;
    QPainter * p = destPainter;
    if ( backBuffer )
    {
        backPixmap = new QPixmap( limits.width(), limits.height() );
        p = new QPainter( backPixmap );
        p->translate( -limits.left(), -limits.top() );
    }

    // fast blit the pixmap if it has the right size..
    if ( pixmap->width() == width && pixmap->height() == height )
        p->drawPixmap( limits.topLeft(), *pixmap, limits );
    // ..else set a scale matrix to the painter and paint a quick 'zoomed' pixmap
    else
    {
        p->save();
        // TODO paint only the needed part
        p->scale( width / (double)pixmap->width(), height / (double)pixmap->height() );
        p->drawPixmap( 0,0, *pixmap, 0,0, pixmap->width(), pixmap->height() );
        p->restore();
        // draw a cross (to  that the pixmap has not the right size)
        p->setPen( Qt::gray );
        p->drawLine( 0, 0, width-1, height-1 );
        p->drawLine( 0, height-1, width-1, 0 );
    }

    // modify pixmap following accessibility settings
    if ( (flags & Accessibility) && backBuffer )
    {
        QImage backImage = backPixmap->convertToImage();
        switch ( Settings::renderMode() )
        {
            case Settings::EnumRenderMode::Inverted:
                // Invert image pixels using QImage internal function
                backImage.invertPixels(false);
                break;
            case Settings::EnumRenderMode::Recolor:
                // Recolor image using KImageEffect::flatten with dither:0
                KImageEffect::flatten( backImage, Settings::recolorForeground(), Settings::recolorBackground() );
                break;
            case Settings::EnumRenderMode::BlackWhite:
                // Manual Gray and Contrast
                unsigned int * data = (unsigned int *)backImage.bits();
                int val, pixels = backImage.width() * backImage.height(),
                    con = Settings::bWContrast(), thr = 255 - Settings::bWThreshold();
                for( int i = 0; i < pixels; ++i )
                {
                    val = qGray( data[i] );
                    if ( val > thr )
                        val = 128 + (127 * (val - thr)) / (255 - thr);
                    else if ( val < thr )
                        val = (128 * val) / thr;
                    if ( con > 2 )
                    {
                        val = con * ( val - thr ) / 2 + thr;
                        if ( val > 255 )
                            val = 255;
                        else if ( val < 0 )
                            val = 0;
                    }
                    data[i] = qRgba( val, val, val, 255 );
                }
                break;
        }
        backPixmap->convertFromImage( backImage );
    }

    // visually enchance links and images if requested
    bool hLinks = ( flags & EnhanceLinks ) && Settings::highlightLinks();
    bool hImages = ( flags & EnhanceImages ) && Settings::highlightImages();
    if ( hLinks || hImages )
    {
        QColor normalColor = QApplication::palette().active().highlight();
        QColor lightColor = normalColor.light( 140 );
        // enlarging limits for intersection is like growing the 'rectGeometry' below
        QRect limitsEnlarged = limits;
        limitsEnlarged.addCoords( -2, -2, 2, 2 );
        // draw rects that are inside the 'limits' paint region as opaque rects
        QValueList< KPDFPageRect * >::const_iterator lIt = page->m_rects.begin(), lEnd = page->m_rects.end();
        for ( ; lIt != lEnd; ++lIt )
        {
            KPDFPageRect * rect = *lIt;
            if ( (hLinks && rect->pointerType() == KPDFPageRect::Link) ||
                 (hImages && rect->pointerType() == KPDFPageRect::Image) )
            {
                QRect rectGeometry = rect->geometry();
                if ( rectGeometry.intersects( limitsEnlarged ) )
                {
                    // expand rect and draw inner border
                    rectGeometry.addCoords( -1,-1,1,1 );
                    p->setPen( lightColor );
                    p->drawRect( rectGeometry );
                    // expand rect to draw outer border
                    rectGeometry.addCoords( -1,-1,1,1 );
                    p->setPen( normalColor );
                    p->drawRect( rectGeometry );
                }
            }
        }
    }

    // draw selection (note: it is rescaled since the text page is at 100% scale)
    if ( ( flags & Highlight ) && ( page->attributes() & KPDFPage::Highlight ) )
    {
        int x = (int)( page->m_sLeft * width / page->m_width ),
            y = (int)( page->m_sTop * height / page->m_height ),
            w = (int)( page->m_sRight * width / page->m_width ) - x,
            h = (int)( page->m_sBottom * height / page->m_height ) - y;
        if ( w > 0 && h > 0 )
        {
            // TODO setRasterOp is no more on Qt4 find an alternative way of doing this
            p->setBrush( Qt::SolidPattern );
            p->setPen( QPen( Qt::black, 1 ) ); // should not be necessary bug a Qt bug makes it necessary
            p->setRasterOp( Qt::NotROP );
            p->drawRect( x, y, w, h );
        }
    }

    // if was backbuffering, copy the backPixmap to destination
    if ( backBuffer )
    {
        delete p;
        destPainter->drawPixmap( limits.left(), limits.top(), *backPixmap );
        delete backPixmap;
    }
}
