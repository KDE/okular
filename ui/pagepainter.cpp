/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pagepainter.h"

// qt / kde includes
#include <qrect.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <qvarlengtharray.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <QApplication>
#include <qimageblitz.h>

// system includes
#include <math.h>

// local includes
#include "core/area.h"
#include "core/page.h"
#include "core/page_p.h"
#include "core/annotations.h"
#include "core/utils.h"
#include "guiutils.h"
#include "settings.h"
#include "core/observer.h"
#include "core/tile.h"
#include "settings_core.h"
#include "core/document_p.h"

K_GLOBAL_STATIC_WITH_ARGS( QPixmap, busyPixmap, ( KIconLoader::global()->loadIcon("okular", KIconLoader::NoGroup, 32, KIconLoader::DefaultState, QStringList(), 0, true) ) )

#define TEXTANNOTATION_ICONSIZE 24

inline QPen buildPen( const Okular::Annotation *ann, double width, const QColor &color )
{
    QPen p(
        QBrush( color ),
        width,
        ann->style().lineStyle() == Okular::Annotation::Dashed ? Qt::DashLine : Qt::SolidLine,
        Qt::SquareCap,
        Qt::MiterJoin
    );
    return p;
}

void PagePainter::paintPageOnPainter( QPainter * destPainter, const Okular::Page * page,
    Okular::DocumentObserver *observer, int flags, int scaledWidth, int scaledHeight, const QRect &limits )
{
        paintCroppedPageOnPainter( destPainter, page, observer, flags, scaledWidth, scaledHeight, limits,
                                   Okular::NormalizedRect( 0, 0, 1, 1 ), 0 );
}

