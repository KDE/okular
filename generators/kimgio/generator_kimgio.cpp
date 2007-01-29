/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QBuffer>
#include <QtGui/QImageReader>
#include <QtGui/QPainter>

#include <klocale.h>
#include <kprinter.h>

#include <okular/core/page.h>

#include "generator_kimgio.h"

OKULAR_EXPORT_PLUGIN(KIMGIOGenerator)

KIMGIOGenerator::KIMGIOGenerator()
    : ThreadedGenerator()
{
    setFeature( ReadRawData );
}

KIMGIOGenerator::~KIMGIOGenerator()
{
}

bool KIMGIOGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    QImageReader reader( fileName );
    if ( !reader.read( &m_img ) ) {
        emit error( i18n( "Unable to load document: %1", reader.errorString() ), -1 );
        return false;
    }

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    return true;
}

bool KIMGIOGenerator::loadDocumentFromData( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector )
{
    QBuffer buffer( const_cast<QByteArray*>( &fileData ) );

    QImageReader reader( &buffer );
    if ( !reader.read( &m_img ) ) {
        emit error( i18n( "Unable to load document: %1", reader.errorString() ), -1 );
        return false;
    }

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    return true;
}

bool KIMGIOGenerator::closeDocument()
{
    m_img = QImage();

    return true;
}

QImage KIMGIOGenerator::image( Okular::PixmapRequest * request )
{
    // perform a smooth scaled generation
    int width = request->width();
    int height = request->height();
    if ( request->page()->rotation() % 2 == 1 )
        qSwap( width, height );

    return m_img.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
}

bool KIMGIOGenerator::print( KPrinter& printer )
{
    QPainter p( &printer );

    uint left, top, right, bottom;
    printer.margins( &left, &top, &right, &bottom );

    int pageWidth = printer.width() - right;
    int pageHeight = printer.height() - bottom;

    QImage image( m_img );
    if ( (image.width() > pageWidth) || (image.height() > pageHeight) )
        image = image.scaled( pageWidth, pageHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    p.drawImage( 0, 0, image );

    return true;
}

#include "generator_kimgio.moc"

