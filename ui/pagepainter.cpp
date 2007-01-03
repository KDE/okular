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
#include <qpalette.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qvarlengtharray.h>
#include <kapplication.h>
#include <kimageeffect.h>
#include <kiconloader.h>
#include <kstaticdeleter.h>
#include <kdebug.h>

// system includes
#include <math.h>

// local includes
#include "pagepainter.h"
#include "core/area.h"
#include "core/page.h"
#include "core/annotations.h"
#include "core/utils.h"
#include "settings.h"

static KStaticDeleter<QPixmap> sd;
QPixmap * busyPixmap = 0;

#define TEXTANNOTATION_ICONSIZE 24

void PagePainter::paintPageOnPainter( QPainter * destPainter, const Okular::Page * page,
    int pixID, int flags, int scaledWidth, int scaledHeight, const QRect &limits )
{
    /** 1 - RETRIEVE THE 'PAGE+ID' PIXMAP OR A SIMILAR 'PAGE' ONE **/
    const QPixmap * pixmap = 0;

    // if a pixmap is present for given id, use it
    QMap< int, Okular::Page::PixmapObject >::const_iterator itPixmap = page->m_pixmaps.find( pixID );
    if ( itPixmap != page->m_pixmaps.end() )
        pixmap = itPixmap.value().m_pixmap;

    // else find the closest match using pixmaps of other IDs (great optim!)
    else if ( !page->m_pixmaps.isEmpty() )
    {
        int minDistance = -1;
        QMap< int, Okular::Page::PixmapObject >::const_iterator it = page->m_pixmaps.begin(), end = page->m_pixmaps.end();
        for ( ; it != end; ++it )
        {
            int pixWidth = (*it).m_pixmap->width(),
                distance = pixWidth > scaledWidth ? pixWidth - scaledWidth : scaledWidth - pixWidth;
            if ( minDistance == -1 || distance < minDistance )
            {
                pixmap = (*it).m_pixmap;
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
        if ( Okular::Settings::changeColors() &&
             Okular::Settings::renderMode() == Okular::Settings::EnumRenderMode::Paper )
            destPainter->fillRect( limits, Okular::Settings::paperColor() );
        else
            destPainter->fillRect( limits, Qt::white );

        if ( !busyPixmap )
        {
            sd.setObject(busyPixmap, new QPixmap());
            *busyPixmap = kapp->iconLoader()->loadIcon("okular", K3Icon::NoGroup, 32, K3Icon::DefaultState, 0, true);
        }
        // draw something on the blank page: the okular icon or a cross (as a fallback)
        if ( busyPixmap && !busyPixmap->isNull() )
        {
            destPainter->drawPixmap( QPoint( 10, 10 ), *busyPixmap );
        }
        else
        {
            destPainter->setPen( Qt::gray );
            destPainter->drawLine( 0, 0, scaledWidth-1, scaledHeight-1 );
            destPainter->drawLine( 0, scaledHeight-1, scaledWidth-1, 0 );
        }
        return;
    }

    /** 2 - FIND OUT WHAT TO PAINT (Flags + Configuration + Presence) **/
    bool canDrawHighlights = (flags & Highlights) && !page->m_highlights.isEmpty();
    bool canDrawTextSelection = (flags & TextSelection) && page->m_textSelections;
    bool canDrawAnnotations = (flags & Annotations) && !page->m_annotations.isEmpty();
    bool enhanceLinks = (flags & EnhanceLinks) && Okular::Settings::highlightLinks();
    bool enhanceImages = (flags & EnhanceImages) && Okular::Settings::highlightImages();
    // vectors containing objects to draw
    // make this a qcolor, rect map, since we don't need
    // to know s_id here! we are only drawing this right?
    QList< QPair<QColor, Okular::NormalizedRect> > * bufferedHighlights = 0;
    QList< Okular::Annotation * > * bufferedAnnotations = 0;
    QList< Okular::Annotation * > * unbufferedAnnotations = 0;
    // fill up lists with visible annotation/highlight objects/text selections
    if ( canDrawHighlights || canDrawTextSelection || canDrawAnnotations )
    {
        // precalc normalized 'limits rect' for intersection
        double nXMin = (double)limits.left() / (double)scaledWidth,
               nXMax = (double)limits.right() / (double)scaledWidth,
               nYMin = (double)limits.top() / (double)scaledHeight,
               nYMax = (double)limits.bottom() / (double)scaledHeight;
        // append all highlights inside limits to their list
        if ( canDrawHighlights )
        {
            if ( !bufferedHighlights )
                 bufferedHighlights = new QList< QPair<QColor, Okular::NormalizedRect> >();
/*            else
            {*/
                
                Okular::NormalizedRect* limitRect = new Okular::NormalizedRect(nXMin, nYMin, nXMax, nYMax );
                QLinkedList< Okular::HighlightAreaRect * >::const_iterator h2It = page->m_highlights.begin(), hEnd = page->m_highlights.end();
                Okular::HighlightAreaRect::const_iterator hIt;
                for ( ; h2It != hEnd; ++h2It )
                    for (hIt=(*h2It)->begin(); hIt!=(*h2It)->end(); ++hIt)
                    {
                        if ((*hIt).intersects(limitRect))
                            bufferedHighlights->append( qMakePair((*h2It)->color,*hIt) );
                    }
                delete limitRect;
            //}
        }
        if ( canDrawTextSelection )
        {
            if ( !bufferedHighlights )
                 bufferedHighlights = new QList< QPair<QColor, Okular::NormalizedRect>  >();
/*            else
            {*/
                Okular::NormalizedRect* limitRect = new Okular::NormalizedRect(nXMin, nYMin, nXMax, nYMax );
                Okular::HighlightAreaRect::const_iterator hIt = page->m_textSelections->begin(), hEnd = page->m_textSelections->end();
                for ( ; hIt != hEnd; ++hIt )
                {
                    if ( (*hIt).intersects( limitRect ) )
                        bufferedHighlights->append( qMakePair( page->m_textSelections->color, *hIt ) );
                }
                delete limitRect;
            //}
        }
        // append annotations inside limits to the un/buffered list
        if ( canDrawAnnotations )
        {
            QLinkedList< Okular::Annotation * >::const_iterator aIt = page->m_annotations.begin(), aEnd = page->m_annotations.end();
            for ( ; aIt != aEnd; ++aIt )
            {
                Okular::Annotation * ann = *aIt;
                if ( ann->flags() & Okular::Annotation::Hidden )
                    continue;

                bool intersects = ann->transformedBoundingRectangle().intersects( nXMin, nYMin, nXMax, nYMax );
                if ( ann->subType() == Okular::Annotation::AText )
                {
                    Okular::TextAnnotation * ta = static_cast< Okular::TextAnnotation * >( ann );
                    if ( ta->textType() == Okular::TextAnnotation::Linked )
                    {
                        Okular::NormalizedRect iconrect( ann->transformedBoundingRectangle().left,
                                                         ann->transformedBoundingRectangle().top,
                                                         ann->transformedBoundingRectangle().left + TEXTANNOTATION_ICONSIZE / page->width(),
                                                         ann->transformedBoundingRectangle().top + TEXTANNOTATION_ICONSIZE / page->height() );
                        intersects = iconrect.intersects( nXMin, nYMin, nXMax, nYMax );
                    }
                }
                if ( intersects )
                {
                    Okular::Annotation::SubType type = ann->subType();
                    if ( type == Okular::Annotation::ALine || type == Okular::Annotation::AHighlight ||
                         type == Okular::Annotation::AInk  /*|| (type == Annotation::AGeom && ann->style().opacity() < 0.99)*/ )
                    {
                        if ( !bufferedAnnotations )
                            bufferedAnnotations = new QList< Okular::Annotation * >();
                        bufferedAnnotations->append( ann );
                    }
                    else
                    {
                        if ( !unbufferedAnnotations )
                            unbufferedAnnotations = new QList< Okular::Annotation * >();
                        unbufferedAnnotations->append( ann );
                    }
                }
            }
        }
        // end of intersections checking
    }

    /** 3 - ENABLE BACKBUFFERING IF DIRECT IMAGE MANIPULATION IS NEEDED **/
    bool bufferAccessibility = (flags & Accessibility) && Okular::Settings::changeColors() && (Okular::Settings::renderMode() != Okular::Settings::EnumRenderMode::Paper);
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
            destPainter->drawImage( limits.left(), limits.top(), destImage, 0, 0,
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
            switch ( Okular::Settings::renderMode() )
            {
                case Okular::Settings::EnumRenderMode::Inverted:
                    // Invert image pixels using QImage internal function
                    backImage.invertPixels(QImage::InvertRgb);
                    break;
                case Okular::Settings::EnumRenderMode::Recolor:
                    // Recolor image using KImageEffect::flatten with dither:0
                    KImageEffect::flatten( backImage, Okular::Settings::recolorForeground(), Okular::Settings::recolorBackground() );
                    break;
                case Okular::Settings::EnumRenderMode::BlackWhite:
                    // Manual Gray and Contrast
                    unsigned int * data = (unsigned int *)backImage.bits();
                    int val, pixels = backImage.width() * backImage.height(),
                        con = Okular::Settings::bWContrast(), thr = 255 - Okular::Settings::bWThreshold();
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
            QList< QPair<QColor, Okular::NormalizedRect> >::const_iterator hIt = bufferedHighlights->begin(), hEnd = bufferedHighlights->end();
            for ( ; hIt != hEnd; ++hIt )
            {
                const Okular::NormalizedRect & r = (*hIt).second;
                // find out the rect to highlight on pixmap
                QRect highlightRect = r.geometry( scaledWidth, scaledHeight ).intersect( limits );
                highlightRect.translate( -limits.left(), -limits.top() );

                // highlight composition (product: highlight color * destcolor)
                unsigned int * data = (unsigned int *)backImage.bits();
                int val, newR, newG, newB,
                    rh = (*hIt).first.red(),
                    gh = (*hIt).first.green(),
                    bh = (*hIt).first.blue(),
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
            QList< Okular::Annotation * >::const_iterator aIt = bufferedAnnotations->begin(), aEnd = bufferedAnnotations->end();
            for ( ; aIt != aEnd; ++aIt )
            {
                Okular::Annotation * a = *aIt;
                Okular::Annotation::SubType type = a->subType();

                // draw LineAnnotation MISSING: all
                if ( type == Okular::Annotation::ALine )
                {
                    // get the annotation
                    Okular::LineAnnotation * la = (Okular::LineAnnotation *) a;

                    NormalizedPath path;
                    // normalize page point to image
                    const QLinkedList<Okular::NormalizedPoint> points = la->transformedLinePoints();
                    QLinkedList<Okular::NormalizedPoint>::const_iterator it = points.begin();
                    QLinkedList<Okular::NormalizedPoint>::const_iterator itEnd = points.end();
                    for ( ; it != itEnd; ++it )
                    {
                        Okular::NormalizedPoint point;
                        point.x = ( (*it).x - xOffset) * xScale;
                        point.y = ( (*it).y - yOffset) * yScale;
                        path.append( point );
                    }

                    // draw the line as normalized path into image
                    drawShapeOnImage( backImage, path, la->lineClosed(),
                                      QPen( a->style().color(), a->style().width() ),
                                      QBrush(), pageScale ,Multiply);

                    if ( path.count() == 2 && fabs( la->lineLeadingForwardPoint() ) > 0.1 )
                    {
                        Okular::NormalizedPoint delta( la->transformedLinePoints().last().x - la->transformedLinePoints().first().x, la->transformedLinePoints().first().y - la->transformedLinePoints().last().y );
                        double angle = atan2( delta.y, delta.x );
                        if ( delta.y < 0 )
                            angle += 2 * M_PI;

                        int sign = la->lineLeadingForwardPoint() > 0.0 ? 1 : -1;
                        double LLx = fabs( la->lineLeadingForwardPoint() ) * cos( angle + sign * M_PI_2 + 2 * M_PI ) / page->width();
                        double LLy = fabs( la->lineLeadingForwardPoint() ) * sin( angle + sign * M_PI_2 + 2 * M_PI ) / page->height();

                        NormalizedPath path2;
                        NormalizedPath path3;

                        Okular::NormalizedPoint point;
                        point.x = ( la->transformedLinePoints().first().x + LLx - xOffset ) * xScale;
                        point.y = ( la->transformedLinePoints().first().y - LLy - yOffset ) * yScale;
                        path2.append( point );
                        point.x = ( la->transformedLinePoints().last().x + LLx - xOffset ) * xScale;
                        point.y = ( la->transformedLinePoints().last().y - LLy - yOffset ) * yScale;
                        path3.append( point );
                        // do we have the extension on the "back"?
                        if ( fabs( la->lineLeadingBackwardPoint() ) > 0.1 )
                        {
                            double LLEx = la->lineLeadingBackwardPoint() * cos( angle - sign * M_PI_2 + 2 * M_PI ) / page->width();
                            double LLEy = la->lineLeadingBackwardPoint() * sin( angle - sign * M_PI_2 + 2 * M_PI ) / page->height();
                            point.x = ( la->transformedLinePoints().first().x + LLEx - xOffset ) * xScale;
                            point.y = ( la->transformedLinePoints().first().y - LLEy - yOffset ) * yScale;
                            path2.append( point );
                            point.x = ( la->transformedLinePoints().last().x + LLEx - xOffset ) * xScale;
                            point.y = ( la->transformedLinePoints().last().y - LLEy - yOffset ) * yScale;
                            path3.append( point );
                        }
                        else
                        {
                            path2.append( path[0] );
                            path3.append( path[1] );
                        }

                        drawShapeOnImage( backImage, path2, false, QPen( a->style().color(), a->style().width() ), QBrush(), pageScale, Multiply );
                        drawShapeOnImage( backImage, path3, false, QPen( a->style().color(), a->style().width() ), QBrush(), pageScale, Multiply );
                    }
                }
                // draw GeomAnnotation MISSING: all
                else if ( type == Okular::Annotation::AGeom )
                {
                    // TODO
                }
                // draw HighlightAnnotation MISSING: under/strike width, feather, capping
                else if ( type == Okular::Annotation::AHighlight )
                {
                    // get the annotation
                    Okular::HighlightAnnotation * ha = (Okular::HighlightAnnotation *) a;
                    Okular::HighlightAnnotation::HighlightType type = ha->highlightType();

                    // draw each quad of the annotation
                    int quads = ha->highlightQuads().size();
                    for ( int q = 0; q < quads; q++ )
                    {
                        NormalizedPath path;
                        const Okular::HighlightAnnotation::Quad & quad = ha->highlightQuads()[ q ];
                        // normalize page point to image
                        for ( int i = 0; i < 4; i++ )
                        {
                            Okular::NormalizedPoint point;
                            point.x = (quad.transformedPoint( i ).x - xOffset) * xScale;
                            point.y = (quad.transformedPoint( i ).y - yOffset) * yScale;
                            path.append( point );
                        }
                        // draw the normalized path into image
                        switch ( type )
                        {
                            // highlight the whole rect
                            case Okular::HighlightAnnotation::Highlight:
                                drawShapeOnImage( backImage, path, true, QPen(), a->style().color(), pageScale, Multiply );
                                break;
                            // highlight the bottom part of the rect
                            case Okular::HighlightAnnotation::Squiggly:
                                path[ 3 ].x = ( path[ 0 ].x + path[ 3 ].x ) / 2.0;
                                path[ 3 ].y = ( path[ 0 ].y + path[ 3 ].y ) / 2.0;
                                path[ 2 ].x = ( path[ 1 ].x + path[ 2 ].x ) / 2.0;
                                path[ 2 ].y = ( path[ 1 ].y + path[ 2 ].y ) / 2.0;
                                drawShapeOnImage( backImage, path, true, QPen(), a->style().color(), pageScale, Multiply );
                                break;
                            // make a line at 3/4 of the height
                            case Okular::HighlightAnnotation::Underline:
                                path[ 0 ].x = ( 3 * path[ 0 ].x + path[ 3 ].x ) / 4.0;
                                path[ 0 ].y = ( 3 * path[ 0 ].y + path[ 3 ].y ) / 4.0;
                                path[ 1 ].x = ( 3 * path[ 1 ].x + path[ 2 ].x ) / 4.0;
                                path[ 1 ].y = ( 3 * path[ 1 ].y + path[ 2 ].y ) / 4.0;
                                path.pop_back();
                                path.pop_back();
                                drawShapeOnImage( backImage, path, false, QPen( a->style().color(), 2 ), QBrush(), pageScale );
                                break;
                            // make a line at 1/2 of the height
                            case Okular::HighlightAnnotation::StrikeOut:
                                path[ 0 ].x = ( path[ 0 ].x + path[ 3 ].x ) / 2.0;
                                path[ 0 ].y = ( path[ 0 ].y + path[ 3 ].y ) / 2.0;
                                path[ 1 ].x = ( path[ 1 ].x + path[ 2 ].x ) / 2.0;
                                path[ 1 ].y = ( path[ 1 ].y + path[ 2 ].y ) / 2.0;
                                path.pop_back();
                                path.pop_back();
                                drawShapeOnImage( backImage, path, false, QPen( a->style().color(), 2 ), QBrush(), pageScale );
                                break;
                        }
                    }
                }
                // draw InkAnnotation MISSING:invar width, PENTRACER
                else if ( type == Okular::Annotation::AInk )
                {
                    // get the annotation
                    Okular::InkAnnotation * ia = (Okular::InkAnnotation *) a;

                    // draw each ink path
                    const QList< QLinkedList<Okular::NormalizedPoint> > transformedInkPaths = ia->transformedInkPaths();

                    int paths = transformedInkPaths.size();
                    for ( int p = 0; p < paths; p++ )
                    {
                        NormalizedPath path;
                        const QLinkedList<Okular::NormalizedPoint> & inkPath = transformedInkPaths[ p ];

                        // normalize page point to image
                        QLinkedList<Okular::NormalizedPoint>::const_iterator pIt = inkPath.begin(), pEnd = inkPath.end();
                        for ( ; pIt != pEnd; ++pIt )
                        {
                            const Okular::NormalizedPoint & inkPoint = *pIt;
                            Okular::NormalizedPoint point;
                            point.x = (inkPoint.x - xOffset) * xScale;
                            point.y = (inkPoint.y - yOffset) * yScale;
                            path.append( point );
                        }
                        // draw the normalized path into image
                        QColor acolor( a->style().color().red(), a->style().color().green(), a->style().color().blue(), (int)( 255.0 * a->style().opacity() ) );
                        drawShapeOnImage( backImage, path, false, QPen( acolor, a->style().width() ), QBrush(), pageScale );
                    }
                }
            } // end current annotation drawing
        }

        // 4B.5. create the back pixmap converting from the local image
        backPixmap = new QPixmap( QPixmap::fromImage( backImage ) );

        // 4B.6. create a painter over the pixmap and set it as the active one
        mixedPainter = new QPainter( backPixmap );
        mixedPainter->translate( -limits.left(), -limits.top() );
    }

    /** 5 -- MIXED FLOW. Draw ANNOTATIONS [OPAQUE ONES] on ACTIVE PAINTER  **/
    if ( unbufferedAnnotations )
    {
        // iterate over annotations and paint AText, AGeom, AStamp
        QList< Okular::Annotation * >::const_iterator aIt = unbufferedAnnotations->begin(), aEnd = unbufferedAnnotations->end();
        for ( ; aIt != aEnd; ++aIt )
        {
            Okular::Annotation * a = *aIt;

            // honor opacity settings on supported types
            unsigned int opacity = (unsigned int)( 255.0 * a->style().opacity() );
            // skip the annotation drawing if all the annotation is fully
            // transparent, but not with text annotations
            if ( opacity <= 0 && a->subType() != Okular::Annotation::AText )
                continue;

            // get annotation boundary and drawn rect
            QRect annotBoundary = a->transformedBoundingRectangle().geometry( scaledWidth, scaledHeight );
            QRect annotRect = annotBoundary.intersect( limits );
            QRect innerRect( annotRect.left() - annotBoundary.left(), annotRect.top() -
                    annotBoundary.top(), annotRect.width(), annotRect.height() );

            Okular::Annotation::SubType type = a->subType();

            // draw TextAnnotation
            if ( type == Okular::Annotation::AText )
            {
                Okular::TextAnnotation * text = (Okular::TextAnnotation *)a;
                if ( text->textType() == Okular::TextAnnotation::InPlace )
                {
                    QRect bigRect = a->transformedBoundingRectangle().geometry( (int)page->width(), (int)page->height() );

                    // the strategy behind 'bigger': if where are we going to
                    // draw is bigger than the Page, then draw the rect only
                    // after then scaling, so it won't be wider than 1px;
                    // otherwise draw it right after the text
                    bool bigger = mixedPainter->device()->width() > page->width();
                    QImage image( bigRect.size(), QImage::Format_ARGB32 );
                    image.fill( qRgba( a->style().color().red(), a->style().color().green(), a->style().color().blue(), opacity ) );
                    QPainter painter( &image );
                    painter.setPen( Qt::black );
                    painter.setFont( text->textFont() );
                    Qt::AlignmentFlag halign = ( text->inplaceAlignment() == 1 ? Qt::AlignHCenter : ( text->inplaceAlignment() == 2 ? Qt::AlignRight : Qt::AlignLeft ) );
                    painter.drawText( 2, 2, image.width() - 2, image.height() - 2,
                                      Qt::AlignTop | halign | Qt::TextWordWrap,
                                      text->inplaceText() );
                    if ( !bigger )
                        painter.drawRect( 0, 0, image.width() - 1, image.height() - 1 );
                    painter.end();
                    image = image.scaled( annotBoundary.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
                    if ( bigger )
                    {
                        painter.begin( &image );
                        painter.setPen( Qt::black );
                        painter.drawRect( 0, 0, image.width() - 1, image.height() - 1 );
                        painter.end();
                    }

//                    mixedPainter->drawImage( annotBoundary.topLeft(), image.scaled( annotBoundary.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
                    mixedPainter->drawImage( annotBoundary.topLeft(), image );
                }
                else if ( text->textType() == Okular::TextAnnotation::Linked )
                {
                // get pixmap, colorize and alpha-blend it
                    QString path;
                    QPixmap pixmap = kapp->iconLoader()->loadIcon( text->textIcon().toLower(), K3Icon::User, 32, K3Icon::DefaultState, &path, true );
                    if ( path.isEmpty() )
                        pixmap = kapp->iconLoader()->loadIcon( text->textIcon().toLower(), K3Icon::NoGroup, 32 );
                    QImage scaledImage;
                    QRect annotBoundary2 = QRect( annotBoundary.topLeft(), QSize( TEXTANNOTATION_ICONSIZE, TEXTANNOTATION_ICONSIZE ) );
                    QRect annotRect2 = annotBoundary2.intersect( limits );
                    QRect innerRect2( annotRect2.left() - annotBoundary2.left(), annotRect2.top() -
                    annotBoundary2.top(), annotRect2.width(), annotRect2.height() );
                    scalePixmapOnImage( scaledImage, &pixmap,
                                        TEXTANNOTATION_ICONSIZE, TEXTANNOTATION_ICONSIZE,
                                        innerRect2, QImage::Format_ARGB32 );
                    // if the annotation color is valid (ie it was set), then
                    // use it to colorize the icon, otherwise the icon will be
                    // "gray"
                    if ( a->style().color().isValid() )
                        colorizeImage( scaledImage, a->style().color(), opacity );
                    pixmap = QPixmap::fromImage( scaledImage );

                // draw the mangled image to painter
                    mixedPainter->drawPixmap( annotRect.topLeft(), pixmap );
                }

            }
            // draw StampAnnotation
            else if ( type == Okular::Annotation::AStamp )
            {
                Okular::StampAnnotation * stamp = (Okular::StampAnnotation *)a;

                // get pixmap and alpha blend it if needed
                QString path;
                QPixmap pixmap = kapp->iconLoader()->loadIcon( stamp->stampIconName().toLower(), K3Icon::User, qMin( annotBoundary.width(), annotBoundary.height() ), K3Icon::DefaultState, &path, true );
                if ( path.isEmpty() )
                    pixmap = kapp->iconLoader()->loadIcon( stamp->stampIconName().toLower(), K3Icon::NoGroup, qMin( annotBoundary.width(), annotBoundary.height() ) );
                QImage scaledImage;
                scalePixmapOnImage( scaledImage, &pixmap, annotBoundary.width(),
                                    annotBoundary.height(), innerRect, QImage::Format_ARGB32 );
                if ( opacity < 255 )
                    changeImageAlpha( scaledImage, opacity );
                pixmap = QPixmap::fromImage( scaledImage );

                // draw the scaled and al
                mixedPainter->drawPixmap( annotRect.topLeft(), pixmap );
            }
            // draw GeomAnnotation
            else if ( type == Okular::Annotation::AGeom )
            {
                Okular::GeomAnnotation * geom = (Okular::GeomAnnotation *)a;
                QImage shape( annotBoundary.size(), QImage::Format_ARGB32 );
                shape.fill( qRgba( 0, 0, 0, 0 ) );
                // width is already divided by two
                double width = geom->geometricalPointWidth() * Okular::Utils::dpiX() / ( 72.0 * 2.0 ) * scaledWidth / page->width();
                QPainter p( &shape );
                p.setPen( QPen( QBrush( QColor( a->style().color().red(), a->style().color().green(), a->style().color().blue(), opacity ) ), width * 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin ) );
                QRectF r( .0, .0, annotBoundary.width(), annotBoundary.height() );
                r.adjust( width, width, -width, -width );
                if ( geom->geometricalType() == Okular::GeomAnnotation::InscribedSquare )
                    p.drawRect( r );
                else
                    p.drawEllipse( r );
                if ( geom->geometricalInnerColor().isValid() )
                {
                    p.setPen( Qt::NoPen );
                    const QColor color = geom->geometricalInnerColor();
                    p.setBrush( QColor( color.red(), color.green(), color.blue(), opacity ) );
                    r.adjust( width, width, -width, -width );
                    if ( geom->geometricalType() == Okular::GeomAnnotation::InscribedSquare )
                        p.drawRect( r );
                    else
                        p.drawEllipse( r );
                }
                p.end();
                mixedPainter->drawImage( annotBoundary.topLeft(), shape );
            }
#if 0
            else // WARNING: TEMPORARY CODE! migrate everything to AGG
            {
                //GeomAnnotation * geom = (GeomAnnotation *)a;
                //if ( geom->geomType == GeomAnnotation::InscribedSquare )
                //{
                    QImage rectImage( innerRect.width(), innerRect.height(), QImage::Format_ARGB32 );
                    const QColor & c = a->style.color;
                    unsigned int color = qRgba( c.red(), c.green(), c.blue(), opacity );
                    rectImage.fill( color );
                    mixedPainter->drawImage( annotRect.topLeft(), rectImage );
                //}
                //else if ( geom->geomType == GeomAnnotation::InscribedCircle )
            }
#endif

            // draw extents rectangle
            if ( Okular::Settings::debugDrawAnnotationRect() )
            {
                mixedPainter->setPen( a->style().color() );
                mixedPainter->drawRect( annotBoundary );
            }
        }
    }

    /** 6 -- MIXED FLOW. Draw LINKS+IMAGES BORDER on ACTIVE PAINTER  **/
    if ( enhanceLinks || enhanceImages )
    {
        mixedPainter->save();
        mixedPainter->scale( scaledWidth, scaledHeight );

        QColor normalColor = QApplication::palette().color( QPalette::Active, QPalette::Highlight );
        QColor lightColor = normalColor.light( 140 );
        // enlarging limits for intersection is like growing the 'rectGeometry' below
        QRect limitsEnlarged = limits;
        limitsEnlarged.adjust( -2, -2, 2, 2 );
        // draw rects that are inside the 'limits' paint region as opaque rects
        QLinkedList< Okular::ObjectRect * >::const_iterator lIt = page->m_rects.begin(), lEnd = page->m_rects.end();
        for ( ; lIt != lEnd; ++lIt )
        {
            Okular::ObjectRect * rect = *lIt;
            if ( (enhanceLinks && rect->objectType() == Okular::ObjectRect::Link) ||
                 (enhanceImages && rect->objectType() == Okular::ObjectRect::Image) )
            {
                if ( limitsEnlarged.intersects( rect->boundingRect( scaledWidth, scaledHeight ) ) )
                {
                    mixedPainter->strokePath( rect->region(), QPen( normalColor ) );
                }
            }
        }
        mixedPainter->restore();
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
        dest = src->toImage();
    }
    // else copy a portion of the src to an internal pixmap (smaller) and convert it
    else
    {
        QPixmap croppedPixmap( r.width(), r.height() );
        QPainter p( &croppedPixmap );
        p.drawPixmap( 0, 0, *src, r.left(), r.top(), r.width(), r.height() );
        dest = croppedPixmap.toImage();
    }
}

void PagePainter::scalePixmapOnImage ( QImage & dest, const QPixmap * src,
    int scaledWidth, int scaledHeight, const QRect & cropRect, QImage::Format format )
{
    // {source, destination, scaling} params
    int srcWidth = src->width(),
        srcHeight = src->height(),
        destLeft = cropRect.left(),
        destTop = cropRect.top(),
        destWidth = cropRect.width(),
        destHeight = cropRect.height();

    // destination image (same geometry as the pageLimits rect)
    dest = QImage( destWidth, destHeight, format );
    unsigned int * destData = (unsigned int *)dest.bits();

    // source image (1:1 conversion from pixmap)
    QImage srcImage = src->toImage();
    unsigned int * srcData = (unsigned int *)srcImage.bits();

    // precalc the x correspondancy conversion in a lookup table
    QVarLengthArray<unsigned int> xOffset( destWidth );
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

#include "okular_pixfmt_rgba.h"
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
        render.color( agg::rgba8( brushColor.red(), brushColor.green(), brushColor.blue(), (int)( agg::rgba8::base_mask * brushColor.alphaF() ) ) );
        rasterizer.add_path( path );
        agg::render_scanlines( rasterizer, scanline, render );
        rasterizer.reset();
    }

    // stroke outline
    double penWidth = (double)pen.width() * penWidthMultiplier;
    if ( penWidth > 0.1 )
    {
        const QColor & penColor = pen.color();
        render.color( agg::rgba8( penColor.red(), penColor.green(), penColor.blue(), (int)( agg::rgba8::base_mask * penColor.alphaF() ) ) );
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
