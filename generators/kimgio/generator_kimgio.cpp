/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qpainter.h>
#include <qimage.h>
#include <kprinter.h>

#include "core/page.h"
#include "generator_kimgio.h"

OKULAR_EXPORT_PLUGIN(KIMGIOGenerator)

KIMGIOGenerator::KIMGIOGenerator() : Generator()
{
}

KIMGIOGenerator::~KIMGIOGenerator()
{
}

bool KIMGIOGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    m_pix = new QPixmap(fileName);

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_pix->width(), m_pix->height(), 0 );
    pagesVector[0] = page;

    return true;
}

bool KIMGIOGenerator::closeDocument()
{
    delete m_pix;
    m_pix = 0;

    return true;
}

bool KIMGIOGenerator::canGeneratePixmap( bool /* async */ ) const
{
    return true;
}

void KIMGIOGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    // perform a smooth scaled generation
    int width = request->width();
    int height = request->height();
    if ( request->page()->rotation() % 2 == 1 )
        qSwap( width, height );
    QImage image = m_pix->toImage().scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    request->page()->setImage( request->id(), image );

    // signal that the request has been accomplished
    signalRequestDone(request);
}

bool KIMGIOGenerator::print( KPrinter& printer )
{
    QPainter p(&printer);
    p.drawPixmap(0, 0, *m_pix);
    return true;
}

#include "generator_kimgio.moc"

