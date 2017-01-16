/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _GENERATOR_DJVU_H_
#define _GENERATOR_DJVU_H_

#include <core/generator.h>

#include <qvector.h>

#include "kdjvu.h"

namespace Okular {
class Annotation;
class ObjectRect;
}

class DjVuGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES( Okular::Generator )
    public:
        DjVuGenerator( QObject *parent, const QVariantList &args );
        ~DjVuGenerator();
        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector ) Q_DECL_OVERRIDE;

        // document information
        Okular::DocumentInfo generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const Q_DECL_OVERRIDE;
        const Okular::DocumentSynopsis * generateDocumentSynopsis() Q_DECL_OVERRIDE;

        // printing
        bool print( QPrinter& printer ) Q_DECL_OVERRIDE;

        QVariant metaData( const QString & key, const QVariant & option ) const Q_DECL_OVERRIDE;

    protected:
        bool doCloseDocument() Q_DECL_OVERRIDE;
        // pixmap generation
        QImage image( Okular::PixmapRequest *request ) Q_DECL_OVERRIDE;
        Okular::TextPage* textPage( Okular::Page *page ) Q_DECL_OVERRIDE;

    private:
        void loadPages( QVector<Okular::Page*> & pagesVector, int rotation );
        Okular::ObjectRect* convertKDjVuLink( int page, KDjVu::Link * link ) const;
        Okular::Annotation* convertKDjVuAnnotation( int w, int h, KDjVu::Annotation * ann ) const;

        KDjVu *m_djvu;

        Okular::DocumentSynopsis *m_docSyn;
};

#endif
