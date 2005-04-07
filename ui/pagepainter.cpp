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
#include <kiconloader.h>

// local includes
#include "pagepainter.h"
#include "core/page.h"
#include "core/annotations.h"
#include "conf/settings.h"

void PagePainter::paintPageOnPainter( QPainter * destPainter, const KPDFPage * page,
    int pixID, int flags, int scaledWidth, int scaledHeight, const QRect & limits )
{
    /** 1 - RETRIEVE THE 'PAGE+ID' PIXMAP OR A SIMILAR 'PAGE' ONE **/
    const QPixmap * pixmap = 0;

    // if a pixmap is present for given id, use it
    if ( page->m_pixmaps.contains( pixID ) )
        pixmap = page->m_pixmaps[ pixID ];

    // else find the closest match using pixmaps of other IDs (great optim!)
    else if ( !page->m_pixmaps.isEmpty() )
    {
        int minDistance = -1;
        QMap< int,QPixmap * >::const_iterator it = page->m_pixmaps.begin(), end = page->m_pixmaps.end();
        for ( ; it != end; ++it )
        {
            int pixWidth = (*it)->width(),
                distance = pixWidth > scaledWidth ? pixWidth - scaledWidth : scaledWidth - pixWidth;
            if ( minDistance == -1 || distance < minDistance )
            {
                pixmap = *it;
                minDistance = distance;
            }
        }
    }

    /** 1B - IF NO PIXMAP, DRAW EMPTY PAGE **/
    double pixmapRescaleRatio = pixmap ? scaledWidth / (double)pixmap->width() : -1;
    long pixmapPixels = pixmap ? (long)pixmap->width() * (long)pixmap->height() : 0;
    if ( !pixmap || pixmapRescaleRatio > 20.0 || pixmapRescaleRatio < 0.25 ||
         (scaledWidth != pixmap->width() && pixmapPixels > 6000000L) )
    {
        if ( Settings::changeColors() &&
             Settings::renderMode() == Settings::EnumRenderMode::Paper )
            destPainter->fillRect( limits, Settings::paperColor() );
        else
            destPainter->fillRect( limits, Qt::white );

        // draw a cross (to  that the pixmap as not yet been loaded)
        // helps a lot on pages that take much to render
        destPainter->setPen( Qt::gray );
        destPainter->drawLine( 0, 0, scaledWidth-1, scaledHeight-1 );
        destPainter->drawLine( 0, scaledHeight-1, scaledWidth-1, 0 );
        // idea here: draw a hourglass (or kpdf icon :-) on top-left corner
        return;
    }

    /** 2 - FIND OUT WHAT TO PAINT (Flags + Configuration + Presence) **/
    bool paintAccessibility = (flags & Accessibility) && Settings::changeColors() && (Settings::renderMode() != Settings::EnumRenderMode::Paper);
    bool paintHighlights = (flags & Highlights) && !page->m_highlights.isEmpty();
    bool paintAnnotations = (flags & Annotations) && !page->m_annotations.isEmpty();
    bool enhanceLinks = (flags & EnhanceLinks) && Settings::highlightLinks();
    bool enhanceImages = (flags & EnhanceImages) && Settings::highlightImages();
    // check if there are really some highlightRects to paint
    if ( paintHighlights || paintAnnotations )
    {
        // precalc normalized 'limits rect' for intersection
        double nXMin = (double)limits.left() / (double)scaledWidth,
               nXMax = (double)limits.right() / (double)scaledWidth,
               nYMin = (double)limits.top() / (double)scaledHeight,
               nYMax = (double)limits.bottom() / (double)scaledHeight;
        // if no rect intersects limits, disable paintHighlights
        if ( paintHighlights )
        {
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
        // if no annotation intersects limits, disable paintAnnotations
        if ( paintAnnotations )
        {
            paintAnnotations = false;
            QValueList< Annotation * >::const_iterator aIt = page->m_annotations.begin(), aEnd = page->m_annotations.end();
            for ( ; aIt != aEnd; ++aIt )
            {
                if ( (*aIt)->boundary.intersects( nXMin, nYMin, nXMax, nYMax ) )
                {
                    paintAnnotations = true;
                    break;
                }
            }
        }
    }

    /** 3 - ENABLE BACKBUFFERING IF DIRECT IMAGE MANIPULATION IS NEEDED **/
    // FIXME: NOT ALL ANNOTATIONS REQUIRES BACKBUFFER
    bool useBackBuffer = paintAccessibility || paintHighlights || paintAnnotations;
    QPixmap * backPixmap = 0;
    QPainter * mixedPainter = 0;

    /** 4A -- REGULAR FLOW. PAINT PIXMAP NORMAL OR RESCALED USING GIVEN QPAINTER **/
    if ( !useBackBuffer )
    {
        // 4A.1. if size is ok, draw the page pixmap using painter
        if ( pixmap->width() == scaledWidth && pixmap->height() == scaledHeight )
            destPainter->drawPixmap( limits.topLeft(), *pixmap, limits );
        // else draw a scaled portion of the magnified pixmap
        else
        {
            QImage destImage;
            scalePixmapOnImage( destImage, pixmap, scaledWidth, scaledHeight, limits );
            destPainter->drawPixmap( limits.left(), limits.top(), destImage, 0, 0,
                                     limits.width(),limits.height() );
        }

        // 4A.2. active painter is the one passed to this method
        mixedPainter = destPainter;
    }
    /** 4B -- BUFFERED FLOW. IMAGE PAINTING + OPERATIONS. QPAINTER OVER PIXMAP  **/
    else
    {
        // the image over which we are going to draw
        QImage backImage;

        // 4B.1. draw the page pixmap: normal or scaled
        if ( pixmap->width() == scaledWidth && pixmap->height() == scaledHeight )
            cropPixmapOnImage( backImage, pixmap, limits );
        else
            scalePixmapOnImage( backImage, pixmap, scaledWidth, scaledHeight, limits );

        // 4B.2. modify pixmap following accessibility settings
        if ( paintAccessibility )
        {
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
        }
        // 4B.3. highlight rects in page
        if ( paintHighlights )
        {
            // draw highlights that are inside the 'limits' paint region
            QValueList< HighlightRect * >::const_iterator hIt = page->m_highlights.begin(), hEnd = page->m_highlights.end();
            for ( ; hIt != hEnd; ++hIt )
            {
                HighlightRect * r = *hIt;
                QRect highlightRect = r->geometry( scaledWidth, scaledHeight );
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
        // 4B.4. paint annotations [COMPOSITED ONES]
        if ( paintAnnotations )
        {
            // TODO draw AText(1), AHighlight
        }

        // 4B.5. create the back pixmap converting from the local image
        backPixmap = new QPixmap( backImage );

        // 4B.6. create a painter over the pixmap and set it as the active one
        mixedPainter = new QPainter( backPixmap );
        mixedPainter->translate( -limits.left(), -limits.top() );
    }

    /** 5 -- MIXED FLOW. Draw ANNOTATIONS [OPAQUE ONES] on ACTIVE PAINTER  **/
    if ( paintAnnotations )
    {
        // iterate over annotations and paint AText(2), ALine, AGeom, AStamp, AInk
        QValueList< Annotation * >::const_iterator aIt = page->m_annotations.begin(), aEnd = page->m_annotations.end();
        for ( ; aIt != aEnd; ++aIt )
        {
            Annotation * a = *aIt;
            QRect annotRect = a->boundary.geometry( scaledWidth, scaledHeight );

            // if annotation doesn't intersect paint region, skip it
            if ( !annotRect.isValid() || !annotRect.intersects( limits ) )
                continue;

            // draw extents rectangle
            if ( Settings::debugDrawAnnotationRect() )
            {
                mixedPainter->setPen( a->style.color );
                mixedPainter->drawRect( annotRect );
            }

            //
            //annotRect = annotRect.intersect( limits );
            Annotation::SubType type = a->subType();

            // stamp annotation TODO
            if ( type == Annotation::AStamp )
            {
                QPixmap pic = DesktopIcon( "kpdf" );
                //QImage destImage;
                //scalePixmapOnImage( destImage, &pic, annotRect.width(), annotRect.height(), QRect(0,0,annotRect.width(), annotRect.height()) );
                //mixedPainter->drawPixmap( annotRect.left(), annotRect.top(), destImage, 0, 0, annotRect.width(), annotRect.height() );
                pic = pic.convertToImage().scale( annotRect.width(), annotRect.height() );
                mixedPainter->drawPixmap( annotRect.left(), annotRect.top(), pic, 0, 0, annotRect.width(), annotRect.height() );
            }
            //else if ( type == Annotation::AText ) TODO
        }
    }

    /** 6 -- MIXED FLOW. Draw LINKS+IMAGES BORDER on ACTIVE PAINTER  **/
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
                QRect rectGeometry = rect->geometry( scaledWidth, scaledHeight );
                if ( rectGeometry.intersects( limitsEnlarged ) )
                {
                    // expand rect and draw inner border
                    rectGeometry.addCoords( -1,-1,1,1 );
                    mixedPainter->setPen( lightColor );
                    mixedPainter->drawRect( rectGeometry );
                    // expand rect to draw outer border
                    rectGeometry.addCoords( -1,-1,1,1 );
                    mixedPainter->setPen( normalColor );
                    mixedPainter->drawRect( rectGeometry );
                }
            }
        }
    }

    /** 7 -- BUFFERED FLOW. Copy BACKPIXMAP on DESTINATION PAINTER **/
    if ( useBackBuffer )
    {
        delete mixedPainter;
        destPainter->drawPixmap( limits.left(), limits.top(), *backPixmap );
        delete backPixmap;
    }
}


