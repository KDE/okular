/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef GENERATOR_COMICBOOK_H
#define GENERATOR_COMICBOOK_H

#include <okular/core/generator.h>

#include "document.h"

class GeneratorThread;

class ComicBookGenerator : public Okular::Generator
{
    Q_OBJECT

    public:
        ComicBookGenerator();
        virtual ~ComicBookGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool closeDocument();

        // [INHERITED] perform actions on document / pages
        bool canGeneratePixmap( bool async ) const;
        void generatePixmap( Okular::PixmapRequest * request );

        // [INHERITED] print document using already configured kprinter
        bool print( KPrinter& printer );

        bool hasFeature( GeneratorFeature feature ) const;

    private Q_SLOTS:
        void threadFinished();

    private:
      ComicBook::Document mDocument;
      GeneratorThread *mThread;
      bool mReady;
};

#endif
