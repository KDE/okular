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

class KOOOGenerator : public Okular::Generator
{
  Q_OBJECT

  public:
    KOOOGenerator( Okular::Document * document );
    virtual ~KOOOGenerator();

    // [INHERITED] load a document and fill up the pagesVector
    bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
    bool closeDocument();

    // [INHERITED] perform actions on document / pages
    bool canGeneratePixmap( bool async );
    void generatePixmap( Okular::PixmapRequest * request );

    // [INHERITED] rotation capability
    bool supportsRotation() { return true; };

    // [INHERITED] print document using already configured kprinter
    bool print( KPrinter& printer );

    const Okular::DocumentInfo* generateDocumentInfo();
    const Okular::DocumentSynopsis* generateDocumentSynopsis();

  private:
    QTextDocument *mDocument;
    Okular::DocumentInfo mDocumentInfo;
    Okular::DocumentSynopsis mDocumentSynopsis;
    OOO::Converter::LinkInfo::List mLinks;
};

#endif
