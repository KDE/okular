/***************************************************************************
 *   Copyright (C) 2008 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_GENERATOR_FAX_H
#define OKULAR_GENERATOR_FAX_H

#include <core/generator.h>

#include <QtGui/QImage>

#include "faxdocument.h"

class FaxGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES( Okular::Generator )

    public:
        FaxGenerator( QObject *parent, const QVariantList &args );
        virtual ~FaxGenerator();

        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector );

        Okular::DocumentInfo generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const;

        bool print( QPrinter& printer );

    protected:
        bool doCloseDocument();
        QImage image( Okular::PixmapRequest * request );

    private:
        QImage m_img;
        FaxDocument::DocumentType m_type;
};

#endif
