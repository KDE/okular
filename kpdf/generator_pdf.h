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
#include <qthread.h>
#include "generator.h"
#include "document.h"
#include "link.h"

class PDFDoc;
class GList;
class KPDFOutputDev;
class PDFGeneratorThread;

/**
 * @short A generator that builds contents from a PDF document.
 *
 * All Generator features are supported and implented by this one.
 * Internally this holds a reference to xpdf's core objects and provides
 * contents generation using the PDFDoc object and a couple of OutputDevices
 * called KPDFOutputDev and KPDFTextDev (both defined in QOutputDev.h).
 *
 * For generating page contents we tell PDFDoc to render a page and grab
 * contents from out OutputDevs when rendering finishes.
 *
 * Background asyncronous contents providing is done via a QThread inherited
 * class defined at the bottom of the file.
 */
class PDFGenerator : public Generator
{
    public:
        PDFGenerator();
        virtual ~PDFGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QValueVector<KPDFPage*> & pagesVector );

        // [INHERITED] document informations
        const DocumentInfo * documentInfo();
        const DocumentSynopsis * documentSynopsis();

        // [INHERITED] perform actions on document / pages
        bool print( KPrinter& printer );
        void requestPixmap( PixmapRequest * request, bool syncronous = false );
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

        // xpdf dependant stuff
        QMutex docLock;
        PDFDoc * pdfdoc;
        KPDFOutputDev * kpdfOutputDev;
        QColor paperColor;

        // asyncronous generation related things
        PDFGeneratorThread * generatorThread;
        PixmapRequest * requestOnThread;
        QValueList< PixmapRequest * > requestsQueue;

        // misc variables for document info and synopsis caching
        bool docInfoDirty;
        DocumentInfo docInfo;
        bool docSynopsisDirty;
        DocumentSynopsis docSyn;
};


/**
 * @short A thread that builds contents for PDFGenerator in the background.
 *
 * 
 */
class PDFGeneratorThread : public QThread
{
/*
    public:
        PDFGeneratorThread(PDFDoc *doc, QMutex *docMutex, int page, double ppp, QObject *o);
        int getPage() const;

    protected:
        void run();

    private:
        PDFDoc *m_doc;
        QMutex *m_docMutex;
        int m_page;
        QObject *m_o;
        double m_ppp;
*/
};

#endif
