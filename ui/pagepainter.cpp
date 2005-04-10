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
            // iterate over annotations and paint AText, ALine, AHighlight, AInk
            QValueList< Annotation * >::const_iterator aIt = bufferedAnnotations->begin(), aEnd = bufferedAnnotations->end();
            for ( ; aIt != aEnd; ++aIt )
            {
                Annotation * a = *aIt;
                Annotation::SubType type = a->subType();

                // draw HighlightAnnotation
                if ( type == Annotation::AHighlight )
                {
                    // get the annotation
                    HighlightAnnotation * ha = (HighlightAnnotation *) a;

                    // precalc costants for normalizing the quads to the image
                    int quads = ha->highlightQuads.size();
                    double xOffset = (float)limits.left() / (float)scaledWidth,
                           xScale = (float)scaledWidth / (float)limits.width(),
                           yOffset = (float)limits.top() / (float)scaledHeight,
                           yScale = (float)scaledHeight / (float)limits.height();

                    // draw each quad of the annotation
                    for ( int q = 0; q < quads; q++ )
                    {
                        NormalizedPath path;
                        const HighlightAnnotation::Quad & quad = ha->highlightQuads[ q ];
                        for ( int i = 0; i < 4; i++ )
                        {
                            // normalize page point to image
                            NormalizedPoint point;
                            point.x = (quad.points[ i ].x - xOffset) * xScale;
                            point.y = (quad.points[ i ].y - yOffset) * yScale;
                            path.append( point );
                        }
                        //drawShapeOnImage( backImage, path );
                    }
                }
                // draw InkAnnotation
                //else if ( type == Annotation::AInk )
            }
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
            else
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

#define dbl_swap(x, y) { double tmp = (x); (x) = (y); (y) = tmp; }
void PagePainter::drawShapeOnImage(
    QImage & image,
    const NormalizedPath & path,
    const QPen & pen,
    const QBrush & brush,
    float /*antiAliasRadius*/ )
{
    // safety checks
    int points = path.size();
    if ( points < 2 )
        return;

    // transform NormalizedPoints inside image
    struct TPoint {
        double x;
        double y;
    };
    TPoint point[ points + 1 ];
    for ( int p = 0; p < points; p++ )
    {
        point[ p ].x = path[ p ].x * (double)image.width();
        point[ p ].y = path[ p ].y * (double)image.height();
    }
    point[ points ].x = point[ 0 ].x;
    point[ points ].y = point[ 0 ].y;

    // create and clear scanline buffers
    int imageHeight = image.height();
    int imageWidth = image.width();
    struct ScanLine
    {
        bool haveStart;
        double start;
        bool haveStop;
        double stop;
        ScanLine() : haveStart( false ), haveStop( false ) {};
    };
    ScanLine lines[ imageHeight ];

    // compute scanlines buffers
    for ( int l = 0; l < points; l++ )
    {
        bool downward = point[ l ].y < point[ l + 1 ].y;
        const TPoint & p1 = downward ? point[ l ] : point[ l + 1 ];
        const TPoint & p2 = downward ? point[ l + 1 ] : point[ l ];

        // scan vertically from p1 to p2
        int startY = (int)round( p1.y );
        int endY = (int)round( p2.y );

        // special horizontal case
        int deltaY = endY - startY;
        if ( !deltaY )
        {
            if ( startY >= 0 && startY < imageHeight )
            {
                ScanLine & line = lines[ startY ];
                line.start = p1.x;
                line.haveStart = true;
                line.stop = p2.x;
                line.haveStop = true;
            }
            continue;
        }

        // standard case
        double dX = p2.x - p1.x;
        for ( int y = startY; y < endY; y ++ )
        {
            // only process image area
            if ( y < 0 || y >= imageHeight )
                continue;

            double x = p1.x + ((double)(y-startY) / (double)deltaY) * dX;
            ScanLine & line = lines[ y ];
            if ( !line.haveStart )
            {
                // add start
                line.start = x;
                line.haveStart = true;
            }
            else if ( !line.haveStop )
            {
                // add stop
                line.stop = x;
                line.haveStop = true;
                if ( line.start > line.stop )
                     dbl_swap( line.start, line.stop );
            }
            else
            {
                // refine bounds
                if ( x < line.start )
                    line.start = x;
                else if ( x > line.stop )
                    line.stop = x;
            }
        }
    }

    // fill scanlines NOTE: bottom scanline is not visible? PLEASE CHECK!
    unsigned int * data = (unsigned int *)image.bits();
    int src, newR, newG, newB,
        rh = brush.color().red(),
        gh = brush.color().green(),
        bh = brush.color().blue();
    for ( int i = 0; i < imageHeight; i++ )
    {
        // get the current line and check it
        ScanLine & line = lines[ i ];
        if ( !line.haveStart || !line.haveStop ||
             line.start > imageWidth || line.stop < 0 )
            continue;
        if ( line.start > line.stop )
            dbl_swap( line.start, line.stop );

        // fill pixels
        int lineStart = line.start > 0 ? (int)line.start : 0;
        int lineStop = line.stop < imageWidth ? (int)line.stop : imageWidth - 1;
        int dataOffset = i * imageWidth + lineStart;
        for ( int x = lineStart; x <= lineStop; x++ )
        {
            src = data[ dataOffset ];
            newR = qt_div_255( qRed(src) * rh );
            newG = qt_div_255( qGreen(src) * gh );
            newB = qt_div_255( qBlue(src) * bh );
            data[ dataOffset ] = qRgba( newR, newG, newB, qAlpha(src) );
            dataOffset++;
        }
    }
}
