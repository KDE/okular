/***************************************************************************
 *   Copyright (C) 2006 by Luigi Toscano <luigi.toscano@tiscali.it>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DVI_GENERATOR_H_
#define _DVI_GENERATOR_H_

#include <okular/core/generator.h>

class dviRenderer;
class dviPageInfo;
class Anchor;

namespace Okular {
class DocumentViewport;
class ObjectRect;
}

class DviGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        DviGenerator( QObject *parent, const QVariantList &args );
        bool loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector );

        bool canGeneratePixmap() const;
        void generatePixmap( Okular::PixmapRequest * request );

        // document information
        const Okular::DocumentInfo *generateDocumentInfo();

        // table of contents
        const Okular::DocumentSynopsis *generateDocumentSynopsis();

    protected:
        bool doCloseDocument();
        Okular::TextPage* textPage( Okular::Page *page );

   private:
        double m_resolution;

        Okular::DocumentInfo *m_docInfo;
        Okular::DocumentSynopsis *m_docSynopsis;

        bool ready;
        dviRenderer *m_dviRenderer;

        void loadPages( QVector< Okular::Page * > & pagesVector, int orientation );
        Okular::TextPage *extractTextFromPage( dviPageInfo *pageInfo, int orientation );
        void fillViewportFromAnchor( Okular::DocumentViewport &vp, const Anchor &anch, 
                                     int pW, int pH, int orientation ); 
        QLinkedList<Okular::ObjectRect*> generateDviLinks( const dviPageInfo *pageInfo, 
                                                   int orientation );
};

#endif
