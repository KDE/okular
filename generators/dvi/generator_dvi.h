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

#include "core/generator.h"

class dviRenderer;
class dviPageInfo;
class DocumentViewport;
class Anchor;

class DviGenerator : public Generator
{
    Q_OBJECT
    public:
        DviGenerator( KPDFDocument * doc );
        bool loadDocument( const QString & fileName, QVector< KPDFPage * > & pagesVector );
        bool closeDocument();

        bool canGeneratePixmap( bool async );
        void generatePixmap( PixmapRequest * request );

        bool canGenerateTextPage();
        void generateSyncTextPage( KPDFPage * page );

        // document information
        const DocumentInfo *generateDocumentInfo();

        // table of contents
        const DocumentSynopsis *generateDocumentSynopsis();

        // rotation
        bool supportsRotation() { return true; };
        void setOrientation(QVector<KPDFPage*> & pagesVector, int orientation);

        // searching
        bool supportsSearching() { return true; };

   private:
        double m_resolution;

        DocumentInfo *m_docInfo;
        DocumentSynopsis *m_docSynopsis;

        bool ready;
        dviRenderer *m_dviRenderer;

        void loadPages( QVector< KPDFPage * > & pagesVector, int orientation );
        KPDFTextPage *extractTextFromPage( dviPageInfo *pageInfo, int orientation );
        void fillViewportFromAnchor( DocumentViewport &vp, const Anchor &anch, 
                                     int pW, int pH, int orientation ); 
        QLinkedList<ObjectRect*> generateDviLinks( const dviPageInfo *pageInfo, 
                                                   int orientation );
};

#endif
