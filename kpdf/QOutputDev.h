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
class TextPage;
class GeneratorPDF;
class KPDFLink;
class KPDFPageRect;

/**
 * @short A SplashOutputDev renderer that grabs text and links.
 * ### MERGE: rename definition/impl to generator_pdf_outputdev.h/.cpp
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

        // to be called before PDFDoc->displayPage( thisclass, .. )
        void setParams( int pixmapWidth, int pixmapHeight, bool generateTextpage,
                        bool decodeLinks, bool decodeImages, GeneratorPDF * parent );

        // generate a valid KPDFLink subclass (or null) from a xpdf's LinkAction
        KPDFLink * generateLink( LinkAction * );

        // takes pointers out of the class (so deletion it's up to others)
        QPixmap * takePixmap();
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

        // generator switches and parameters
        bool m_generateText;
        bool m_generateLinks;
        bool m_generateImages;
        int m_pixmapWidth;
        int m_pixmapHeight;
        QPixmap * m_pixmap;
        GeneratorPDF * m_generator;

        // text page generated on demand
        TextPage * m_text;

        // rectangles on page (associated to links/images)
        QValueList< KPDFPageRect * > m_rects;
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
