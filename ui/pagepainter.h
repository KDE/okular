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

#include "core/page.h"  // for NormalizedPoint
class QPainter;
class QRect;

/**
 * @short Paints a Okular::Page to an open painter using given flags.
 */
class PagePainter
{
    public:
        // list of flags passed to the painting function. by OR-ing those flags
        // you can decide wether or not to permit drawing of a certain feature.
        enum PagePainterFlags { Accessibility = 1, EnhanceLinks = 2,
                                EnhanceImages = 4, Highlights = 8,
                                TextSelection = 16, Annotations = 32 };

        // draw (using painter 'p') the 'page' requested by 'id' using features
        // in 'flags'. 'limits' is the bounding rect of the paint operation,
        // 'scaledWidth' and 'scaledHeight' the expected size of page contents
        static void paintPageOnPainter( QPainter * p, const Okular::Page * page, int pixID,
            int flags, int scaledWidth, int scaledHeight, const QRect & pageLimits );

    private:
        // create an image taking the 'cropRect' portion of a pixmap. the
        // cropRect must be inside the source pixmap
        static void cropPixmapOnImage( QImage & dest, const QPixmap * src,
            const QRect & cropRect );

        // create an image taking the 'cropRect' portion of a pixmap scaled
        // to 'scaledWidth' by 'scaledHeight' pixels. cropRect must be inside
        // the QRect(0,0, scaledWidth,scaledHeight)
        static void scalePixmapOnImage( QImage & dest, const QPixmap * src,
            int scaledWidth, int scaledHeight, const QRect & cropRect, QImage::Format format = QImage::Format_RGB32 );

        // set the alpha component of the image to a given value
        static void changeImageAlpha( QImage & image, unsigned int alpha );

        // colorize a gray image to the given color
        static void colorizeImage( QImage & image, const QColor & color,
            unsigned int alpha = 255 );

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