/** Private Helpers :: Pixmap conversion **/
void PagePainter::cropPixmapOnImage( QImage & dest, const QPixmap * src, const QRect & r )
{
    // handle quickly the case in which the whole pixmap has to be converted
    if ( r == QRect( 0, 0, src->width(), src->height() ) )
    {
        dest = src->convertToImage();
    }
    // else copy a portion of the src to an internal pixmap (smaller) and convert it
    else
    {
        QPixmap croppedPixmap( r.width(), r.height() );
        copyBlt( &croppedPixmap, 0, 0, src, r.left(), r.top(), r.width(), r.height() );
        dest = croppedPixmap.convertToImage();
    }
}

void PagePainter::scalePixmapOnImage ( QImage & dest, const QPixmap * src,
    int scaledWidth, int scaledHeight, const QRect & cropRect )
{
    // {source, destination, scaling} params
    int srcWidth = src->width(),
        srcHeight = src->height(),
        destLeft = cropRect.left(),
        destTop = cropRect.top(),
        destWidth = cropRect.width(),
        destHeight = cropRect.height();

    // destination image (same geometry as the pageLimits rect)
    dest = QImage( destWidth, destHeight, 32 );
    unsigned int * destData = (unsigned int *)dest.bits();

    // source image (1:1 conversion from pixmap)
    QImage srcImage = src->convertToImage();
    unsigned int * srcData = (unsigned int *)srcImage.bits();

    // precalc the x correspondancy conversion in a lookup table
    unsigned int xOffset[ destWidth ];
    for ( int x = 0; x < destWidth; x++ )
        xOffset[ x ] = ((x + destLeft) * srcWidth) / scaledWidth;

    // for each pixel of the destination image apply the color of the
    // corresponsing pixel on the source image (note: keep parenthesis)
    for ( int y = 0; y < destHeight; y++ )
    {
        unsigned int srcOffset = srcWidth * (((destTop + y) * srcHeight) / scaledHeight);
        for ( int x = 0; x < destWidth; x++ )
            (*destData++) = srcData[ srcOffset + xOffset[x] ];
    }
}

/** Private Helpers :: Image Drawing **/
//void image_draw_line( const QImage & img, bool antiAlias = true ) {}

