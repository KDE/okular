/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_TIFF_H_
#define _OKULAR_GENERATOR_TIFF_H_

#include "core/generator.h"

class TIFFGeneratorThread;

class TIFFGenerator : public Generator
{
    Q_OBJECT
    public:
        TIFFGenerator( KPDFDocument * document );
        virtual ~TIFFGenerator();

        bool loadDocument( const QString & fileName, QVector<KPDFPage*> & pagesVector );

        bool canGeneratePixmap( bool async );
        void generatePixmap( PixmapRequest * request );

        const DocumentInfo * generateDocumentInfo();

        bool supportsRotation() { return true; };
        void setOrientation( QVector<KPDFPage*> & pagesVector, int orientation );

    private slots:
        void slotThreadFinished();

    private:
        class Private;
        Private * const d;

        TIFFGeneratorThread* thread;

        void loadPages( QVector<KPDFPage*> & pagesVector, int rotation );

        bool ready;
        DocumentInfo * m_docInfo;
};

#endif
