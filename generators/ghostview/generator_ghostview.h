/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_GHOSTVIEW_H_
#define _OKULAR_GENERATOR_GHOSTVIEW_H_

#include "core/generator.h"

class GSInterpreterCMD;
class GSLogWindow;
class GSInterpreterLib;
class GSInternalDocument;
class KTempFile;
class KActionCollection;

class GSGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        /** virtual methods to reimplement **/
        // load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector );
        bool closeDocument();

        // Document description and Table of contents
        const Okular::DocumentInfo * generateDocumentInfo();
        const Okular::DocumentSynopsis * generateDocumentSynopsis() { return 0L; }
        const Okular::DocumentFonts * generateDocumentFonts() { return 0L; }

        // page contents generation
        bool canGeneratePixmap( bool async ) ;
        void generatePixmap( Okular::PixmapRequest * request ) ;

        bool supportsRotation() { return true; } ;
        void setOrientation(QVector<Okular::Page*>&, int);

        // paper size management
        bool supportsPaperSizes();
        QStringList paperSizes();
        void setPaperSize( QVector<Okular::Page*> & pagesVector, int newsize );

        QString getXMLFile() { return QString::null; };
        void setupGUI(KActionCollection  * /*ac*/ , QToolBox * /* tBox */) ;
        void freeGUI() ;

    	// may come useful later
        //virtual bool hasFonts() const ;

        // print document using already configured kprinter
        bool print( KPrinter& /*printer*/ );
        // access meta data of the generator
        QString getMetaData( const QString &/*key*/, const QString &/*option*/ ) { return QString(); }
        // tell generator to re-parse configuration and return true if something changed
        bool reparseConfig() { return false; }
        QString fileName();

        void addPages( KConfigDialog* dlg );
        /** constructor: takes the Document as a parameter **/
        GSGenerator( Okular::Document * doc );
        ~GSGenerator();

    public slots:
        void slotPixmapGenerated(const QImage* img);
        void slotAsyncPixmapGenerated(QPixmap * img);

    private:
        // conversion handling
        bool m_converted;
        KTempFile * dscForPDF;
        QMutex convertLock;
        GSInterpreterLib* m_convert;
//         QVector<Okular::Page*> m_pages;

        bool loadDocumentWithDSC( const QString & name, QVector< Okular::Page * > & pagesVector , bool ps );
        bool loadPages( QVector< Okular::Page * > & pagesVector );
        bool initInterpreter();
        int rotation( CDSC_ORIENTATION_ENUM orientation );
        int angle( CDSC_ORIENTATION_ENUM orientation );
        CDSC_ORIENTATION_ENUM orientation( int rot );
        QMutex docLock;
        QMutex syncLock;
        bool m_asyncBusy;

        // pixmap requests
        Okular::PixmapRequest* m_asRequest;
        Okular::PixmapRequest* m_sRequest;

        // backendish stuff
        GSInterpreterLib* pixGenerator;
        GSInterpreterCMD* asyncGenerator;
        GSInternalDocument* internalDoc;

        // gui stuff
        GSLogWindow * m_logWindow ;
        KActionCollection* m_actionCollection;
        QToolBox * m_box;
};

#endif