void PagePainter::paintCroppedPageOnPainter( QPainter * destPainter, const Okular::Page * page,
    Okular::DocumentObserver *observer, int flags, int scaledWidth, int scaledHeight, const QRect &limits,
    const Okular::NormalizedRect &crop, Okular::NormalizedPoint *viewPortPoint )
{
	/* Calculate the cropped geometry of the page */
	QRect scaledCrop = crop.geometry( scaledWidth, scaledHeight );
	int croppedWidth = scaledCrop.width();
	int croppedHeight = scaledCrop.height();

    QColor paperColor = Qt::white;
    QColor backgroundColor = paperColor;
    if ( Okular::SettingsCore::changeColors() )
    {
        switch ( Okular::SettingsCore::renderMode() )
        {
            case Okular::SettingsCore::EnumRenderMode::Inverted:
                backgroundColor = Qt::black;
                break;
            case Okular::SettingsCore::EnumRenderMode::Paper:
                paperColor = Okular::SettingsCore::paperColor();
                backgroundColor = paperColor;
                break;
            case Okular::SettingsCore::EnumRenderMode::Recolor:
                backgroundColor = Okular::Settings::recolorBackground();
                break;
            default: ;
        }
    }
    destPainter->fillRect( limits, backgroundColor );

    const bool hasTilesManager = page->hasTilesManager( observer );
    const QPixmap *pixmap = 0;

    if ( !hasTilesManager )
    {
        /** 1 - RETRIEVE THE 'PAGE+ID' PIXMAP OR A SIMILAR 'PAGE' ONE **/
        pixmap = page->_o_nearestPixmap( observer, scaledWidth, scaledHeight );

        /** 1B - IF NO PIXMAP, DRAW EMPTY PAGE **/
        double pixmapRescaleRatio = pixmap ? scaledWidth / (double)pixmap->width() : -1;
        long pixmapPixels = pixmap ? (long)pixmap->width() * (long)pixmap->height() : 0;
        if ( !pixmap || pixmapRescaleRatio > 20.0 || pixmapRescaleRatio < 0.25 ||
             (scaledWidth != pixmap->width() && pixmapPixels > 6000000L) )
        {
            // draw something on the blank page: the okular icon or a cross (as a fallback)
            if ( !busyPixmap->isNull() )
            {
                destPainter->drawPixmap( QPoint( 10, 10 ), *busyPixmap );
            }
            else
            {
                destPainter->setPen( Qt::gray );
                destPainter->drawLine( 0, 0, croppedWidth-1, croppedHeight-1 );
                destPainter->drawLine( 0, croppedHeight-1, croppedWidth-1, 0 );
            }
            return;
        }
    }

    /** 2 - FIND OUT WHAT TO PAINT (Flags + Configuration + Presence) **/
    bool canDrawHighlights = (flags & Highlights) && !page->m_highlights.isEmpty();
    bool canDrawTextSelection = (flags & TextSelection) && page->textSelection();
    bool canDrawAnnotations = (flags & Annotations) && !page->m_annotations.isEmpty();
    bool enhanceLinks = (flags & EnhanceLinks) && Okular::Settings::highlightLinks();
    bool enhanceImages = (flags & EnhanceImages) && Okular::Settings::highlightImages();
    // vectors containing objects to draw
    // make this a qcolor, rect map, since we don't need
    // to know s_id here! we are only drawing this right?
    QList< QPair<QColor, Okular::NormalizedRect> > * bufferedHighlights = 0;
    QList< Okular::Annotation * > * bufferedAnnotations = 0;
    QList< Okular::Annotation * > * unbufferedAnnotations = 0;
    Okular::Annotation *boundingRectOnlyAnn = 0; // Paint the bounding rect of this annotation
    // fill up lists with visible annotation/highlight objects/text selections
    if ( canDrawHighlights || canDrawTextSelection || canDrawAnnotations )
    {
        // precalc normalized 'limits rect' for intersection
        double nXMin = ( (double)limits.left() / (double)scaledWidth ) + crop.left,
               nXMax = ( (double)limits.right() / (double)scaledWidth )  + crop.left,
               nYMin = ( (double)limits.top() / (double)scaledHeight ) + crop.top,
               nYMax = ( (double)limits.bottom() / (double)scaledHeight ) + crop.top;
        // append all highlights inside limits to their list
        if ( canDrawHighlights )
        {
            if ( !bufferedHighlights )
                 bufferedHighlights = new QList< QPair<QColor, Okular::NormalizedRect> >();
/*            else
            {*/
                
                Okular::NormalizedRect* limitRect = new Okular::NormalizedRect(nXMin, nYMin, nXMax, nYMax );
                QLinkedList< Okular::HighlightAreaRect * >::const_iterator h2It = page->m_highlights.constBegin(), hEnd = page->m_highlights.constEnd();
                Okular::HighlightAreaRect::const_iterator hIt;
                for ( ; h2It != hEnd; ++h2It )
                    for (hIt=(*h2It)->constBegin(); hIt!=(*h2It)->constEnd(); ++hIt)
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
                const Okular::RegularAreaRect *textSelection = page->textSelection();
                Okular::HighlightAreaRect::const_iterator hIt = textSelection->constBegin(), hEnd = textSelection->constEnd();
                for ( ; hIt != hEnd; ++hIt )
                {
                    if ( (*hIt).intersects( limitRect ) )
                        bufferedHighlights->append( qMakePair( page->textSelectionColor(), *hIt ) );
                }
                delete limitRect;
            //}
        }
        // append annotations inside limits to the un/buffered list
        if ( canDrawAnnotations )
        {
            QLinkedList< Okular::Annotation * >::const_iterator aIt = page->m_annotations.constBegin(), aEnd = page->m_annotations.constEnd();
            for ( ; aIt != aEnd; ++aIt )
            {
                Okular::Annotation * ann = *aIt;
                int flags = ann->flags();

                if ( flags & Okular::Annotation::Hidden )
                    continue;

                if ( flags & Okular::Annotation::ExternallyDrawn )
                {
                    // ExternallyDrawn annots are never rendered by PagePainter.
                    // Just paint the boundingRect if the annot is BeingMoved
                    if ( flags & Okular::Annotation::BeingMoved )
                        boundingRectOnlyAnn = ann;
                    continue;
                }

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
    bool bufferAccessibility = (flags & Accessibility) && Okular::SettingsCore::changeColors() && (Okular::SettingsCore::renderMode() != Okular::SettingsCore::EnumRenderMode::Paper);
    bool useBackBuffer = bufferAccessibility || bufferedHighlights || bufferedAnnotations || viewPortPoint;
    QPixmap * backPixmap = 0;
    QPainter * mixedPainter = 0;
    QRect limitsInPixmap = limits.translated( scaledCrop.topLeft() );
        // limits within full (scaled but uncropped) pixmap

    /** 4A -- REGULAR FLOW. PAINT PIXMAP NORMAL OR RESCALED USING GIVEN QPAINTER **/
    if ( !useBackBuffer )
    {
        if ( hasTilesManager )
        {
            const Okular::NormalizedRect normalizedLimits( limitsInPixmap, scaledWidth, scaledHeight );
            const QList<Okular::Tile> tiles = page->tilesAt( observer, normalizedLimits );
            QList<Okular::Tile>::const_iterator tIt = tiles.constBegin(), tEnd = tiles.constEnd();
            while ( tIt != tEnd )
            {
                const Okular::Tile &tile = *tIt;
                QRect tileRect = tile.rect().geometry( scaledWidth, scaledHeight ).translated( -scaledCrop.topLeft() );
                QRect limitsInTile = limits & tileRect;
                if ( !limitsInTile.isEmpty() )
                {
                    if ( tile.pixmap()->width() == tileRect.width() && tile.pixmap()->height() == tileRect.height() )
                        destPainter->drawPixmap( limitsInTile.topLeft(), *(tile.pixmap()),
                                limitsInTile.translated( -tileRect.topLeft() ) );
                    else
                        destPainter->drawPixmap( tileRect, *(tile.pixmap()) );
                }
                tIt++;
            }
        }
        else
        {
            // 4A.1. if size is ok, draw the page pixmap using painter
            if ( pixmap->width() == scaledWidth && pixmap->height() == scaledHeight )
                destPainter->drawPixmap( limits.topLeft(), *pixmap, limitsInPixmap );

            // else draw a scaled portion of the magnified pixmap
            else
            {
                QImage destImage;
                scalePixmapOnImage( destImage, pixmap, scaledWidth, scaledHeight, limitsInPixmap );
                destPainter->drawImage( limits.left(), limits.top(), destImage, 0, 0,
                                         limits.width(),limits.height() );
            }
        }

        // 4A.2. active painter is the one passed to this method
        mixedPainter = destPainter;
    }
    /** 4B -- BUFFERED FLOW. IMAGE PAINTING + OPERATIONS. QPAINTER OVER PIXMAP  **/
    else
    {
        // the image over which we are going to draw
        QImage backImage;

        bool has_alpha;
        if ( pixmap )
            has_alpha = pixmap->hasAlpha();
        else
            has_alpha = true;

        if ( hasTilesManager )
        {
            backImage = QImage( limits.width(), limits.height(), QImage::Format_ARGB32_Premultiplied );
            backImage.fill( paperColor.rgb() );
            QPainter p( &backImage );
            const Okular::NormalizedRect normalizedLimits( limitsInPixmap, scaledWidth, scaledHeight );
            const QList<Okular::Tile> tiles = page->tilesAt( observer, normalizedLimits );
            QList<Okular::Tile>::const_iterator tIt = tiles.constBegin(), tEnd = tiles.constEnd();
            while ( tIt != tEnd )
            {
                const Okular::Tile &tile = *tIt;
                QRect tileRect = tile.rect().geometry( scaledWidth, scaledHeight ).translated( -scaledCrop.topLeft() );
                QRect limitsInTile = limits & tileRect;
                if ( !limitsInTile.isEmpty() )
                {
                    if ( !tile.pixmap()->hasAlpha() )
                        has_alpha = false;

                    if ( tile.pixmap()->width() == tileRect.width() && tile.pixmap()->height() == tileRect.height() )
                    {
                        p.drawPixmap( limitsInTile.translated( -limits.topLeft() ).topLeft(), *(tile.pixmap()),
                                limitsInTile.translated( -tileRect.topLeft() ) );
                    }
                    else
                    {
                        double xScale = tile.pixmap()->width() / (double)tileRect.width();
                        double yScale = tile.pixmap()->height() / (double)tileRect.height();
                        QTransform transform( xScale, 0, 0, yScale, 0, 0 );
                        p.drawPixmap( limitsInTile.translated( -limits.topLeft() ), *(tile.pixmap()),
                                transform.mapRect( limitsInTile ).translated( -transform.mapRect( tileRect ).topLeft() ) );
                    }
                }
                ++tIt;
            }
            p.end();
        }
        else
        {
            // 4B.1. draw the page pixmap: normal or scaled
            if ( pixmap->width() == scaledWidth && pixmap->height() == scaledHeight )
                cropPixmapOnImage( backImage, pixmap, limitsInPixmap );
            else
                scalePixmapOnImage( backImage, pixmap, scaledWidth, scaledHeight, limitsInPixmap );
        }

        // 4B.2. modify pixmap following accessibility settings
        if ( bufferAccessibility )
        {
            switch ( Okular::SettingsCore::renderMode() )
            {
                case Okular::SettingsCore::EnumRenderMode::Inverted:
                    // Invert image pixels using QImage internal function
                    backImage.invertPixels(QImage::InvertRgb);
                    break;
                case Okular::SettingsCore::EnumRenderMode::Recolor:
                    // Recolor image using Blitz::flatten with dither:0
                    Blitz::flatten( backImage, Okular::Settings::recolorForeground(), Okular::Settings::recolorBackground() );
                    break;
                case Okular::SettingsCore::EnumRenderMode::BlackWhite:
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
            QList< QPair<QColor, Okular::NormalizedRect> >::const_iterator hIt = bufferedHighlights->constBegin(), hEnd = bufferedHighlights->constEnd();
            for ( ; hIt != hEnd; ++hIt )
            {
                const Okular::NormalizedRect & r = (*hIt).second;
                // find out the rect to highlight on pixmap
                QRect highlightRect = r.geometry( scaledWidth, scaledHeight ).translated( -scaledCrop.topLeft() ).intersect( limits );
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
                        //for odt or epub
                        if(has_alpha)
                        {
                            newR = qRed(val);
                            newG = qGreen(val);
                            newB = qBlue(val);

                            if(newR == newG && newG == newB && newR == 0)
                                newR = newG = newB = 255;

                            newR = (newR * rh) / 255;
                            newG = (newG * gh) / 255;
                            newB = (newB * bh) / 255;
                        }
                        else
                        {
                            newR = (qRed(val) * rh) / 255;
                            newG = (qGreen(val) * gh) / 255;
                            newB = (qBlue(val) * bh) / 255;
                        }
                        data[ x + offset ] = qRgba( newR, newG, newB, 255 );
                    }
                    offset += backImage.width();
                }
            }
        }
        // 4B.4. paint annotations [COMPOSITED ONES]
        if ( bufferedAnnotations )
        {
            // Albert: This is quite "heavy" but all the backImage that reach here are QImage::Format_ARGB32_Premultiplied
            // and have to be so that the QPainter::CompositionMode_Multiply works
            // we could also put a
            // backImage = backImage.convertToFormat(QImage::Format_ARGB32_Premultiplied)
            // that would be almost a noop, but we'll leave the assert for now
            Q_ASSERT(backImage.format() == QImage::Format_ARGB32_Premultiplied);
            // precalc constants for normalizing [0,1] page coordinates into normalized [0,1] limit rect coordinates
            double pageScale = (double)croppedWidth / page->width();
            double xOffset = (double)limits.left() / (double)scaledWidth + crop.left,
                   xScale = (double)scaledWidth / (double)limits.width(),
                   yOffset = (double)limits.top() / (double)scaledHeight + crop.top,
                   yScale = (double)scaledHeight / (double)limits.height();

            // paint all buffered annotations in the page
            QList< Okular::Annotation * >::const_iterator aIt = bufferedAnnotations->constBegin(), aEnd = bufferedAnnotations->constEnd();
            for ( ; aIt != aEnd; ++aIt )
            {
                Okular::Annotation * a = *aIt;
                Okular::Annotation::SubType type = a->subType();
                QColor acolor = a->style().color();
                if ( !acolor.isValid() )
                    acolor = Qt::yellow;
                acolor.setAlphaF( a->style().opacity() );

                // draw LineAnnotation MISSING: all
                if ( type == Okular::Annotation::ALine )
                {
                    // get the annotation
                    Okular::LineAnnotation * la = (Okular::LineAnnotation *) a;

                    NormalizedPath path;
                    // normalize page point to image
                    const QLinkedList<Okular::NormalizedPoint> points = la->transformedLinePoints();
                    QLinkedList<Okular::NormalizedPoint>::const_iterator it = points.constBegin();
                    QLinkedList<Okular::NormalizedPoint>::const_iterator itEnd = points.constEnd();
                    for ( ; it != itEnd; ++it )
                    {
                        Okular::NormalizedPoint point;
                        point.x = ( (*it).x - xOffset) * xScale;
                        point.y = ( (*it).y - yOffset) * yScale;
                        path.append( point );
                    }

                    const QPen linePen = buildPen( a, a->style().width(), a->style().color() );
                    QBrush fillBrush;

                    if ( la->lineClosed() && la->lineInnerColor().isValid() )
                        fillBrush = QBrush( la->lineInnerColor() );

                    // draw the line as normalized path into image
                    drawShapeOnImage( backImage, path, la->lineClosed(),
                                      linePen,
                                      fillBrush, pageScale ,Multiply);

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

                        drawShapeOnImage( backImage, path2, false, linePen, QBrush(), pageScale, Multiply );
                        drawShapeOnImage( backImage, path3, false, linePen, QBrush(), pageScale, Multiply );
                    }
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
                                drawShapeOnImage( backImage, path, true, Qt::NoPen, acolor, pageScale, Multiply );
                                break;
                            // highlight the bottom part of the rect
                            case Okular::HighlightAnnotation::Squiggly:
                                path[ 3 ].x = ( path[ 0 ].x + path[ 3 ].x ) / 2.0;
                                path[ 3 ].y = ( path[ 0 ].y + path[ 3 ].y ) / 2.0;
                                path[ 2 ].x = ( path[ 1 ].x + path[ 2 ].x ) / 2.0;
                                path[ 2 ].y = ( path[ 1 ].y + path[ 2 ].y ) / 2.0;
                                drawShapeOnImage( backImage, path, true, Qt::NoPen, acolor, pageScale, Multiply );
                                break;
                            // make a line at 3/4 of the height
                            case Okular::HighlightAnnotation::Underline:
                                path[ 0 ].x = ( 3 * path[ 0 ].x + path[ 3 ].x ) / 4.0;
                                path[ 0 ].y = ( 3 * path[ 0 ].y + path[ 3 ].y ) / 4.0;
                                path[ 1 ].x = ( 3 * path[ 1 ].x + path[ 2 ].x ) / 4.0;
                                path[ 1 ].y = ( 3 * path[ 1 ].y + path[ 2 ].y ) / 4.0;
                                path.pop_back();
                                path.pop_back();
                                drawShapeOnImage( backImage, path, false, QPen( acolor, 2 ), QBrush(), pageScale );
                                break;
                            // make a line at 1/2 of the height
                            case Okular::HighlightAnnotation::StrikeOut:
                                path[ 0 ].x = ( path[ 0 ].x + path[ 3 ].x ) / 2.0;
                                path[ 0 ].y = ( path[ 0 ].y + path[ 3 ].y ) / 2.0;
                                path[ 1 ].x = ( path[ 1 ].x + path[ 2 ].x ) / 2.0;
                                path[ 1 ].y = ( path[ 1 ].y + path[ 2 ].y ) / 2.0;
                                path.pop_back();
                                path.pop_back();
                                drawShapeOnImage( backImage, path, false, QPen( acolor, 2 ), QBrush(), pageScale );
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

                    const QPen inkPen = buildPen( a, a->style().width(), acolor );

                    int paths = transformedInkPaths.size();
                    for ( int p = 0; p < paths; p++ )
                    {
                        NormalizedPath path;
                        const QLinkedList<Okular::NormalizedPoint> & inkPath = transformedInkPaths[ p ];

                        // normalize page point to image
                        QLinkedList<Okular::NormalizedPoint>::const_iterator pIt = inkPath.constBegin(), pEnd = inkPath.constEnd();
                        for ( ; pIt != pEnd; ++pIt )
                        {
                            const Okular::NormalizedPoint & inkPoint = *pIt;
                            Okular::NormalizedPoint point;
                            point.x = (inkPoint.x - xOffset) * xScale;
                            point.y = (inkPoint.y - yOffset) * yScale;
                            path.append( point );
                        }
                        // draw the normalized path into image
                        drawShapeOnImage( backImage, path, false, inkPen, QBrush(), pageScale );
                    }
                }
            } // end current annotation drawing
        }

        if(viewPortPoint)
        {
            QPainter painter(&backImage);
            painter.translate( -limits.left(), -limits.top() );
            painter.setPen( QApplication::palette().color( QPalette::Active, QPalette::Highlight ) );
            painter.drawLine( 0, viewPortPoint->y * scaledHeight + 1, scaledWidth - 1, viewPortPoint->y * scaledHeight + 1 );
// ROTATION CURRENTLY NOT IMPLEMENTED
/*
            if( page->rotation() == Okular::Rotation0)
            {

            }
            else if(page->rotation() == Okular::Rotation270)
            {
                painter.drawLine( viewPortPoint->y * scaledHeight + 1, 0, viewPortPoint->y * scaledHeight + 1, scaledWidth - 1);
            }
            else if(page->rotation() == Okular::Rotation180)
            {
                painter.drawLine( 0, (1.0 - viewPortPoint->y) * scaledHeight - 1, scaledWidth - 1, (1.0 - viewPortPoint->y) * scaledHeight - 1 );
            }
            else if(page->rotation() == Okular::Rotation90) // not right, rotation clock-wise
            {
                painter.drawLine( scaledWidth - (viewPortPoint->y * scaledHeight + 1), 0, scaledWidth - (viewPortPoint->y * scaledHeight + 1), scaledWidth - 1);
            }
*/
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
        QList< Okular::Annotation * >::const_iterator aIt = unbufferedAnnotations->constBegin(), aEnd = unbufferedAnnotations->constEnd();
        for ( ; aIt != aEnd; ++aIt )
        {
            Okular::Annotation * a = *aIt;

            // honor opacity settings on supported types
            unsigned int opacity = (unsigned int)( 255.0 * a->style().opacity() );
            // skip the annotation drawing if all the annotation is fully
            // transparent, but not with text annotations
            if ( opacity <= 0 && a->subType() != Okular::Annotation::AText )
                continue;

            QColor acolor = a->style().color();
            if ( !acolor.isValid() )
                acolor = Qt::yellow;
            acolor.setAlpha( opacity );

            // get annotation boundary and drawn rect
            QRect annotBoundary = a->transformedBoundingRectangle().geometry( scaledWidth, scaledHeight ).translated( -scaledCrop.topLeft() );
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
                    QImage image( annotBoundary.size(), QImage::Format_ARGB32 );
                    image.fill( acolor.rgba() );
                    QPainter painter( &image );
                    painter.setPen( Qt::black );
                    painter.setFont( text->textFont() );
                    Qt::AlignmentFlag halign = ( text->inplaceAlignment() == 1 ? Qt::AlignHCenter : ( text->inplaceAlignment() == 2 ? Qt::AlignRight : Qt::AlignLeft ) );
                    const double invXScale = (double)page->width() / scaledWidth;
                    const double invYScale = (double)page->height() / scaledHeight;
                    painter.scale( 1 / invXScale, 1 / invYScale );
                    painter.drawText( 2 * invXScale, 2 * invYScale,
                                      (image.width() - 2) * invXScale,
                                      (image.height() - 2) * invYScale,
                                      Qt::AlignTop | halign | Qt::TextWordWrap,
                                      text->contents() );
                    painter.resetTransform();
                    painter.drawRect( 0, 0, image.width() - 1, image.height() - 1 );
                    painter.end();

                    mixedPainter->drawImage( annotBoundary.topLeft(), image );
                }
                else if ( text->textType() == Okular::TextAnnotation::Linked )
                {
                // get pixmap, colorize and alpha-blend it
                    QString path;
                    QPixmap pixmap = GuiUtils::iconLoader()->loadIcon( text->textIcon().toLower(), KIconLoader::User, 32, KIconLoader::DefaultState, QStringList(), &path, true );
                    if ( path.isEmpty() )
                        pixmap = GuiUtils::iconLoader()->loadIcon( text->textIcon().toLower(), KIconLoader::NoGroup, 32 );
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
                        GuiUtils::colorizeImage( scaledImage, a->style().color(), opacity );
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
                QPixmap pixmap = GuiUtils::loadStamp( stamp->stampIconName(), annotBoundary.size() );
                if ( !pixmap.isNull() ) // should never happen but can happen on huge sizes
                {
                    QImage scaledImage;
                    scalePixmapOnImage( scaledImage, &pixmap, annotBoundary.width(),
                                        annotBoundary.height(), innerRect, QImage::Format_ARGB32 );
                    if ( opacity < 255 )
                        changeImageAlpha( scaledImage, opacity );
                    pixmap = QPixmap::fromImage( scaledImage );

                    // draw the scaled and al
                    mixedPainter->drawPixmap( annotRect.topLeft(), pixmap );
                }
            }
            // draw GeomAnnotation
            else if ( type == Okular::Annotation::AGeom )
            {
                Okular::GeomAnnotation * geom = (Okular::GeomAnnotation *)a;
                // check whether there's anything to draw
                if ( geom->style().width() || geom->geometricalInnerColor().isValid() )
                {
                    mixedPainter->save();
                    const double width = geom->style().width() * Okular::Utils::dpiX() / ( 72.0 * 2.0 ) * scaledWidth / page->width();
                    QRectF r( .0, .0, annotBoundary.width(), annotBoundary.height() );
                    r.adjust( width, width, -width, -width );
                    r.translate( annotBoundary.topLeft() );
                    if ( geom->geometricalInnerColor().isValid() )
                    {
                        r.adjust( width, width, -width, -width );
                        const QColor color = geom->geometricalInnerColor();
                        mixedPainter->setPen( Qt::NoPen );
                        mixedPainter->setBrush( QColor( color.red(), color.green(), color.blue(), opacity ) );
                        if ( geom->geometricalType() == Okular::GeomAnnotation::InscribedSquare )
                            mixedPainter->drawRect( r );
                        else
                            mixedPainter->drawEllipse( r );
                        r.adjust( -width, -width, width, width );
                    }
                    if ( geom->style().width() ) // need to check the original size here..
                    {
                        mixedPainter->setPen( buildPen( a, width * 2, acolor ) );
                        mixedPainter->setBrush( Qt::NoBrush );
                        if ( geom->geometricalType() == Okular::GeomAnnotation::InscribedSquare )
                            mixedPainter->drawRect( r );
                        else
                            mixedPainter->drawEllipse( r );
                    }
                    mixedPainter->restore();
                }
            }

            // draw extents rectangle
            if ( Okular::Settings::debugDrawAnnotationRect() )
            {
                mixedPainter->setPen( a->style().color() );
                mixedPainter->drawRect( annotBoundary );
            }
        }
    }

    if ( boundingRectOnlyAnn )
    {
        QRect annotBoundary = boundingRectOnlyAnn->transformedBoundingRectangle().geometry( scaledWidth, scaledHeight ).translated( -scaledCrop.topLeft() );
        mixedPainter->setPen( Qt::DashLine );
        mixedPainter->drawRect( annotBoundary );
    }

    /** 6 -- MIXED FLOW. Draw LINKS+IMAGES BORDER on ACTIVE PAINTER  **/
    if ( enhanceLinks || enhanceImages )
    {
        mixedPainter->save();
        mixedPainter->scale( scaledWidth, scaledHeight );
        mixedPainter->translate( -crop.left, -crop.top );

        QColor normalColor = QApplication::palette().color( QPalette::Active, QPalette::Highlight );
        // enlarging limits for intersection is like growing the 'rectGeometry' below
        QRect limitsEnlarged = limits;
        limitsEnlarged.adjust( -2, -2, 2, 2 );
        // draw rects that are inside the 'limits' paint region as opaque rects
        QLinkedList< Okular::ObjectRect * >::const_iterator lIt = page->m_rects.constBegin(), lEnd = page->m_rects.constEnd();
        for ( ; lIt != lEnd; ++lIt )
        {
            Okular::ObjectRect * rect = *lIt;
            if ( (enhanceLinks && rect->objectType() == Okular::ObjectRect::Action) ||
                 (enhanceImages && rect->objectType() == Okular::ObjectRect::Image) )
            {
                if ( limitsEnlarged.intersects( rect->boundingRect( scaledWidth, scaledHeight ).translated( -scaledCrop.topLeft() ) ) )
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
        dest = dest.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    // else copy a portion of the src to an internal pixmap (smaller) and convert it
    else
    {
        QImage croppedImage( r.width(), r.height(), QImage::Format_ARGB32_Premultiplied );
        QPainter p( &croppedImage );
        p.drawPixmap( 0, 0, *src, r.left(), r.top(), r.width(), r.height() );
        p.end();
        dest = croppedImage;
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
    QImage srcImage = src->toImage().convertToFormat(format);
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
static inline int qt_div_255(int x) { return (x + (x>>8) + 0x80) >> 8; }

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

    // stroke outline
    double penWidth = (double)pen.width() * penWidthMultiplier;
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen2 = pen;
    pen2.setWidthF(penWidth);
    painter.setPen(pen2);
    painter.setBrush(brush);

    if (op == Multiply) {
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
    }

    if ( brush.style() == Qt::NoBrush )
    {
        // create a polygon
        QPolygonF poly( closeShape ? pointsNumber + 1 : pointsNumber );
        for ( int i = 0; i < pointsNumber; ++i )
        {
            poly[ i ] = QPointF( normPath[ i ].x * fImageWidth, normPath[ i ].y * fImageHeight );
        }
        if ( closeShape )
            poly[ pointsNumber ] = poly[ 0 ];

        painter.drawPolyline( poly );
    }
    else
    {
        // create a 'path'
        QPainterPath path;
        path.setFillRule( Qt::WindingFill );

        path.moveTo( normPath[ 0 ].x * fImageWidth, normPath[ 0 ].y * fImageHeight );
        for ( int i = 1; i < pointsNumber; i++ )
        {
            path.lineTo( normPath[ i ].x * fImageWidth, normPath[ i ].y * fImageHeight );
        }
        if ( closeShape )
            path.closeSubpath();

        painter.drawPath( path );
    }
}

/* kate: replace-tabs on; indent-width 4; */

