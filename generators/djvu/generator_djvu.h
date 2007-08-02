/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _GENERATOR_DJVU_H_
#define _GENERATOR_DJVU_H_

#include <okular/core/generator.h>

#include <qvector.h>

#include "kdjvu.h"

namespace Okular {
class Annotation;
class ObjectRect;
}

class DjVuGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        DjVuGenerator();
        ~DjVuGenerator();
        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool closeDocument();

        // pixmap generation
        bool canGeneratePixmap() const;
        void generatePixmap( Okular::PixmapRequest * request );

        // document information
        const Okular::DocumentInfo * generateDocumentInfo();
        const Okular::DocumentSynopsis * generateDocumentSynopsis();

        // printing
        bool print( KPrinter& printer );

    protected:
        Okular::TextPage* textPage( Okular::Page *page );

    private slots:
        void djvuImageGenerated( int page, const QImage & img );

    private:
        void loadPages( QVector<Okular::Page*> & pagesVector, int rotation );
        Okular::ObjectRect* convertKDjVuLink( int page, KDjVu::Link * link ) const;
        Okular::Annotation* convertKDjVuAnnotation( int w, int h, KDjVu::Annotation * ann ) const;

        KDjVu *m_djvu;

        Okular::DocumentInfo *m_docInfo;
        Okular::DocumentSynopsis *m_docSyn;
        bool ready;

        Okular::PixmapRequest *m_request;
};

#endif
