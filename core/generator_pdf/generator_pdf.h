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
#include "core/generator.h"
#include "core/document.h"
#include "core/link.h"

class Dict;
class GfxFont;
class LinkDest;
class Ref;
class PDFDoc;
class GList;
class TextPage;

class ObjectRect;
class KPDFOutputDev;
class PDFPixmapGeneratorThread;

/**
 * @short A generator that builds contents from a PDF document.
 *
 * All Generator features are supported and implented by this one.
 * Internally this holds a reference to xpdf's core objects and provides
 * contents generation using the PDFDoc object and a couple of OutputDevices
 * called KPDFOutputDev and KPDFTextDev (both defined in gp_outputdev.h).
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
        PDFGenerator( KPDFDocument * document );
        virtual ~PDFGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QValueVector<KPDFPage*> & pagesVector );

        // [INHERITED] document informations
        const DocumentInfo * generateDocumentInfo();
        const DocumentSynopsis * generateDocumentSynopsis();

        // [INHERITED] document informations
        bool isAllowed( int permissions );

        // [INHERITED] perform actions on document / pages
        bool canGeneratePixmap();
        void generatePixmap( PixmapRequest * request );
        void generateSyncTextPage( KPDFPage * page );

        // [INHERITED] capability querying
        bool supportsSearching() const;
        bool hasFonts() const;

        // [INHERITED] font related
        void putFontInfo(KListView *list);

        // [INHERITED] print page using an already configured kprinter
        bool print( KPrinter& printer );

        // [INHERITED] reply to some metadata requests
        QString getMetaData( const QString & key, const QString & option );

        // [INHERITED] reparse configuration
        bool reparseConfig();

    private:
        // friend class to access private document related variables
        friend class PDFPixmapGeneratorThread;

        void scanFonts(Dict *resDict, KListView *list, Ref **fonts, int &fontsLen, int &fontsSize, QValueVector<Ref> *visitedXObjects);
        void scanFont(GfxFont *font, KListView *list, Ref **fonts, int &fontsLen, int &fontsSize);

        void fillViewportFromLink( DocumentViewport &viewport, LinkDest *destination );

        // private functions for accessing document informations via PDFDoc
        QString getDocumentInfo( const QString & data, bool canReturnNull = false ) const;
        QString getDocumentDate( const QString & data ) const;
        // private function for creating the document synopsis hieracy
        void addSynopsisChildren( QDomNode * parent, GList * items );
        // private function for creating the transition information
        void addTransition( int pageNumber, KPDFPage * page );
        // (async related) receive data from the generator thread
        void customEvent( QCustomEvent * );

        // xpdf dependant stuff
        QMutex docLock;
        PDFDoc * pdfdoc;
        KPDFOutputDev * kpdfOutputDev;
        QColor paperColor;

        // asyncronous generation related stuff
        PDFPixmapGeneratorThread * generatorThread;

        // misc variables for document info and synopsis caching
        bool ready;
        PixmapRequest * pixmapRequest;
        bool docInfoDirty;
        DocumentInfo docInfo;
        bool docSynopsisDirty;
        DocumentSynopsis docSyn;
};


/**
 * @short A thread that builds contents for PDFGenerator in the background.
 *
 */
class PDFPixmapGeneratorThread : public QThread
{
    public:
        PDFPixmapGeneratorThread( PDFGenerator * generator );
        ~PDFPixmapGeneratorThread();

        // set the request to the thread (it will be reparented)
        void startGeneration( PixmapRequest * request );
        // end generation
        void endGeneration();

        // methods for getting contents from the GUI thread
        QImage * takeImage() const;
        TextPage * takeTextPage() const;
        QValueList< ObjectRect * > takeObjectRects() const;

    private:
        // can't be called from the outside (but from startGeneration)
        void run();

        class PPGThreadPrivate * d;
};

#endif
