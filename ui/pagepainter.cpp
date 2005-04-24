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

// system includes
#include <math.h>

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
    bool canDrawHighlights = (flags & Highlights) && !page->m_highlights.isEmpty();
    bool canDrawAnnotations = (flags & Annotations) && !page->m_annotations.isEmpty();
    bool enhanceLinks = (flags & EnhanceLinks) && Settings::highlightLinks();
    bool enhanceImages = (flags & EnhanceImages) && Settings::highlightImages();
    // vectors containing objects to draw
    QValueList< HighlightRect * > * bufferedHighlights = 0;
    QValueList< Annotation * > * bufferedAnnotations = 0;
    QValueList< Annotation * > * unbufferedAnnotations = 0;
    // fill up lists with visible annotation/highlight objects
    if ( canDrawHighlights || canDrawAnnotations )
    {
        // precalc normalized 'limits rect' for intersection
        double nXMin = (double)limits.left() / (double)scaledWidth,
               nXMax = (double)limits.right() / (double)scaledWidth,
               nYMin = (double)limits.top() / (double)scaledHeight,
               nYMax = (double)limits.bottom() / (double)scaledHeight;
        // append all highlights inside limits to their list
        if ( canDrawHighlights )
        {
            QValueList< HighlightRect * >::const_iterator hIt = page->m_highlights.begin(), hEnd = page->m_highlights.end();
            for ( ; hIt != hEnd; ++hIt )
                if ( (*hIt)->intersects( nXMin, nYMin, nXMax, nYMax ) )
                {
                    if ( !bufferedHighlights )
                        bufferedHighlights = new QValueList< HighlightRect * >();
                    bufferedHighlights->append( *hIt );
                }
        }
        // append annotations inside limits to the un/buffered list
        if ( canDrawAnnotations )
        {
            QValueList< Annotation * >::const_iterator aIt = page->m_annotations.begin(), aEnd = page->m_annotations.end();
            for ( ; aIt != aEnd; ++aIt )
            {
                Annotation * ann = *aIt;
                if ( ann->boundary.intersects( nXMin, nYMin, nXMax, nYMax ) )
                {
                    Annotation::SubType type = ann->subType();
                    if ( type == Annotation::ALine || type == Annotation::AHighlight ||
                         type == Annotation::AInk  /*|| (type == Annotation::AGeom && ann->style.opacity < 0.99)*/ )
                    {
                        if ( !bufferedAnnotations )
                            bufferedAnnotations = new QValueList< Annotation * >();
                        bufferedAnnotations->append( ann );
                    }
                    else
                    {
                        if ( !unbufferedAnnotations )
                            unbufferedAnnotations = new QValueList< Annotation * >();
                        unbufferedAnnotations->append( ann );
                    }
                }
            }
        }
        // end of intersections checking
    }

    /** 3 - ENABLE BACKBUFFERING IF DIRECT IMAGE MANIPULATION IS NEEDED **/
    bool bufferAccessibility = (flags & Accessibility) && Settings::changeColors() && (Settings::renderMode() != Settings::EnumRenderMode::Paper);
    bool useBackBuffer = bufferAccessibility || bufferedHighlights || bufferedAnnotations;
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
        if ( bufferAccessibility )
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
        if ( bufferedHighlights )
        {
            // draw highlights that are inside the 'limits' paint region
            QValueList< HighlightRect * >::const_iterator hIt = bufferedHighlights->begin(), hEnd = bufferedHighlights->end();
            for ( ; hIt != hEnd; ++hIt )
            {
                HighlightRect * r = *hIt;
                // find out the rect to highlight on pixmap
                QRect highlightRect = r->geometry( scaledWidth, scaledHeight ).intersect( limits );
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
        // 4B.4. paint annotations [COMPOSITED ONES]
        if ( bufferedAnnotations )
        {
            // precalc costants for normalizing the quads to the image
            double pageScale = (double)scaledWidth / page->width();
            double xOffset = (double)limits.left() / (double)scaledWidth,
                   xScale = (double)scaledWidth / (double)limits.width(),
                   yOffset = (double)limits.top() / (double)scaledHeight,
                   yScale = (double)scaledHeight / (double)limits.height();

            // paint all buffered annotations in the page
            QValueList< Annotation * >::const_iterator aIt = bufferedAnnotations->begin(), aEnd = bufferedAnnotations->end();
            for ( ; aIt != aEnd; ++aIt )
            {
                Annotation * a = *aIt;
                Annotation::SubType type = a->subType();

                // draw TextAnnotation (InPlace) MISSING: all
                if ( type == Annotation::AText )
                {
                    // TODO
                }
                // draw LineAnnotation MISSING: all
                else if ( type == Annotation::ALine )
                {
                    // TODO
                    // get the annotation
                    /*LineAnnotation * la = (LineAnnotation *) a;

                    NormalizedPath path;
                    // normalize page point to image
                    const NormalizedPoint & inkPoint = *pIt;
                    NormalizedPoint point;
                    point.x = (inkPoint.x - xOffset) * xScale;
                    point.y = (inkPoint.y - yOffset) * yScale;
                    path.append( point );
                    // draw the normalized path into image
                    drawShapeOnImage( backImage, path, false, QPen( a->style.color ), QBrush(), pageScale );
                    */
                }
                // draw GeomAnnotation MISSING: all
                else if ( type == Annotation::AGeom )
                {
                    // TODO
                }
                // draw HighlightAnnotation MISSING: under/strike width, feather, capping
                else if ( type == Annotation::AHighlight )
                {
                    // get the annotation
                    HighlightAnnotation * ha = (HighlightAnnotation *) a;
                    HighlightAnnotation::HighlightType type = ha->highlightType;

                    // draw each quad of the annotation
                    int quads = ha->highlightQuads.size();
                    for ( int q = 0; q < quads; q++ )
                    {
                        NormalizedPath path;
                        const HighlightAnnotation::Quad & quad = ha->highlightQuads[ q ];
                        // normalize page point to image
                        for ( int i = 0; i < 4; i++ )
                        {
                            NormalizedPoint point;
                            point.x = (quad.points[ i ].x - xOffset) * xScale;
                            point.y = (quad.points[ i ].y - yOffset) * yScale;
                            path.append( point );
                        }
                        // draw the normalized path into image
                        switch ( type )
                        {
                            // highlight the whole rect
                            case HighlightAnnotation::Highlight:
                                drawShapeOnImage( backImage, path, true, QPen(), a->style.color, pageScale, Multiply );
                                break;
                            // highlight the bottom part of the rect
                            case HighlightAnnotation::Squiggly:
                                path[ 0 ].x = ( path[ 0 ].x + path[ 3 ].x ) / 2.0;
                                path[ 0 ].y = ( path[ 0 ].y + path[ 3 ].y ) / 2.0;
                                path[ 1 ].x = ( path[ 1 ].x + path[ 2 ].x ) / 2.0;
                                path[ 1 ].y = ( path[ 1 ].y + path[ 2 ].y ) / 2.0;
                                drawShapeOnImage( backImage, path, true, QPen(), a->style.color, pageScale, Multiply );
                                break;
                            // make a line at 3/4 of the height
                            case HighlightAnnotation::Underline:
                                path[ 0 ].x = ( path[ 0 ].x + 3*path[ 3 ].x ) / 4.0;
                                path[ 0 ].y = ( path[ 0 ].y + 3*path[ 3 ].y ) / 4.0;
                                path[ 1 ].x = ( path[ 1 ].x + 3*path[ 2 ].x ) / 4.0;
                                path[ 1 ].y = ( path[ 1 ].y + 3*path[ 2 ].y ) / 4.0;
                                path.pop_back();
                                path.pop_back();
                                drawShapeOnImage( backImage, path, false, QPen( a->style.color, 2 ), QBrush(), pageScale );
                                break;
                            // make a line at 1/2 of the height
                            case HighlightAnnotation::StrikeOut:
                                path[ 0 ].x = ( path[ 0 ].x + path[ 3 ].x ) / 2.0;
                                path[ 0 ].y = ( path[ 0 ].y + path[ 3 ].y ) / 2.0;
                                path[ 1 ].x = ( path[ 1 ].x + path[ 2 ].x ) / 2.0;
                                path[ 1 ].y = ( path[ 1 ].y + path[ 2 ].y ) / 2.0;
                                path.pop_back();
                                path.pop_back();
                                drawShapeOnImage( backImage, path, false, QPen( a->style.color, 2 ), QBrush(), pageScale );
                                break;
                        }
                    }
                }
                // draw InkAnnotation MISSING:invar width, PENTRACER
                else if ( type == Annotation::AInk )
                {
                    // get the annotation
                    InkAnnotation * ia = (InkAnnotation *) a;

                    // draw each ink path
                    int paths = ia->inkPaths.size();
                    for ( int p = 0; p < paths; p++ )
                    {
                        NormalizedPath path;
                        const QValueList<NormalizedPoint> & inkPath = ia->inkPaths[ p ];

                        // normalize page point to image
                        QValueList<NormalizedPoint>::const_iterator pIt = inkPath.begin(), pEnd = inkPath.end();
                        for ( ; pIt != pEnd; ++pIt )
                        {
                            const NormalizedPoint & inkPoint = *pIt;
                            NormalizedPoint point;
                            point.x = (inkPoint.x - xOffset) * xScale;
                            point.y = (inkPoint.y - yOffset) * yScale;
                            path.append( point );
                        }
                        // draw the normalized path into image
                        drawShapeOnImage( backImage, path, false, QPen( a->style.color, a->style.width ), QBrush(), pageScale );
                    }
                }
            } // end current annotation drawing
        }

        // 4B.5. create the back pixmap converting from the local image
        backPixmap = new QPixmap( backImage );

        // 4B.6. create a painter over the pixmap and set it as the active one
        mixedPainter = new QPainter( backPixmap );
        mixedPainter->translate( -limits.left(), -limits.top() );
    }

    /** 5 -- MIXED FLOW. Draw ANNOTATIONS [OPAQUE ONES] on ACTIVE PAINTER  **/
    if ( unbufferedAnnotations )
    {
        // iterate over annotations and paint AText, AGeom, AStamp
        QValueList< Annotation * >::const_iterator aIt = unbufferedAnnotations->begin(), aEnd = unbufferedAnnotations->end();
        for ( ; aIt != aEnd; ++aIt )
        {
            Annotation * a = *aIt;

            // honour opacity settings on supported types
            unsigned int opacity = (unsigned int)( 255.0 * a->style.opacity );
            if ( opacity <= 0 )
                continue;

            // get annotation boundary and drawn rect
            QRect annotBoundary = a->boundary.geometry( scaledWidth, scaledHeight );
            QRect annotRect = annotBoundary.intersect( limits );
            QRect innerRect( annotRect.left() - annotBoundary.left(), annotRect.top() -
                    annotBoundary.top(), annotRect.width(), annotRect.height() );

            Annotation::SubType type = a->subType();

            // draw TextAnnotation (only the 'Linked' variant)
            if ( type == Annotation::AText )
            {
                TextAnnotation * text = (TextAnnotation *)a;
                if ( text->textType != TextAnnotation::Linked )
                    continue;

                // get pixmap, colorize and alpha-blend it
                QPixmap pixmap = DesktopIcon( text->textIcon );
                QImage scaledImage;
                scalePixmapOnImage( scaledImage, &pixmap, annotBoundary.width(),
                    annotBoundary.height(), innerRect );
                colorizeImage( scaledImage, a->style.color, opacity );
                scaledImage.setAlphaBuffer( true );
                pixmap.convertFromImage( scaledImage );

                // draw the mangled image to painter
                mixedPainter->drawPixmap( annotRect.topLeft(), pixmap );
            }
            // draw StampAnnotation
            else if ( type == Annotation::AStamp )
            {
                StampAnnotation * stamp = (StampAnnotation *)a;

                // get pixmap and alpha blend it if needed
                QPixmap pixmap = DesktopIcon( stamp->stampIconName );
                QImage scaledImage;
                scalePixmapOnImage( scaledImage, &pixmap, annotBoundary.width(),
                                    annotBoundary.height(), innerRect );
                if ( opacity < 255 )
                    changeImageAlpha( scaledImage, opacity );
                scaledImage.setAlphaBuffer( true );
                pixmap.convertFromImage( scaledImage );

                // draw the scaled and al
                mixedPainter->drawPixmap( annotRect.topLeft(), pixmap );
            }
            // draw GeomAnnotation
            else // WARNING: TEMPORARY CODE! migrate everything to AGG
            {
                //GeomAnnotation * geom = (GeomAnnotation *)a;
                //if ( geom->geomType == GeomAnnotation::InscribedSquare )
                //{
                    QImage rectImage( innerRect.width(), innerRect.height(), 32 );
                    const QColor & c = a->style.color;
                    unsigned int color = qRgba( c.red(), c.green(), c.blue(), opacity );
                    rectImage.fill( color );
                    rectImage.setAlphaBuffer( true );
                    QPixmap pixmap( rectImage );
                    mixedPainter->drawPixmap( annotRect.topLeft(), pixmap );
                //}
                //else if ( geom->geomType == GeomAnnotation::InscribedCircle )
            }

            // draw extents rectangle
            if ( Settings::debugDrawAnnotationRect() )
            {
                mixedPainter->setPen( a->style.color );
                mixedPainter->drawRect( annotBoundary );
            }
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

    // delete object containers
    delete bufferedHighlights;
    delete bufferedAnnotations;
    delete unbufferedAnnotations;
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
// from Arthur - qt4
inline int qt_div_255(int x) { return (x + (x>>8) + 0x80) >> 8; }

void PagePainter::changeImageAlpha( QImage & image, unsigned int destAlpha )
{
    // iterate over all pixels changing the alpha component value
    unsigned int * data = (unsigned int *)image.bits();
    unsigned int pixels = image.width() * image.height();

    int source, sourceAlpha;
    for( register unsigned int i = 0; i < pixels; ++i )
    {   // optimize this loop keeping byte order into account
        source = data[i];
        if ( (sourceAlpha = qAlpha( source )) == 255 )
        {
            // use destAlpha
            data[i] = qRgba( qRed(source), qGreen(source), qBlue(source), destAlpha );
        }
        else
        {
            // use destAlpha * sourceAlpha product
            sourceAlpha = qt_div_255( destAlpha * sourceAlpha );
            data[i] = qRgba( qRed(source), qGreen(source), qBlue(source), sourceAlpha );
        }
    }
}

void PagePainter::colorizeImage( QImage & grayImage, const QColor & color,
    unsigned int destAlpha )
{
    // iterate over all pixels changing the alpha component value
    unsigned int * data = (unsigned int *)grayImage.bits();
    unsigned int pixels = grayImage.width() * grayImage.height();
    int red = color.red(),
        green = color.green(),
        blue = color.blue();

    int source, sourceSat, sourceAlpha;
    for( register unsigned int i = 0; i < pixels; ++i )
    {   // optimize this loop keeping byte order into account
        source = data[i];
        sourceSat = qRed( source );
        int newR = qt_div_255( sourceSat * red ),
            newG = qt_div_255( sourceSat * green ),
            newB = qt_div_255( sourceSat * blue );
        if ( (sourceAlpha = qAlpha( source )) == 255 )
        {
            // use destAlpha
            data[i] = qRgba( newR, newG, newB, destAlpha );
        }
        else
        {
            // use destAlpha * sourceAlpha product
            if ( destAlpha < 255 )
                sourceAlpha = qt_div_255( destAlpha * sourceAlpha );
            data[i] = qRgba( newR, newG, newB, sourceAlpha );
        }
    }
}

//BEGIN of Anti-Grain dependant code
/** Shape Drawing using Anti-Grain Geometry library **/
// The following code uses the AGG2.3 lib imported into the "painter_agg2"
// directory. This is to be replaced by Arthur calls for drawing antialiased
// primitives, but until that AGG2 does its job very fast and good-looking.

#include "kpdf_pixfmt_rgba.h"
#include "agg_rendering_buffer.h"
#include "agg_renderer_base.h"
#include "agg_scanline_u.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_renderer_scanline.h"
#include "agg_conv_stroke.h"
#include "agg_path_storage.h"

void PagePainter::drawShapeOnImage(
    QImage & image,
    const NormalizedPath & normPath,
    bool closeShape,
    const QPen & pen,
    const QBrush & brush,
    double penWidthMultiplier,
    RasterOperation op
    //float antiAliasRadius
    )
{
    // safety checks
    int pointsNumber = normPath.size();
    if ( pointsNumber < 2 )
        return;

    int imageWidth = image.width();
    int imageHeight = image.height();
    double fImageWidth = (double)imageWidth;
    double fImageHeight = (double)imageHeight;

    // create a 'path'
    agg::path_storage path;
    path.move_to( normPath[ 0 ].x * fImageWidth, normPath[ 0 ].y * fImageHeight );
    for ( int i = 1; i < pointsNumber; i++ )
        path.line_to( normPath[ i ].x * fImageWidth, normPath[ i ].y * fImageHeight );
        //path.curve4( normPath[ i ].x * fImageWidth + 2, normPath[ i ].y * fImageHeight - 2,
        //             normPath[ i ].x * fImageWidth, normPath[ i ].y * fImageHeight );
    if ( closeShape )
        path.close_polygon();

    // create the 'rendering buffer' over qimage memory
    agg::rendering_buffer buffer( image.bits(), imageWidth, imageHeight, imageWidth << 2 );
    // create 'pixel buffer', 'clipped renderer', 'scanline renderer' on bgra32 format
    typedef agg::pixfmt_bgra32 bgra32;
    typedef agg::renderer_base< bgra32 > rb_bgra32;
    bgra32 pixels( buffer, op == Multiply ? 1 : 0 );
    rb_bgra32 rb( pixels );
    agg::renderer_scanline_aa_solid< rb_bgra32 > render( rb );
    // create rasterizer and scaline
    agg::rasterizer_scanline_aa<> rasterizer;
    agg::scanline_u8 scanline;

#if 0
    //draw RAINBOW
    agg::rgba8 span[ imageWidth ];
    for( int x = 0; x < imageWidth; x++ )
    {
        agg::rgba c( 380.0 + 400.0 * x / imageWidth, 0.8 );
        span[ x ] = agg::rgba8(c);
    }
    for( int y = 0; y < imageHeight; y++ )
        pixels.blend_color_hspan( 0, y, imageWidth, span, 0, (123*y)/imageHeight );
#endif

    // fill rect
    if ( brush.style() != Qt::NoBrush )
    {
        const QColor & brushColor = brush.color();
        render.color( agg::rgba8( brushColor.red(), brushColor.green(), brushColor.blue() ) );
        rasterizer.add_path( path );
        agg::render_scanlines( rasterizer, scanline, render );
        rasterizer.reset();
    }

    // stroke outline
    double penWidth = (double)pen.width() * penWidthMultiplier;
    if ( penWidth > 0.1 )
    {
        const QColor & penColor = pen.color();
        render.color( agg::rgba8( penColor.red(), penColor.green(), penColor.blue() ) );
#if 0
        // BSPLINE curve over path
        typedef agg::conv_bspline< agg::path_storage > conv_bspline_type;
        conv_bspline_type bspline( path );
        bspline.interpolation_step( 0.2 );
        agg::conv_stroke< conv_bspline_type > strokedPath( bspline );
#else
        agg::conv_stroke< agg::path_storage > strokedPath( path );
#endif
        strokedPath.width( penWidth );
        rasterizer.add_path( strokedPath );
        agg::render_scanlines( rasterizer, scanline, render );
    }
}

//END of Anti-Grain dependant code
