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
#include "core/link.h"

class QPixmap;
class TextPage;
class KPDFPageRect;

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
        void setParams( int pixmapWidth, int pixmapHeight, bool generateTextPage,
                        bool decodeLinks, bool decodeImages, bool qtThreadSafety = false );

        // takes pointers out of the class (so deletion it's up to others)
        QPixmap * takePixmap();
        QImage * takeImage();
        TextPage * takeTextPage();
        QValueList< KPDFPageRect * > takeRects();

        /** inherited from OutputDev */
        // Start a page.
        virtual void startPage(int pageNum, GfxState *state);
        // End a page.
        virtual void endPage();
        //----- link borders
        virtual void drawLink(Link *link, Catalog *catalog);
        //----- update text state
        virtual void updateFont(GfxState *state);
        //----- text drawing
        virtual void drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, Unicode *u, int uLen);
        virtual GBool beginType3Char(GfxState *state, double x, double y, double dx, double dy, CharCode code, Unicode *u, int uLen);
        //----- image drawing
        virtual void drawImage(GfxState *state, Object *ref, Stream *str, int width, int height,
                             GfxImageColorMap *colorMap, int *maskColors, GBool inlineImg);

    private:
        // delete all interal objects and data
        void clear();
        // generate a valid KPDFLink subclass (or null) from a xpdf's LinkAction
        KPDFLink * generateLink( LinkAction * a );
        // fills up a Viewport structure out of a given LinkGoto link
        KPDFLinkGoto::Viewport decodeViewport( GString *, class LinkDest * );

        // generator switches and parameters
        bool m_qtThreadSafety;
        bool m_generateText;
        bool m_generateLinks;
        bool m_generateImages;
        int m_pixmapWidth;
        int m_pixmapHeight;

        // Internal objects
        PDFDoc * m_doc;
        QPixmap * m_pixmap;
        QImage * m_image;
        TextPage * m_text;  // text page generated on demand
        QValueList< KPDFPageRect * > m_rects; // pageRects (links/images)
};


/**
 * @short Collect text into a takeable TextPage.
 *
 * This is the simplest OutputDev. It harvests text from currently
 * rendered page and provides a method for getting the TextPage.
 * Xpdf's textOutputDev can't return a textpage, unfortunately, and
 * KPDFOutputDev is too heavy for sucha a simple task.
 */
class KPDFTextDev : public OutputDev
{
    public:
        KPDFTextDev();
        virtual ~KPDFTextDev();

        // takes pointers out of the class (so deletion it's up to others)
        TextPage * takeTextPage();

        /** inherited from OutputDev */
        // top left corner is (0,0)
        virtual GBool upsideDown() { return gTrue; }
        // use draw char to get text data
        virtual GBool useDrawChar() { return gTrue; }
        // use drawChar even for Type3 chars
        virtual GBool interpretType3Chars() { return gFalse; }
        // do not pass non-text to this device
        virtual GBool needNonText() { return gFalse; }

        // Start a page.
        virtual void startPage(int, GfxState *state);
        // End a page.
        virtual void endPage();
        //----- update text state
        virtual void updateFont(GfxState *state);
        //----- text drawing
        virtual void drawChar(GfxState *state, double x, double y, double dx, double dy, double originX, double originY, CharCode code, Unicode *u, int uLen);

    private:
        // text page generated by execution
        TextPage * m_text;
};

#endif
