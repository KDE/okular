/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_PAGEPAINTER_H_
#define _KPDF_PAGEPAINTER_H_

class KPDFPage;
class QPainter;
class QRect;

/**
 * @short Paints a KPDFPage to an open painter using given flags.
 */
class PagePainter
{
    public:
        // list of flags passed to the painting function. by OR-ing those flags
        // you can decide wether or not to permit drawing of a certain feature.
        enum PagePainterFlags { Accessibility = 1, EnhanceLinks = 2,
                                EnhanceImages = 4, Highlights = 8,
                                Annotations = 16 };

        // draw (using painter 'p') the 'page' requested by 'id' using features
        // in 'flags'. 'limits' is the bounding rect of the paint operation,
        // 'pageWidth' and 'pageHeight' the expected size of page contents
        static void paintPageOnPainter( const KPDFPage * page, int id, int flags,
            QPainter * p, int pageWidth, int pageHeight, const QRect & pageLimits );

    private:
        // create an image taking the 'cropRect' portion of a pixmap. the
        // cropRect must be inside the source pixmap
        static void cropPixmapOnImage( const QPixmap * src, QImage & dest,
            const QRect & cropRect );
        // create an image taking the 'cropRect' portion of a pixmap scaled
        // to 'pageWidth' by 'pageHeight' pixels. cropRect must be inside
        // the QRect(0,0, pageWidth,pageHeight)
        static void scalePixmapOnImage( const QPixmap * src, QImage & dest,
            int pageWidth, int pageHeight, const QRect & cropRect );
};

#endif
