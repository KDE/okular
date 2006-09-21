/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _DJVU_GENERATOR_H_
#define _DJVU_GENERATOR_H_

#include "core/generator.h"
#include <qvector.h>

class KDjVu;

class DjVuGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        DjVuGenerator( Okular::Document * doc );
        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool closeDocument();

        // pixmap generation
        bool canGeneratePixmap( bool async );
        void generatePixmap( Okular::PixmapRequest * request );

        // document information
        const Okular::DocumentInfo * generateDocumentInfo();
        const Okular::DocumentSynopsis * generateDocumentSynopsis();

        // rotation handling
        bool supportsRotation() { return true; };
        void setOrientation( QVector<Okular::Page*> & pagesVector, int orientation );

    private slots:
        void djvuPixmapGenerated( int page, const QPixmap & pix );

    private:
        void loadPages( QVector<Okular::Page*> & pagesVector, int rotation );

        KDjVu *m_djvu;

        Okular::DocumentInfo *m_docInfo;
        Okular::DocumentSynopsis *m_docSyn;
        bool ready;

        Okular::PixmapRequest *m_request;
};

#endif
