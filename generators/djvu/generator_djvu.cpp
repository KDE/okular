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
#include "core/area.h"
#include "core/document.h"
#include "core/link.h"
#include "core/page.h"
#include "core/utils.h"

#include <qdom.h>
#include <qstring.h>
#include <kdebug.h>
#include <klocale.h>

static void recurseCreateTOC( QDomDocument &maindoc, QDomNode &parent, QDomNode &parentDestination )
{
    QDomNode n = parent.firstChild();
    while( !n.isNull() )
    {
        QDomElement el = n.toElement();

        QDomElement newel = maindoc.createElement( el.attribute( "title" ) );
        parentDestination.appendChild( newel );

        if ( !el.attribute( "destination" ).isEmpty() )
        {
            bool ok = true;
            int page = el.attribute( "destination" ).toInt( &ok );
            if ( ok && ( page > 0 ) )
            {
                Okular::DocumentViewport vp;
                vp.pageNumber = page - 1;
                newel.setAttribute( "Viewport", vp.toString() );
            }
        }

        if ( el.hasChildNodes() )
        {
            recurseCreateTOC( maindoc, n, newel );
        }
        n = n.nextSibling();
    }
}

OKULAR_EXPORT_PLUGIN(DjVuGenerator)

DjVuGenerator::DjVuGenerator( Okular::Document * doc ) : Okular::Generator ( doc ),
  m_docInfo( 0 ), m_docSyn( 0 ), ready( false )
{
    m_djvu = new KDjVu();
    connect( m_djvu, SIGNAL( pixmapGenerated( int, const QPixmap & ) ), this, SLOT( djvuPixmapGenerated( int, const QPixmap & ) ) );
}

bool DjVuGenerator::loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector )
{
    if ( !m_djvu->openFile( fileName ) )
        return false;

    loadPages( pagesVector, 0 );

    ready = true;
    return true;
}

bool DjVuGenerator::closeDocument()
{
    m_djvu->closeFile();

    delete m_docInfo;
    m_docInfo = 0;
    delete m_docSyn;
    m_docSyn = 0;

    ready = false;

    return true;
}

bool DjVuGenerator::canGeneratePixmap( bool /*async*/ )
{
    return ready;
}

void DjVuGenerator::generatePixmap( Okular::PixmapRequest * request )
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

const Okular::DocumentInfo * DjVuGenerator::generateDocumentInfo()
{
    if ( m_docInfo )
        return m_docInfo;

    m_docInfo = new Okular::DocumentInfo();

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

const Okular::DocumentSynopsis * DjVuGenerator::generateDocumentSynopsis()
{
    if ( m_docSyn )
        return m_docSyn;

    const QDomDocument *doc = m_djvu->documentBookmarks();
    if ( doc )
    {
        m_docSyn = new Okular::DocumentSynopsis();
        recurseCreateTOC( *m_docSyn, *const_cast<QDomDocument*>( doc ), *m_docSyn );
    }

    return m_docSyn;
}

void DjVuGenerator::djvuPixmapGenerated( int page, const QPixmap & pix )
{
    m_request->page->setPixmap( m_request->id, new QPixmap( pix ) );

    QList<KDjVu::Link*> links = m_djvu->linksForPage( page );
    if ( links.count() > 0 )
    {
        QLinkedList<Okular::ObjectRect *> rects;
        QList<KDjVu::Link*>::ConstIterator it = links.constBegin();
        QList<KDjVu::Link*>::ConstIterator itEnd = links.constEnd();
        for ( ; it != itEnd; ++it )
        {
            KDjVu::Link *curlink = (*it);
            Okular::Link *newlink = 0;
            int newpage = -1;
            switch ( curlink->type() )
            {
                case KDjVu::Link::PageLink:
                {
                    KDjVu::PageLink* l = static_cast<KDjVu::PageLink*>( curlink );
                    bool ok = true;
                    QString target = l->page();
                    if ( ( target.length() > 0 ) && target.at(0) == QLatin1Char( '#' ) )
                        target.remove( 0, 1 );
                    int tmppage = target.toInt( &ok );
                    if ( ok || target.isEmpty() )
                    {
                        Okular::DocumentViewport vp;
                        if ( !target.isEmpty() )
                        {
                            vp.pageNumber = ( target.at(0) == QLatin1Char( '+' ) || target.at(0) == QLatin1Char( '-' ) ) ? page + tmppage : tmppage - 1;
                            newpage = vp.pageNumber;
                        }
                        newlink = new Okular::LinkGoto( QString::null, vp );
                    }
                    break;
                }
                case KDjVu::Link::UrlLink:
                {
                    KDjVu::UrlLink* l = static_cast<KDjVu::UrlLink*>( curlink );
                    QString url = l->url();
                    newlink = new Okular::LinkBrowse( url );
                    break;
                }
            }
            if ( newlink )
            {
                const KDjVu::Page* p = m_djvu->pages().at( newpage == -1 ? page : newpage );
                int width = p->width();
                int height = p->height();
                bool scape_orientation = ( m_request->documentRotation % 2 == 1 );
                if ( scape_orientation )
                    qSwap( width, height );
                Okular::ObjectRect *newrect = 0;
                switch ( curlink->areaType() )
                {
                    case KDjVu::Link::RectArea:
                    case KDjVu::Link::EllipseArea:
                    {
                        QRect r( QPoint( curlink->point().x(), p->height() - curlink->point().y() - curlink->size().height() ), curlink->size() );
                        bool ellipse = ( curlink->areaType() == KDjVu::Link::EllipseArea );
                        newrect = new Okular::ObjectRect( Okular::NormalizedRect( Okular::Utils::rotateRect( r, width, height, m_request->documentRotation ), width, height ), ellipse, Okular::ObjectRect::Link, newlink );
                        break;
                    }
                    case KDjVu::Link::PolygonArea:
                    {
                        QPolygon poly = curlink->polygon();
                        QPolygonF newpoly;
                        for ( int i = 0; i < poly.count(); ++i )
                        {
                            int x = poly.at(i).x();
                            int y = poly.at(i).y();
                            if ( scape_orientation )
                                qSwap( x, y );
                            else
                            {
                                y = height - y;
                            }
                            newpoly << QPointF( (double)(x)/width, (double)(y)/height );
                        }
                        if ( !newpoly.isEmpty() )
                        {
                            newpoly << newpoly.first();
                            newrect = new Okular::ObjectRect( newpoly, Okular::ObjectRect::Link, newlink );
                        }
                        break;
                    }
                    default: ;
                }
                if ( newrect )
                    rects.append( newrect );
                else
                    delete newlink;
            }
            // delete the links as soon as we process them
            delete curlink;
        }
        if ( rects.count() > 0 )
            m_request->page->setObjectRects( rects );
    }

    ready = true;
    signalRequestDone( m_request );
}

void DjVuGenerator::loadPages( QVector<Okular::Page*> & pagesVector, int rotation )
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
        Okular::Page *page = new Okular::Page( i, w, h, p->orientation() + rotation );
        pagesVector[i] = page;
    }
}


#include "generator_djvu.moc"
