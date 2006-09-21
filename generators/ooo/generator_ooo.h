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

#include "core/generator.h"
#include "converter.h"

class QTextDocument;

class KOOOGenerator : public Generator
{
  Q_OBJECT

  public:
    KOOOGenerator( KPDFDocument * document );
    virtual ~KOOOGenerator();

    // [INHERITED] load a document and fill up the pagesVector
    bool loadDocument( const QString & fileName, QVector<KPDFPage*> & pagesVector );
    bool closeDocument();

    // [INHERITED] perform actions on document / pages
    bool canGeneratePixmap( bool async );
    void generatePixmap( PixmapRequest * request );

    // [INHERITED] rotation capability
    bool supportsRotation() { return true; };
    void setOrientation( QVector<KPDFPage*> & pagesVector, int orientation );

    // [INHERITED] print document using already configured kprinter
    bool print( KPrinter& printer );

    const DocumentInfo* generateDocumentInfo();
    const DocumentSynopsis* generateDocumentSynopsis();

  private:
    QTextDocument *mDocument;
    DocumentInfo mDocumentInfo;
    DocumentSynopsis mDocumentSynopsis;
    OOO::Converter::LinkInfo::List mLinks;
};

#endif
