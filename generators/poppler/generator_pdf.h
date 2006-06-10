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

#define UNSTABLE_POPPLER_QT4

#include <poppler-qt4.h>

#include <qmutex.h>
#include <qcolor.h>
#include <qstring.h>
#include <qthread.h>
#include "core/generator.h"
#include "core/document.h"
#include "core/link.h"
#include "core/textpage.h"

class ObjectRect;
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
 * Background asynchronous contents providing is done via a QThread inherited
 * class defined at the bottom of the file.
 */
class PDFGenerator : public Generator
{
    Q_OBJECT
    public:
        PDFGenerator( KPDFDocument * document );
        virtual ~PDFGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector<KPDFPage*> & pagesVector );
        void loadPages(QVector<KPDFPage*> &pagesVector, int rotation=-1, bool clear=false);
        // [INHERITED] document information
        const DocumentInfo * generateDocumentInfo();
        const DocumentSynopsis * generateDocumentSynopsis();
        const DocumentFonts * generateDocumentFonts();
        const QList<EmbeddedFile*> * embeddedFiles();

        // [INHERITED] document information
        bool isAllowed( int permissions );

        // [INHERITED] perform actions on document / pages
        bool canGeneratePixmap( bool async );
        void generatePixmap( PixmapRequest * request );
        bool canGenerateTextPage();
        void generateSyncTextPage( KPDFPage * page );

        // bah
        QString getXMLFile() { return QString::null; };
        void setupGUI(KActionCollection  * /*ac*/ , QToolBox * /* tBox */) { ; };

        bool supportsSearching() { return true; };
        bool supportsRotation() { return true; };
        bool prefersInternalSearching() { return false; };

        RegularAreaRect * findText (const QString & text, SearchDir dir, 
          const bool strictCase, const RegularAreaRect * lastRect, 
          KPDFPage * page );
        QString getText( const RegularAreaRect * area, KPDFPage * page );

        void setOrientation(QVector<KPDFPage*> & pagesVector, int orientation);

        // [INHERITED] print page using an already configured kprinter
        bool print( KPrinter& printer );

        // [INHERITED] reply to some metadata requests
        QString getMetaData( const QString & key, const QString & option );

        // [INHERITED] reparse configuration
        bool reparseConfig();

    private slots:
        // (async related) receive data from the generator thread
        void threadFinished();

    private:
        // friend class to access private document related variables
        friend class PDFPixmapGeneratorThread;
        
        // create the document synopsis hieracy
        void addSynopsisChildren( QDomNode * parentSource, QDomNode * parentDestination );
        // fetch annotations from the pdf file and add they to the page
        void addAnnotations( Poppler::Page * popplerPage, KPDFPage * page );
        // fetch the transition information and add it to the page
        void addTransition( Poppler::Page * popplerPage, KPDFPage * page );
        
        KPDFTextPage * abstractTextPage(const QList<Poppler::TextBox*> &text, double height, double width, int rot);
        
        // poppler dependant stuff
        QMutex docLock;
        Poppler::Document *pdfdoc;

        // asynchronous generation related stuff
        PDFPixmapGeneratorThread * generatorThread;

        // misc variables for document info and synopsis caching
        bool ready;
        PixmapRequest * pixmapRequest;
        bool docInfoDirty;
        DocumentInfo docInfo;
        bool docSynopsisDirty;
        DocumentSynopsis docSyn;
        bool docFontsDirty;
        DocumentFonts docFonts;
        bool docEmbeddedFilesDirty;
        QList<EmbeddedFile*> docEmbeddedFiles;
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

        PixmapRequest *request() const;

        // methods for getting contents from the GUI thread
        QImage * takeImage() const;
        QList<Poppler::TextBox*> takeText();
        QLinkedList< ObjectRect * > takeObjectRects() const;

    private:
        // can't be called from the outside (but from startGeneration)
        void run();

        class PPGThreadPrivate * d;
};

#endif
