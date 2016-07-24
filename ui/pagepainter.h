/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_PAGEPAINTER_H_
#define _OKULAR_PAGEPAINTER_H_

#include <QtGui/QBrush>
#include <QtGui/QImage>
#include <QtGui/QPen>

#include "core/area.h"  // for NormalizedPoint

class QPainter;
class QRect;
namespace Okular {
    class DocumentObserver;
    class Page;
}

/**
 * @short Paints a Okular::Page to an open painter using given flags.
 */
class Q_DECL_EXPORT PagePainter
{
    public:
        // list of flags passed to the painting function. by OR-ing those flags
        // you can decide whether or not to permit drawing of a certain feature.
        enum PagePainterFlags { Accessibility = 1, EnhanceLinks = 2,
                                EnhanceImages = 4, Highlights = 8,
                                TextSelection = 16, Annotations = 32 };

        // draw (using painter 'p') the 'page' requested by 'observer' using features
        // in 'flags'. 'limits' is the bounding rect of the paint operation,
        // 'scaledWidth' and 'scaledHeight' the expected size of page contents
        static void paintPageOnPainter( QPainter * p, const Okular::Page * page, Okular::DocumentObserver *observer,
            int flags, int scaledWidth, int scaledHeight, const QRect & pageLimits );

        // draw (using painter 'p') the 'page' requested by 'observer' using features
        // in 'flags'.
        // 'pageLimits' is the bounding rect of the paint operation relative to the
        // top left of the (cropped) page.
        // 'scaledWidth' and 'scaledHeight' the size of the page pixmap (before cropping).
        // 'crop' is the (normalized) cropped rectangle within the page.
        // The painter's (0,0) is assumed to be top left of the painted ('pageLimits') rect.
        static void paintCroppedPageOnPainter( QPainter * p, const Okular::Page * page, Okular::DocumentObserver *observer,
            int flags, int scaledWidth, int scaledHeight, const QRect & pageLimits,
            const Okular::NormalizedRect & crop, Okular::NormalizedPoint *viewPortPoint );

    private:
        static void cropPixmapOnImage( QImage & dest, const QPixmap * src, const QRect & r );
        static void recolor(QImage *image, const QColor &foreground, const QColor &background);

        // create an image taking the 'cropRect' portion of an image scaled
        // to 'scaledWidth' by 'scaledHeight' pixels. cropRect must be inside
        // the QRect(0,0, scaledWidth,scaledHeight)
        static void scalePixmapOnImage( QImage & dest, const QPixmap *src,
            int scaledWidth, int scaledHeight, const QRect & cropRect, QImage::Format format = QImage::Format_ARGB32_Premultiplied );

        // set the alpha component of the image to a given value
        static void changeImageAlpha( QImage & image, unsigned int alpha );

        // my pretty dear raster function
        typedef QList< Okular::NormalizedPoint > NormalizedPath;
        enum RasterOperation { Normal, Multiply };
        static void drawShapeOnImage(
            QImage & image,
            const NormalizedPath & imagePoints,
            bool closeShape = true,
            const QPen & pen = QPen(),
            const QBrush & brush = QBrush(),
            double penWidthMultiplier = 1.0,
            RasterOperation op = Normal
            //float antiAliasRadius = 1.0
        );
};

#endif
