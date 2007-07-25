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

    // if have no pixmap, draw blank page with gray cross and exit
    if ( !pixmap )
    {
        QColor color = Qt::white;
        if ( KpdfSettings::changeColors() )
        {
            switch ( KpdfSettings::renderMode() )
            {
                case KpdfSettings::EnumRenderMode::Inverted:
                    color = Qt::black;
                    break;
                case KpdfSettings::EnumRenderMode::Paper:
                    color = KpdfSettings::paperColor();
                    break;
                case KpdfSettings::EnumRenderMode::Recolor:
                    color = KpdfSettings::recolorBackground();
                    break;
                default: ;
            }
        }
        destPainter->fillRect( limits, color );

        // draw a cross (to  that the pixmap as not yet been loaded)
        // helps a lot on pages that take much to render
        destPainter->setPen( Qt::gray );
        destPainter->drawLine( 0, 0, width-1, height-1 );
        destPainter->drawLine( 0, height-1, width-1, 0 );
        // idea here: draw a hourglass (or kpdf icon :-) on top-left corner
        return;
    }

    // find out what to paint over the pixmap (manipulations / overlays)
    bool paintAccessibility = (flags & Accessibility) && KpdfSettings::changeColors() && (KpdfSettings::renderMode() != KpdfSettings::EnumRenderMode::Paper);
    bool paintHighlights = (flags & Highlights) && !page->m_highlights.isEmpty();
    bool enhanceLinks = (flags & EnhanceLinks) && KpdfSettings::highlightLinks();
    bool enhanceImages = (flags & EnhanceImages) && KpdfSettings::highlightImages();
    // check if there are really some highlightRects to paint
    if ( paintHighlights )
    {
        // precalc normalized 'limits rect' for intersection
        double nXMin = (double)limits.left() / (double)width,
               nXMax = (double)limits.right() / (double)width,
               nYMin = (double)limits.top() / (double)height,
               nYMax = (double)limits.bottom() / (double)height;
        // if no rect intersects limits, disable paintHighlights
        paintHighlights = false;
        QValueList< HighlightRect * >::const_iterator hIt = page->m_highlights.begin(), hEnd = page->m_highlights.end();
        for ( ; hIt != hEnd; ++hIt )
        {
            if ( (*hIt)->intersects( nXMin, nYMin, nXMax, nYMax ) )
            {
                paintHighlights = true;
                break;
            }
        }
    }

    // use backBuffer if 'pixmap direct manipulation' is needed
    bool backBuffer = paintAccessibility || paintHighlights;
    QPixmap * backPixmap = 0;
    QPainter * p = destPainter;
    if ( backBuffer )
    {
        // let's paint using a buffered painter
        backPixmap = new QPixmap( limits.width(), limits.height() );
        p = new QPainter( backPixmap );
        p->translate( -limits.left(), -limits.top() );
    }

    // 1. fast blit the pixmap if it has the right size..
    if ( pixmap->width() == width && pixmap->height() == height )
        p->drawPixmap( limits.topLeft(), *pixmap, limits );
    // ..else set a scale matrix to the painter and paint a quick 'zoomed' pixmap
    else
    {
        p->save();
        // TODO paint only the needed part (note: hope that Qt4 transforms are faster)
        p->scale( width / (double)pixmap->width(), height / (double)pixmap->height() );
        p->drawPixmap( 0,0, *pixmap, 0,0, pixmap->width(), pixmap->height() );
        p->restore();
    }

    // 2. mangle pixmap: convert it to 32-bit qimage and perform pixel-level manipulations
    if ( backBuffer )
    {
        QImage backImage = backPixmap->convertToImage();
        // 2.1. modify pixmap following accessibility settings
        if ( paintAccessibility )
        {
            switch ( KpdfSettings::renderMode() )
            {
                case KpdfSettings::EnumRenderMode::Inverted:
                    // Invert image pixels using QImage internal function
                    backImage.invertPixels(false);
                    break;
                case KpdfSettings::EnumRenderMode::Recolor:
                    // Recolor image using KImageEffect::flatten with dither:0
                    KImageEffect::flatten( backImage, KpdfSettings::recolorForeground(), KpdfSettings::recolorBackground() );
                    break;
                case KpdfSettings::EnumRenderMode::BlackWhite:
                    // Manual Gray and Contrast
                    unsigned int * data = (unsigned int *)backImage.bits();
                    int val, pixels = backImage.width() * backImage.height(),
                        con = KpdfSettings::bWContrast(), thr = 255 - KpdfSettings::bWThreshold();
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
        }
        // 2.2. highlight rects in page
        if ( paintHighlights )
        {
            // draw highlights that are inside the 'limits' paint region
            QValueList< HighlightRect * >::const_iterator hIt = page->m_highlights.begin(), hEnd = page->m_highlights.end();
            for ( ; hIt != hEnd; ++hIt )
            {
                HighlightRect * r = *hIt;
                QRect highlightRect = r->geometry( width, height );
                if ( highlightRect.isValid() && highlightRect.intersects( limits ) )
                {
                    // find out the rect to highlight on pixmap
                    highlightRect = highlightRect.intersect( limits );
                    highlightRect.moveBy( -limits.left(), -limits.top() );

                    // highlight composition (product: highlight color * destcolor)
                    unsigned int * data = (unsigned int *)backImage.bits();
                    int val, newR, newG, newB,
                        rh = r->color.red(),
                        gh = r->color.green(),
                        bh = r->color.blue(),
                        offset = highlightRect.top() * backImage.width();
                    for( int y = highlightRect.top(); y <= highlightRect.bottom(); ++y )
                    {
                        for( int x = highlightRect.left(); x <= highlightRect.right(); ++x )
                        {
                            val = data[ x + offset ];
                            newR = (qRed(val) * rh) / 255;
                            newG = (qGreen(val) * gh) / 255;
                            newB = (qBlue(val) * bh) / 255;
                            data[ x + offset ] = qRgba( newR, newG, newB, 255 );
                        }
                        offset += backImage.width();
                    }
                }
            }
        }
        backPixmap->convertFromImage( backImage );
    }

    // 3. visually enchance links and images if requested
    if ( enhanceLinks || enhanceImages )
    {
        QColor normalColor = QApplication::palette().active().highlight();
        QColor lightColor = normalColor.light( 140 );
        // enlarging limits for intersection is like growing the 'rectGeometry' below
        QRect limitsEnlarged = limits;
        limitsEnlarged.addCoords( -2, -2, 2, 2 );
        // draw rects that are inside the 'limits' paint region as opaque rects
        QValueList< ObjectRect * >::const_iterator lIt = page->m_rects.begin(), lEnd = page->m_rects.end();
        for ( ; lIt != lEnd; ++lIt )
        {
            ObjectRect * rect = *lIt;
            if ( (enhanceLinks && rect->objectType() == ObjectRect::Link) ||
                 (enhanceImages && rect->objectType() == ObjectRect::Image) )
            {
                QRect rectGeometry = rect->geometry( width, height );
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

    // 4. if was backbuffering, copy the backPixmap to destination
    if ( backBuffer )
    {
        delete p;
        destPainter->drawPixmap( limits.left(), limits.top(), *backPixmap );
        delete backPixmap;
    }
}
