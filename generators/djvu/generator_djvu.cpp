/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_djvu.h"
#include "kdjvu.h"
#include "core/page.h"

#include <qstring.h>
#include <kdebug.h>
#include <klocale.h>

KPDF_EXPORT_PLUGIN(DjVuGenerator)

DjVuGenerator::DjVuGenerator( KPDFDocument * doc ) : Generator ( doc ),
  m_docInfo( 0 ), ready( false )
{
    m_djvu = new KDjVu();
    connect( m_djvu, SIGNAL( pixmapGenerated( int, const QPixmap & ) ), this, SLOT( djvuPixmapGenerated( int, const QPixmap & ) ) );
}

bool DjVuGenerator::loadDocument( const QString & fileName, QVector< KPDFPage * > & pagesVector )
{
    delete m_docInfo;
    m_docInfo = 0;

    if ( !m_djvu->openFile( fileName ) )
        return false;

    loadPages( pagesVector, 0 );

    ready = true;
    return true;
}

bool DjVuGenerator::canGeneratePixmap( bool /*async*/ )
{
    return ready;
}

void DjVuGenerator::generatePixmap( PixmapRequest * request )
{
    ready = false;

    m_request = request;

    QPixmap pix = m_djvu->pixmap( request->pageNumber, request->width, request->height, request->documentRotation );
    if ( pix.isNull() )
    {

        m_djvu->requestPixmap( request->pageNumber, request->width, request->height, request->documentRotation );
    }
    else
    {
        djvuPixmapGenerated( request->pageNumber, pix );
    }
}

const DocumentInfo * DjVuGenerator::generateDocumentInfo()
{
    if ( m_docInfo )
        return m_docInfo;

    m_docInfo = new DocumentInfo();

    m_docInfo->set( "mimeType", "image/x-djvu" );

    if ( m_djvu )
    {
        // compile internal structure reading properties from KDjVu
        QString doctype = m_djvu->getMetaData( "documentType" );
        m_docInfo->set( "documentType", doctype.isEmpty() ? i18n( "Unknown" ) : doctype, i18n( "Type of document" ) );
        QString numcomponents = m_djvu->getMetaData( "componentFile" );
        m_docInfo->set( "componentFile", numcomponents.isEmpty() ? i18n( "Unknown" ) : numcomponents, i18n( "Component Files" ) );
    }
    else
    {
        m_docInfo->set( "documentType", i18n( "Unknown" ), i18n( "Type of document" ) );
        m_docInfo->set( "componentFile", i18n( "Unknown" ), i18n( "Component Files" ) );
    }

    return m_docInfo;
}

void DjVuGenerator::setOrientation( QVector<KPDFPage*> & pagesVector, int orientation )
{
    loadPages( pagesVector, orientation );
}

void DjVuGenerator::djvuPixmapGenerated( int /*page*/, const QPixmap & pix )
{
    m_request->page->setPixmap( m_request->id, new QPixmap( pix ) );

    ready = true;
    signalRequestDone( m_request );
}

void DjVuGenerator::loadPages( QVector<KPDFPage*> & pagesVector, int rotation )
{
    const QVector<KDjVu::Page*> &djvu_pages = m_djvu->pages();
    int numofpages = djvu_pages.count();
    pagesVector.resize( numofpages );

    for ( int i = 0; i < numofpages; ++i )
    {
        const KDjVu::Page *p = djvu_pages.at( i );
        if (pagesVector[i])
            delete pagesVector[i];
        int w = p->width();
        int h = p->height();
        if ( rotation % 2 == 1 )
            qSwap( w, h );
        KPDFPage *page = new KPDFPage( i, w, h, p->orientation() + rotation );
        pagesVector[i] = page;
    }
}


#include "generator_djvu.moc"
