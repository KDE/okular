/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_GENERATOR_PDF_H_
#define _KPDF_GENERATOR_PDF_H_

#include <qmutex.h>
#include <qcolor.h>
#include <qstring.h>
#include "generator.h"
#include "document.h"
#include "link.h"

class PDFDoc;
class GList;
class KPDFOutputDev;

/**
 * @short A generator that builds contents from a PDF document.
 *
 * ...
 */
class GeneratorPDF : public Generator
{
    public:
        GeneratorPDF();
        virtual ~GeneratorPDF();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QValueVector<KPDFPage*> & pagesVector );

        // [INHERITED] document informations
        const DocumentInfo * documentInfo();
        const DocumentSynopsis * documentSynopsis();

        // [INHERITED] perform actions on document / pages
        bool print( KPrinter& printer );
        bool requestPixmap( int id, KPDFPage * page, int width, int height, bool syncronous = false );
        void requestTextPage( KPDFPage * page );

        // [INHERITED] reparse configuration
        bool reparseConfig();

        // used by the KPDFOutputDev child
        KPDFLinkGoto::Viewport decodeLinkViewport( class GString * namedDest, class LinkDest * dest );

    private:
        // private functions for accessing document informations via PDFDoc
        QString getDocumentInfo( const QString & data ) const;
        QString getDocumentDate( const QString & data ) const;
        // private function for creating the document synopsis hieracy
        void addSynopsisChildren( QDomNode * parent, GList * items );

        // private classes
        QMutex docLock;
        PDFDoc * pdfdoc;
        KPDFOutputDev * kpdfOutputDev;
        QColor paperColor;
        bool docInfoDirty;
        DocumentInfo docInfo;
        bool docSynopsisDirty;
        DocumentSynopsis docSyn;
};

/*
#ifndef THUMBNAILGENERATOR_H
#define THUMBNAILGENERATOR_H

#include <qthread.h> 

class QMutex;

class ThumbnailGenerator : public QThread
{
    public:
        ThumbnailGenerator(PDFDoc *doc, QMutex *docMutex, int page, double ppp, QObject *o);
        
        int getPage() const;

    protected:
        void run();
    
    private:
        PDFDoc *m_doc;
        QMutex *m_docMutex;
        int m_page;
        QObject *m_o;
        double m_ppp;
};

#endif
*/

#endif
