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

#include <core/generator.h>
#include <interfaces/guiinterface.h>

#include <QtGui/QImage>

class KIMGIOGenerator : public Okular::Generator, public Okular::GuiInterface
{
    Q_OBJECT
    Q_INTERFACES( Okular::GuiInterface )
    public:
        KIMGIOGenerator();
        virtual ~KIMGIOGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool loadDocumentFromData( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector );

        // [INHERITED] print document using already configured kprinter
        bool print( KPrinter& printer );

    protected:
        bool doCloseDocument();
        QImage image( Okular::PixmapRequest * request );

    private slots:
        void slotTest();

    private:
        QImage m_img;
};

#endif
