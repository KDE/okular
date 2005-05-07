/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qpainter.h>
#include <qpixmap.h>
#include <kprinter.h>

#include "core/page.h"
#include "generator_kimgio.h"

KIMGIOGenerator::KIMGIOGenerator( KPDFDocument * document ) : Generator( document )
{
}

KIMGIOGenerator::~KIMGIOGenerator()
{
    delete m_pix;
}

bool KIMGIOGenerator::loadDocument( const QString & fileName, QValueVector<KPDFPage*> & pagesVector )
{
    m_pix = new QPixmap(fileName);

    pagesVector.resize( 1 );

    KPDFPage * page = new KPDFPage( 0, m_pix->width(), m_pix->height(), 0 );
    pagesVector[0] = page;

    return true;
}

bool KIMGIOGenerator::canGeneratePixmap()
{
    return true;
}

void KIMGIOGenerator::generatePixmap( PixmapRequest * request )
{
    QPixmap *p = new QPixmap(*m_pix);
    request->page->setPixmap(request->id, p);
}

void KIMGIOGenerator::generateSyncTextPage( KPDFPage * )
{
}

bool KIMGIOGenerator::supportsSearching() const
{
    return false;
}

bool KIMGIOGenerator::hasFonts() const
{
    return false;
}

void KIMGIOGenerator::putFontInfo( KListView * )
{
}

bool KIMGIOGenerator::print( KPrinter& printer )
{
    QPainter p(&printer);
    p.drawPixmap(0, 0, *m_pix);
    return true;
}
