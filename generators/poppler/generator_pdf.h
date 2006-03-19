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
#include "core/textpage.h"

class PDFDoc;
class GooList;
class TextPage;
class LinkDest;
class Page;
class Dict;
class Ref;

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
    Q_OBJECT
    public:
        PDFGenerator( KPDFDocument * document );
        virtual ~PDFGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QValueVector<KPDFPage*> & pagesVector );
        void loadPages(QValueVector<KPDFPage*> &pagesVector, int rotation=-1, bool clear=false);
        // [INHERITED] document informations
        const DocumentInfo * generateDocumentInfo();
        const DocumentSynopsis * generateDocumentSynopsis();
        const DocumentFonts * generateDocumentFonts();

        // [INHERITED] document informations
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
        QString * getText( const RegularAreaRect * area, KPDFPage * page );

        void setOrientation(QValueVector<KPDFPage*> & pagesVector, int orientation);

        // [INHERITED] print page using an already configured kprinter
        bool print( KPrinter& printer );

        // [INHERITED] reply to some metadata requests
        QString getMetaData( const QString & key, const QString & option );

        // [INHERITED] reparse configuration
        bool reparseConfig();
    signals:
        void error(QString & string, int duration);
        void warning(QString & string, int duration);
        void notice(QString & string, int duration);

    private:
        // friend class to access private document related variables
        friend class PDFPixmapGeneratorThread;
        void fillViewportFromLink( DocumentViewport &viewport, LinkDest *destination );

        // access document informations
        QString getDocumentInfo( const QString & data ) const;
        QString getDocumentDate( const QString & data ) const;
        // create the document synopsis hieracy
        void addSynopsisChildren( QDomNode * parent, GooList * items );
        // add fonts (in resDict) to the private 'docFonts' class
        void addFonts( Dict * resDict, Ref ** fonts, int &fontsLen, int &fontsSize );
        // fetch annotations from the pdf file and add they to the page
        void addAnnotations( Page * xpdfPage, KPDFPage * page );
        // fetch the transition information and add it to the page
        void addTransition( Page * xpdfPage, KPDFPage * page );
        
        static KPDFTextPage * abstractTextPage(TextPage *tp, double height, double width, int rot);
        TextPage * fastTextPage (KPDFPage * page);

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
        bool docFontsDirty;
        DocumentFonts docFonts;
	
	// static instances counter
	static unsigned int m_count;
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
