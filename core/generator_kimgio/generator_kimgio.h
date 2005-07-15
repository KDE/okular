/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_GENERATOR_PNG_H_
#define _KPDF_GENERATOR_PNG_H_
#include "core/generator.h"


class KIMGIOGenerator : public Generator
{
    public:
        KIMGIOGenerator( KPDFDocument * document );
        virtual ~KIMGIOGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QValueVector<KPDFPage*> & pagesVector );

        // [INHERITED] perform actions on document / pages
        bool canGeneratePixmap();
        void generatePixmap( PixmapRequest * request );
        void generateSyncTextPage( KPDFPage * page ) {;};
	// [INHERITED] capability querying
        bool supportsSearching() const;
        bool hasFonts() const;
        bool canGenerateTextPage() { return false; };
        bool supportsSearching()  { return false; };
        bool prefersInternalSearching()  { return false; };
        RegularAreaRect* findText(const QString&, SearchDir, bool, const RegularAreaRect*, KPDFPage*)
             { return 0; };
        QString * getText(const RegularAreaRect*, KPDFPage*) { return 0; };

        // font related
	
        // [INHERITED] print document using already configured kprinter
        bool print( KPrinter& printer );

    private:
        QPixmap *m_pix;
};

#endif
