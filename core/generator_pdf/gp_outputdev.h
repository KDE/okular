/***************************************************************************
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Helio Chissini de Castro                        *
 *                           <helio@conectiva.com.br>                      *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef KPDFOUTPUTDEV_H
#define KPDFOUTPUTDEV_H

#ifdef __GNUC__
#pragma interface
#endif

#include <qvaluelist.h>
#include "xpdf/PDFDoc.h" // for 'Object'
#include "xpdf/SplashOutputDev.h"

class QPixmap;
class KPDFLink;
class ObjectRect;
class DocumentViewport;

/**
 * @short A SplashOutputDev renderer that grabs text and links.
 *
 * This output device:
 * - renders the page using SplashOutputDev (its parent)
 * - harvests text into a textPage (for searching text)
 * - harvests links and collects them
 * - collects images and collects them
 */
class KPDFOutputDev : public SplashOutputDev
{
    public:
        KPDFOutputDev( SplashColor paperColor );
        virtual ~KPDFOutputDev();

        // initialize device -> attach device to PDFDoc
        void initDevice( class PDFDoc * pdfDoc );

        // set parameters before rendering *each* page
        // @param qtThreadSafety: duplicate memory buffer (slow but safe)
        void setParams( int pixmapWidth, int pixmapHeight, 
                        bool decodeLinks, bool decodeImages, bool qtThreadSafety = false );

        // takes pointers out of the class (so deletion it's up to others)
        QPixmap * takePixmap();
        QImage * takeImage();
        QValueList< ObjectRect * > takeObjectRects();

        /** inherited from OutputDev */
        // End a page.
        virtual void endPage();
        //----- link borders
        virtual void processLink(Link *link, Catalog *catalog);
        //----- image drawing
        virtual void drawImage(GfxState *state, Object *ref, Stream *str, int width, int height,
                             GfxImageColorMap *colorMap, int *maskColors, GBool inlineImg);

    private:
        // delete all interal objects and data
        void clear();
        // generate a valid KPDFLink subclass (or null) from a xpdf's LinkAction
        KPDFLink * generateLink( LinkAction * a );
        // fills up a Viewport structure out of a given LinkGoto link
        DocumentViewport decodeViewport( GString *, class LinkDest * );

        // generator switches and parameters
        bool m_qtThreadSafety;
        bool m_generateLinks;
        bool m_generateImages;
        int m_pixmapWidth;
        int m_pixmapHeight;

        // Internal objects
        PDFDoc * m_doc;
        QPixmap * m_pixmap;
        QImage * m_image;
        QValueList< ObjectRect * > m_rects; // objectRects (links/images)
};

#endif
