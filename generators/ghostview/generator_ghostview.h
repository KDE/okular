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

class GSLogWindow;
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

        // Document description and Table of contents
        const Okular::DocumentInfo * generateDocumentInfo();
        const Okular::DocumentSynopsis * generateDocumentSynopsis() { return 0L; }
        const Okular::DocumentFonts * generateDocumentFonts() { return 0L; }

        // page contents generation
        bool canGeneratePixmap() const;
        void generatePixmap( Okular::PixmapRequest * request );

        // page size management
        Okular::PageSize::List pageSizes() const;
        void pageSizeChanged( const Okular::PageSize &, const Okular::PageSize & );

        QString xmlFile() const;
        void setupGui( KActionCollection *, QToolBox * );
        void freeGui();

        // print document using already configured kprinter
        bool print( KPrinter& /*printer*/ );
        QString fileName() const;

        bool reparseConfig();
        void addPages( KConfigDialog* dlg );

        /** constructor **/
        GSGenerator();
        ~GSGenerator();

    public slots:
        void slotImageGenerated(QImage *img, Okular::PixmapRequest *request);

    protected:
        bool doCloseDocument();

    private:
        // conversion handling
        bool m_converted;
        KTempFile * dscForPDF;

        bool loadDocumentWithDSC( const QString & name, QVector< Okular::Page * > & pagesVector , bool ps );
        bool loadPages( QVector< Okular::Page * > & pagesVector );
        int rotation( CDSC_ORIENTATION_ENUM orientation ) const;
        int angle( CDSC_ORIENTATION_ENUM orientation ) const;
        CDSC_ORIENTATION_ENUM orientation( int rot ) const;

        // backendish stuff
        GSInternalDocument* internalDoc;

        Okular::PixmapRequest *m_request;

        // gui stuff
        GSLogWindow * m_logWindow ;
        KActionCollection* m_actionCollection;
        QToolBox * m_box;

        Okular::PageSize::List m_pageSizes;
};

#endif
