/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt / kde includes
#include <qrect.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qapplication.h>
#include <kimageeffect.h>

// local includes
#include "pagepainter.h"
#include "core/page.h"
#include "conf/settings.h"

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
/*  Code disabled (after HEAD merge). The user can recognize a wrong sized
        pixmap even without the grey cross..
        // draw a cross (for telling that the pixmap has not the right size)
        p->setPen( Qt::gray );
        p->drawLine( 0, 0, width-1, height-1 );
        p->drawLine( 0, height-1, width-1, 0 );
*/  }

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
    if ( ( flags & Highlights ) && !page->m_highlights.isEmpty() )
    {
        QValueList< HighlightRect * >::const_iterator hIt = page->m_highlights.begin(), hEnd = page->m_highlights.end();
        for ( ; hIt != hEnd; ++hIt )
        {
            HighlightRect * r = *hIt;
            int x = (int)( r->left * width ),
                y = (int)( r->top * height ),
                w = (int)( r->right * width ) - x,
                h = (int)( r->bottom * height ) - y;
            if ( w > 0 && h > 0 )
            {
                // TODO setRasterOp is no more on Qt4 find an alternative way of doing this
                p->setBrush( Qt::SolidPattern );
                p->setPen( QPen( Qt::black, 1 ) ); // should not be necessary bug a Qt bug makes it necessary
                p->setRasterOp( Qt::NotROP );
                p->drawRect( x, y, w, h );
            }
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
