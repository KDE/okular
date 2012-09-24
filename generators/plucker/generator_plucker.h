/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_PLUCKER_H_
#define _OKULAR_GENERATOR_PLUCKER_H_

#include <core/document.h>
#include <core/generator.h>

#include <QtGui/QTextBlock>

#include "qunpluck.h"

class QTextDocument;

class PluckerGenerator : public Okular::Generator
{
    Q_OBJECT

    public:
        PluckerGenerator( QObject *parent, const QVariantList &args );
        virtual ~PluckerGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );

        // [INHERITED] document information
        const Okular::DocumentInfo * generateDocumentInfo();
        
        // [INHERITED] perform actions on document / pages
        QImage image( Okular::PixmapRequest *request );

        // [INHERITED] text exporting
        Okular::ExportFormat::List exportFormats() const;
        bool exportTo( const QString &fileName, const Okular::ExportFormat &format );

        // [INHERITED] print document using already configured kprinter
        bool print( QPrinter& printer );

    protected:
        bool doCloseDocument();

    private:
      QList<QTextDocument*> mPages;
      QSet<int> mLinkAdded;
      Link::List mLinks;
      Okular::DocumentInfo mDocumentInfo;
};

#endif
