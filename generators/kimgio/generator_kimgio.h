/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_KIMGIO_H_
#define _OKULAR_GENERATOR_KIMGIO_H_

#include "core/generator.h"

class KIMGIOGenerator : public Generator
{
    Q_OBJECT
    public:
        KIMGIOGenerator( KPDFDocument * document );
        virtual ~KIMGIOGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector<KPDFPage*> & pagesVector );

        // [INHERITED] perform actions on document / pages
        bool canGeneratePixmap( bool async );
        void generatePixmap( PixmapRequest * request );

//        bool hasFonts() const;

        // [INHERITED] rotation capability
        bool supportsRotation() { return true; };
        void setOrientation(QVector<KPDFPage*> & pagesVector, int orientation);

        // [INHERITED] print document using already configured kprinter
        bool print( KPrinter& printer );

    private:
        QPixmap *m_pix;
};

#endif
