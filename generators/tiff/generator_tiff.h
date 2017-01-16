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

#include <core/generator.h>

#include <QtCore/qloggingcategory.h>
#include <qhash.h>

class TIFFGenerator : public Okular::Generator
{
    Q_OBJECT
    Q_INTERFACES( Okular::Generator )
    public:
        TIFFGenerator( QObject *parent, const QVariantList &args );
        virtual ~TIFFGenerator();

        bool loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector ) Q_DECL_OVERRIDE;
        bool loadDocumentFromData( const QByteArray & fileData, QVector< Okular::Page * > & pagesVector ) Q_DECL_OVERRIDE;

        Okular::DocumentInfo generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const Q_DECL_OVERRIDE;

        bool print( QPrinter& printer ) Q_DECL_OVERRIDE;

    protected:
        bool doCloseDocument() Q_DECL_OVERRIDE;
        QImage image( Okular::PixmapRequest * request ) Q_DECL_OVERRIDE;

    private:
        class Private;
        Private * const d;

        bool loadTiff( QVector< Okular::Page * > & pagesVector, const char *name );
        void loadPages( QVector<Okular::Page*> & pagesVector );
        int mapPage( int page ) const;

        QHash< int, int > m_pageMapping;
};

Q_DECLARE_LOGGING_CATEGORY(OkularTiffDebug)

#endif
