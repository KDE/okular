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
                                EnhanceImages = 4, Highlights = 8 };

        // draw (using painter 'p') the 'page' requested by 'id' using features
        // in 'flags'. 'limits' is the bounding rect of the paint operation,
        // 'width' and 'height' the expected size of page contents (used only
        // to pick up an alternative pixmap if the pixmap of 'id' is missing.
        static void paintPageOnPainter( const KPDFPage * page, int id, int flags,
            QPainter * p, const QRect & limits, int width = -1, int height = -1 );
};

#endif
