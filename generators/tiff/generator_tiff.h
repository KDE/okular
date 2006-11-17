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

class TIFFGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        TIFFGenerator();
        virtual ~TIFFGenerator();

        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool closeDocument();

        bool canGeneratePixmap( bool async ) const;
        void generatePixmap( Okular::PixmapRequest * request );

        const Okular::DocumentInfo * generateDocumentInfo();

        bool supportsRotation() const { return true; };

        bool print( KPrinter& printer );

    private slots:
        void slotThreadFinished();

    private:
        class Private;
        Private * const d;

        TIFFGeneratorThread* thread;

        void loadPages( QVector<Okular::Page*> & pagesVector, int rotation );

        bool ready;
        Okular::DocumentInfo * m_docInfo;
};

#endif
