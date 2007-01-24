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

#include <okular/core/threadedgenerator.h>

class TIFFGenerator : public Okular::ThreadedGenerator
{
    Q_OBJECT
    public:
        TIFFGenerator();
        virtual ~TIFFGenerator();

        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool closeDocument();

        const Okular::DocumentInfo * generateDocumentInfo();

        bool print( KPrinter& printer );

    protected:
        QImage image( Okular::PixmapRequest * request );

    private:
        class Private;
        Private * const d;

        void loadPages( QVector<Okular::Page*> & pagesVector );

        Okular::DocumentInfo * m_docInfo;
};

#endif
