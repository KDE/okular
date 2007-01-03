/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_OOO_H_
#define _OKULAR_GENERATOR_OOO_H_

#include <okular/core/document.h>
#include <okular/core/generator.h>

#include "converter.h"

class QTextDocument;

class KOOOGenerator : public Okular::Generator
{
  Q_OBJECT

  public:
    KOOOGenerator();
    virtual ~KOOOGenerator();

    // [INHERITED] load a document and fill up the pagesVector
    bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
    bool closeDocument();

    // [INHERITED] perform actions on document / pages
    bool canGeneratePixmap( bool async ) const;
    void generatePixmap( Okular::PixmapRequest * request );

    // [INHERITED] print document using already configured kprinter
    bool print( KPrinter& printer );

    // [INHERITED] text exporting
    Okular::ExportFormat::List exportFormats() const;
    bool exportTo( const QString &fileName, const Okular::ExportFormat &format );

    const Okular::DocumentInfo* generateDocumentInfo();
    const Okular::DocumentSynopsis* generateDocumentSynopsis();

  private:
    QTextDocument *mDocument;
    Okular::DocumentInfo mDocumentInfo;
    Okular::DocumentSynopsis mDocumentSynopsis;
    OOO::Converter::LinkInfo::List mLinks;
    OOO::Converter::AnnotationInfo::List mAnnotations;
};

#endif
