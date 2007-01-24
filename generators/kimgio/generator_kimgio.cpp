/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qpainter.h>
#include <kprinter.h>

#include <okular/core/page.h>

#include "generator_kimgio.h"

OKULAR_EXPORT_PLUGIN(KIMGIOGenerator)

KIMGIOGenerator::KIMGIOGenerator()
    : ThreadedGenerator()
{
}

KIMGIOGenerator::~KIMGIOGenerator()
{
}

bool KIMGIOGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    if ( !m_img.load( fileName ) )
        return false;

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    return true;
}

bool KIMGIOGenerator::loadDocumentFromData( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector )
{
    if ( !m_img.loadFromData( fileData ) )
        return false;

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
    p.drawImage( 0, 0, m_img );
    return true;
}

bool KIMGIOGenerator::hasFeature( GeneratorFeature feature ) const
{
    return feature == Okular::Generator::ReadRawData;
}

#include "generator_kimgio.moc"

