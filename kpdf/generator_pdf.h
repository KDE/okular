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
class PDFPixmapGeneratorThread;

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

    protected:

    private:
        // friend class to access private document related variables
        friend class PDFPixmapGeneratorThread;

        // private functions for accessing document informations via PDFDoc
        QString getDocumentInfo( const QString & data ) const;
        QString getDocumentDate( const QString & data ) const;
        // private function for creating the document synopsis hieracy
        void addSynopsisChildren( QDomNode * parent, GList * items );
        // (GT) take the first queued item from the stack and feed it to the thread
        void startNewThreadedGeneration();
        // (GT) receive data from the generator thread
        void customEvent( QCustomEvent * );

        // xpdf dependant stuff
        QMutex docLock;
        PDFDoc * pdfdoc;
        KPDFOutputDev * kpdfOutputDev;
        QColor paperColor;

        // asyncronous generation related things
        PDFPixmapGeneratorThread * generatorThread;
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
class PDFPixmapGeneratorThread : public QThread
{
    public:
        PDFPixmapGeneratorThread( PDFGenerator * generator );
        ~PDFPixmapGeneratorThread();

        // set the request to the thread (it will be reparented)
        void startGeneration( PixmapRequest * request );
        // get a const reference to the currently processed pixmap
        const PixmapRequest * currentRequest() const;
        // return wether we can add a new request or not
        bool isReady() const;
        // end generation
        void endGeneration();

    private:
        // can't be called from the outside (but from startGeneration)
        void run();

        // local members
        PDFGenerator * m_generator;
        PixmapRequest * m_currentRequest;
        bool m_genTextPage;
        bool m_genPageRects;
};

#endif
