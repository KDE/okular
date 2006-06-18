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

class DjVuGenerator : public Generator
{
    Q_OBJECT
    public:
        DjVuGenerator( KPDFDocument * doc );
        bool loadDocument( const QString & fileName, QVector<KPDFPage*> & pagesVector );

        // pixmap generation
        bool canGeneratePixmap( bool async );
        void generatePixmap( PixmapRequest * request );

        // document information
        const DocumentInfo * generateDocumentInfo();

        // rotation handling
        bool supportsRotation() { return true; };
        void setOrientation( QVector<KPDFPage*> & pagesVector, int orientation );

    private slots:
        void djvuPixmapGenerated( int page, const QPixmap & pix );

    private:
        void loadPages( QVector<KPDFPage*> & pagesVector, int rotation );

        KDjVu *m_djvu;

        DocumentInfo *m_docInfo;
        bool ready;

        PixmapRequest *m_request;
};

#endif
