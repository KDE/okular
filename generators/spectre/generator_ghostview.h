/***************************************************************************
 *   Copyright (C) 2007 by Albert Astals Cid <aacid@kde.org>               *
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

#include <libspectre/spectre.h>

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

        // print document using already configured kprinter
        bool print( QPrinter& /*printer*/ );
        QString fileName() const;

        bool reparseConfig();
        void addPages( KConfigDialog* dlg );

        /** constructor **/
        GSGenerator( QObject *parent, const QVariantList &args );
        ~GSGenerator();

    public slots:
        void slotImageGenerated(QImage *img, Okular::PixmapRequest *request);

    protected:
        bool doCloseDocument();

    private:
        bool loadPages( QVector< Okular::Page * > & pagesVector );
        Okular::Rotation orientation(SpectreOrientation orientation) const;

        // backendish stuff
        SpectreDocument *m_internalDocument;
        Okular::DocumentInfo *m_docInfo;

        Okular::PixmapRequest *m_request;
};

#endif
