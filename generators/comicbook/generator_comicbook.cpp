/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qpainter.h>
#include <kprinter.h>

#include <okular/core/page.h>

#include "generator_comicbook.h"

OKULAR_EXPORT_PLUGIN(ComicBookGenerator)

ComicBookGenerator::ComicBookGenerator()
    : Generator()
{
}

ComicBookGenerator::~ComicBookGenerator()
{
}

bool ComicBookGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    if ( !mDocument.open( fileName ) )
        return false;

    int pages = mDocument.pages();
    pagesVector.resize( pages );

    for ( int i = 0; i < pages; ++i ) {
        Okular::Page * page = new Okular::Page( i, 800, 600, Okular::Rotation0 );
        pagesVector[i] = page;
    }

    return true;
}

bool ComicBookGenerator::closeDocument()
{
    return true;
}

bool ComicBookGenerator::canGeneratePixmap( bool ) const
{
    return true;
}

void ComicBookGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    int width = request->width();
    int height = request->height();

    QImage image = mDocument.pageImage( request->pageNumber() );
    image = image.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( image ) ) );

    // signal that the request has been accomplished
    signalRequestDone(request);
}

bool ComicBookGenerator::print( KPrinter& printer )
{
    QPainter p( &printer );

    for ( int i = 0; i < mDocument.pages(); ++i ) {
        const QImage image = mDocument.pageImage( i );

        if ( i != 0 )
            printer.newPage();

        p.drawImage( 0, 0, image );
    }

    return true;
}

bool ComicBookGenerator::hasFeature( GeneratorFeature ) const
{
    return false;
}

#include "generator_comicbook.moc"

