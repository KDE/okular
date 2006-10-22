/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PDF_H_
#define _OKULAR_GENERATOR_PDF_H_

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

namespace Okular {
class ObjectRect;
}

class PDFPixmapGeneratorThread;

/**
 * @short A generator that builds contents from a PDF document.
 *
 * All Generator features are supported and implented by this one.
 * Internally this holds a reference to xpdf's core objects and provides
 * contents generation using the PDFDoc object and a couple of OutputDevices
 * called Okular::OutputDev and Okular::TextDev (both defined in gp_outputdev.h).
 *
 * For generating page contents we tell PDFDoc to render a page and grab
 * contents from out OutputDevs when rendering finishes.
 *
 * Background asynchronous contents providing is done via a QThread inherited
 * class defined at the bottom of the file.
 */
class PDFGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        PDFGenerator();
        virtual ~PDFGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool closeDocument();
        void loadPages(QVector<Okular::Page*> &pagesVector, int rotation=-1, bool clear=false);
        // [INHERITED] document information
        const Okular::DocumentInfo * generateDocumentInfo();
        const Okular::DocumentSynopsis * generateDocumentSynopsis();
        const Okular::DocumentFonts * generateDocumentFonts();
        const QList<Okular::EmbeddedFile*> * embeddedFiles() const;
        PageSizeMetric pagesSizeMetric() const { return Points; }

        // [INHERITED] document information
        bool isAllowed( int permissions ) const;

        // [INHERITED] perform actions on document / pages
        bool canGeneratePixmap( bool async ) const;
        void generatePixmap( Okular::PixmapRequest * request );
        bool canGenerateTextPage() const;
        void generateSyncTextPage( Okular::Page * page );

        bool supportsSearching() const { return true; };
        bool supportsRotation() const { return true; };
        bool prefersInternalSearching() const { return false; };

        Okular::RegularAreaRect * findText (const QString & text, Okular::SearchDir dir, 
          const bool strictCase, const Okular::RegularAreaRect * lastRect, 
          Okular::Page * page ) const;
        QString getText( const Okular::RegularAreaRect * area, Okular::Page * page ) const;

        // [INHERITED] print page using an already configured kprinter
        bool print( KPrinter& printer );

        // [INHERITED] reply to some metadata requests
        QString metaData( const QString & key, const QString & option ) const;

        // [INHERITED] reparse configuration
        bool reparseConfig();

        // [INHERITED] text exporting
        bool canExportToText() const { return true; };
        bool exportToText( const QString & fileName );

    private slots:
        // (async related) receive data from the generator thread
        void threadFinished();

    private:
        // friend class to access private document related variables
        friend class PDFPixmapGeneratorThread;
        
        // create the document synopsis hieracy
        void addSynopsisChildren( QDomNode * parentSource, QDomNode * parentDestination );
        // fetch annotations from the pdf file and add they to the page
        void addAnnotations( Poppler::Page * popplerPage, Okular::Page * page );
        // fetch the transition information and add it to the page
        void addTransition( Poppler::Page * popplerPage, Okular::Page * page );
        
        Okular::TextPage * abstractTextPage(const QList<Poppler::TextBox*> &text, double height, double width, int rot);
        
        // poppler dependant stuff
        mutable QMutex docLock;
        Poppler::Document *pdfdoc;

        // asynchronous generation related stuff
        PDFPixmapGeneratorThread * generatorThread;

        // misc variables for document info and synopsis caching
        bool ready;
        Okular::PixmapRequest * pixmapRequest;
        bool docInfoDirty;
        Okular::DocumentInfo docInfo;
        bool docSynopsisDirty;
        Okular::DocumentSynopsis docSyn;
        bool docFontsDirty;
        Okular::DocumentFonts docFonts;
        mutable bool docEmbeddedFilesDirty;
        mutable QList<Okular::EmbeddedFile*> docEmbeddedFiles;
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
        void startGeneration( Okular::PixmapRequest * request );
        // end generation
        void endGeneration();

        Okular::PixmapRequest *request() const;

        // methods for getting contents from the GUI thread
        QImage * takeImage() const;
        QList<Poppler::TextBox*> takeText();
        QLinkedList< Okular::ObjectRect * > takeObjectRects() const;

    private:
        // can't be called from the outside (but from startGeneration)
        void run();

        class PPGThreadPrivate * d;
};

#endif
