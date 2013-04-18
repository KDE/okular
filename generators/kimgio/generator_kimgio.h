/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <aacid@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_GENERATOR_KIMGIO_H_
#define _OKULAR_GENERATOR_KIMGIO_H_

#include <core/generator.h>
#include <core/document.h>

#include <QtGui/QImage>

class KIMGIOGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        KIMGIOGenerator( QObject *parent, const QVariantList &args );
        virtual ~KIMGIOGenerator();

        // [INHERITED] load a document and fill up the pagesVector
        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );
        bool loadDocumentFromData( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector );

        // [INHERITED] print document using already configured kprinter
        bool print( QPrinter& printer );

        // [INHERITED] document information
        const Okular::DocumentInfo * generateDocumentInfo();

    protected:
        bool doCloseDocument();
        QImage image( Okular::PixmapRequest * request );

    private slots:
        void slotTest();

    private:
        QImage m_img;
        Okular::DocumentInfo docInfo;
};

#endif
