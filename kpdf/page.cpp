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

// system includes
#include <string.h>

// local includes
#include "Link.h"
#include "TextOutputDev.h"
#include "settings.h"
#include "page.h"


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
    QMap<int,QPixmap *>::iterator it = m_pixmaps.begin(), end = m_pixmaps.end();
    for ( ; it != end; ++it )
        delete *it;
    QValueList< KPDFLink * >::iterator lIt = m_links.begin(), lEnd = m_links.end();
    for ( ; lIt != lEnd; ++lIt )
        delete *lIt;
    QValueList< KPDFActiveRect * >::iterator rIt = m_rects.begin(), rEnd = m_rects.end();
    for ( ; rIt != rEnd; ++rIt )
        delete *rIt;
    delete m_text;
}


bool KPDFPage::hasPixmap( int id, int width, int height ) const
{
    if ( !m_pixmaps.contains( id ) )
        return false;
    QPixmap * p = m_pixmaps[ id ];
    return p ? ( p->width() == width && p->height() == height ) : false;
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

bool KPDFPage::hasActiveRect( int mouseX, int mouseY ) const
{
    if ( m_rects.count() < 1 )
        return false;
    QValueList< KPDFActiveRect * >::const_iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( mouseX, mouseY ) )
            return true;
    return false;
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

const KPDFLink * KPDFPage::getLink( int mouseX, int mouseY ) const
{
    QValueList< KPDFLink * >::const_iterator it = m_links.begin(), end = m_links.end();
    for ( ; it != end; ++it )
        if ( (*it)->contains( mouseX, mouseY ) )
            return *it;
    return 0;
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

void KPDFPage::setActiveRects( const QValueList<KPDFActiveRect *> rects )
{
    QValueList< KPDFActiveRect * >::iterator it = m_rects.begin(), end = m_rects.end();
    for ( ; it != end; ++it )
        delete *it;
    m_rects = rects;
}


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

    // visually enchance links active area if requested
    if ( ( flags & EnhanceLinks ) && Settings::highlightLinks() )
    {
        QColor normalColor = QApplication::palette().active().highlight();
        QColor lightColor = normalColor.light( 140 );
        // draw links that are inside the 'limits' paint region as opaque rects
        QValueList< KPDFLink * >::const_iterator lIt = page->m_links.begin(), lEnd = page->m_links.end();
        for ( ; lIt != lEnd; ++lIt )
        {
            QRect linkGeometry = (*lIt)->geometry();
            if ( linkGeometry.intersects( limits ) )
            {
                // expand rect and draw inner border
                linkGeometry.addCoords( -1,-1,1,1 );
                p->setPen( lightColor );
                p->drawRect( linkGeometry );
                // expand rect to draw outer border
                linkGeometry.addCoords( -1,-1,1,1 );
                p->setPen( normalColor );
                p->drawRect( linkGeometry );
            }
        }
    }

    // visually enchance image borders if requested
    if ( ( flags & EnhanceRects ) && Settings::highlightImages() )
    {
        QColor normalColor = QApplication::palette().active().highlight();
        QColor lightColor = normalColor.light( 140 );
        // draw links that are inside the 'limits' paint region as opaque rects
        QValueList< KPDFActiveRect * >::const_iterator rIt = page->m_rects.begin(), rEnd = page->m_rects.end();
        for ( ; rIt != rEnd; ++rIt )
        {
            QRect rectGeometry = (*rIt)->geometry();
            if ( rectGeometry.intersects( limits ) )
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


KPDFLink::KPDFLink( LinkAction * a )
    : m_type( Unknown ), x_min( 0 ), x_max( 0 ), y_min( 0 ), y_max( 0 ),
    m_dest( 0 ), m_destNamed( 0 ), m_fileName( 0 ), m_parameters( 0 ), m_uri( 0 )
{
    // set link action params processing (XPDF)LinkAction
    switch ( a->getKind() )
    {
    case actionGoTo: {
        LinkGoTo * g = (LinkGoTo *) a;
        m_type = Goto;
        // copy link dest (LinkDest class)
        LinkDest * d = g->getDest();
        m_dest = d ? d->copy() : 0;
        // copy link namedDest (const char *)
        GString * nd = g->getNamedDest();
        copyString( m_destNamed, nd ? nd->getCString() : 0 );
        } break;

    case actionGoToR: {
        m_type = Goto;
        LinkGoToR * g = (LinkGoToR *) a;
        // copy link file (const char *)
        copyString( m_fileName, g->getFileName()->getCString() );
        // copy link dest (LinkDest class)
        LinkDest * d = g->getDest();
        m_dest = d ? d->copy() : 0;
        // copy link namedDest (const char *)
        GString * nd = g->getNamedDest();
        copyString( m_destNamed, nd ? nd->getCString() : 0 );
        } break;

    case actionLaunch: {
        m_type = Execute;
        LinkLaunch * e = (LinkLaunch *)a;
        // copy name and parameters of the file to open(in case of PDF)/launch
        copyString( m_fileName, e->getFileName()->getCString() );
        GString * par = e->getParams();
        copyString( m_parameters, par ? par->getCString() : 0 );
        } break;

    case actionURI:
        m_type = URI;
        // copy URI (const char *)
        copyString( m_uri, ((LinkURI *)a)->getURI()->getCString() );
        break;

    case actionNamed:
        m_type = Named;
        // copy Action Name (const char * like Quit, Next, Back, etc..)
        copyString( m_uri, ((LinkNamed *)a)->getName()->getCString() );
        break;

    case actionMovie: {
        m_type = Movie;
        LinkMovie * m = (LinkMovie *) a;
        // copy Movie parameters (2 IDs and a const char *)
        Ref * r = m->getAnnotRef();
        m_refNum = r->num;
        m_refGen = r->gen;
        copyString( m_uri, m->getTitle()->getCString() );
        } break;

    case actionUnknown:
        break;
    }
}

void KPDFLink::setGeometry( int l, int t, int r, int b )
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


bool KPDFLink::contains( int x, int  y ) const
{
    return (x > x_min) && (x < x_max) && (y > y_min) && (y < y_max);
}

void KPDFLink::copyString( char * &dest, const char * src ) const
{
    if ( src )
    {
        dest = new char[ strlen(src) + 1 ];
        strcpy( &dest[0], src );
    }
}

QRect KPDFLink::geometry() const
{
    return QRect( x_min, y_min, x_max - x_min, y_max - y_min );
}


KPDFLink::LinkType KPDFLink::type() const
{
    return m_type;
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


KPDFActiveRect::KPDFActiveRect(int left, int top, int width, int height)
    : m_left(left), m_top(top), m_right(left + width), m_bottom(top + height)
{
}

bool KPDFActiveRect::contains(int x, int y)
{
    return (x > m_left) && (x < m_right) && (y > m_top) && (y < m_bottom);
}

QRect KPDFActiveRect::geometry() const
{
    return QRect( m_left, m_top, m_right - m_left, m_bottom - m_top );
}
