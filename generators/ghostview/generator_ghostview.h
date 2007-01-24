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

#include <okular/core/generator.h>
#include <okular/interfaces/guiinterface.h>
#include <okular/interfaces/configinterface.h>

class GSInterpreterCMD;
class GSLogWindow;
class GSInterpreterLib;
class GSInternalDocument;
class KTempFile;
class KActionCollection;

class GSGenerator : public Okular::Generator, public Okular::ConfigInterface, public Okular::GuiInterface
{
    Q_OBJECT
    Q_INTERFACES( Okular::ConfigInterface )
    Q_INTERFACES( Okular::GuiInterface )

    public:
        /** virtual methods to reimplement **/
        // load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector );
        bool closeDocument();

        // Document description and Table of contents
        const Okular::DocumentInfo * generateDocumentInfo();
        const Okular::DocumentSynopsis * generateDocumentSynopsis() { return 0L; }
        const Okular::DocumentFonts * generateDocumentFonts() { return 0L; }

        // page size management
        Okular::PageSize::List pageSizes() const;
        void pageSizeChanged( const Okular::PageSize &, const Okular::PageSize & );

        QString xmlFile() const;
        void setupGui( KActionCollection *, QToolBox * );
        void freeGui();

        // print document using already configured kprinter
        bool print( KPrinter& /*printer*/ );
        QString fileName() const;

        bool hasFeature( GeneratorFeature feature ) const;

        bool reparseConfig();
        void addPages( KConfigDialog* dlg );

        /** constructor **/
        GSGenerator();
        ~GSGenerator();

    public slots:
        void slotPixmapGenerated(const QImage* img);
        void slotAsyncPixmapGenerated(QPixmap * img);

    protected:
        // page contents generation
        bool canGeneratePixmap() const;
        void generatePixmap( Okular::PixmapRequest * request ) ;

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
        int rotation( CDSC_ORIENTATION_ENUM orientation ) const;
        int angle( CDSC_ORIENTATION_ENUM orientation ) const;
        CDSC_ORIENTATION_ENUM orientation( int rot ) const;
        mutable QMutex docLock;
        mutable QMutex syncLock;
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

        Okular::PageSize::List m_pageSizes;
};

#endif
