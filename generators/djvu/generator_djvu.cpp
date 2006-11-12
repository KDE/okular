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
#include "core/annotations.h"
#include "core/area.h"
#include "core/document.h"
#include "core/link.h"
#include "core/page.h"
#include "core/utils.h"

#include <qdom.h>
#include <qstring.h>
#include <quuid.h>
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

DjVuGenerator::DjVuGenerator() : Okular::Generator(),
  m_docInfo( 0 ), m_docSyn( 0 ), ready( false )
{
    m_djvu = new KDjVu();
    connect( m_djvu, SIGNAL( imageGenerated( int, const QImage & ) ), this, SLOT( djvuImageGenerated( int, const QImage & ) ) );
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

bool DjVuGenerator::canGeneratePixmap( bool /*async*/ ) const
{
    return ready;
}

void DjVuGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    ready = false;

    m_request = request;

    QImage img = m_djvu->image( request->pageNumber(), request->width(), request->height(), request->page()->rotation() );
    if ( img.isNull() )
    {

        m_djvu->requestImage( request->pageNumber(), request->width(), request->height(), request->page()->rotation() );
    }
    else
    {
        djvuImageGenerated( request->pageNumber(), img );
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
        QString doctype = m_djvu->metaData( "documentType" ).toString();
        m_docInfo->set( "documentType", doctype.isEmpty() ? i18n( "Unknown" ) : doctype, i18n( "Type of document" ) );
        QVariant numcomponents = m_djvu->metaData( "componentFile" );
        m_docInfo->set( "componentFile", numcomponents.type() != QVariant::Int ? i18n( "Unknown" ) : numcomponents.toString(), i18n( "Component Files" ) );
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

void DjVuGenerator::djvuImageGenerated( int page, const QImage & img )
{
    m_request->page()->setImage( m_request->id(), img );

    QList<KDjVu::Link*> links;
    QList<KDjVu::Annotation*> annots;
    m_djvu->linksAndAnnotationsForPage( page, links, annots );
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
                bool scape_orientation = false; // hack by tokoe, should always create default page
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
                        newrect = new Okular::ObjectRect( Okular::NormalizedRect( Okular::Utils::rotateRect( r, width, height, 0 ), width, height ), ellipse, Okular::ObjectRect::Link, newlink );
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
            m_request->page()->setObjectRects( rects );
    }
    m_request->page()->deleteAnnotations();
    if ( annots.count() > 0 )
    {
        QList<KDjVu::Annotation*>::ConstIterator it = annots.constBegin();
        QList<KDjVu::Annotation*>::ConstIterator itEnd = annots.constEnd();
        for ( ; it != itEnd; ++it )
        {
            KDjVu::Annotation *ann = (*it);
            Okular::Annotation *newann = 0;
            const KDjVu::Page* p = m_djvu->pages().at( page );
            int width = p->width();
            int height = p->height();
            bool scape_orientation = false; // hack by tokoe, should always create default page
            if ( scape_orientation )
                qSwap( width, height );
            switch ( ann->type() )
            {
                case KDjVu::Annotation::TextAnnotation:
                {
                    KDjVu::TextAnnotation* txtann = static_cast<KDjVu::TextAnnotation*>( ann );
                    Okular::TextAnnotation * newtxtann = new Okular::TextAnnotation();
                    // boundary
                    QRect rect( QPoint( txtann->point().x(), height - txtann->point().y() - txtann->size().height() ), txtann->size() );
                    newtxtann->boundary = Okular::NormalizedRect( Okular::Utils::rotateRect( rect, width, height, 0 ), width, height );
                    // type
                    newtxtann->textType = txtann->inlineText() ? Okular::TextAnnotation::InPlace : Okular::TextAnnotation::Linked;
                    newtxtann->style.opacity = txtann->color().alphaF();
                    // FIXME remove once the annotation text handling is fixed
                    newtxtann->inplaceText = ann->comment();
                    newann = newtxtann;
                    break;
                }
                case KDjVu::Annotation::LineAnnotation:
                {
                    KDjVu::LineAnnotation* lineann = static_cast<KDjVu::LineAnnotation*>( ann );
                    Okular::LineAnnotation * newlineann = new Okular::LineAnnotation();
                    // boundary
                    QPoint a( lineann->point().x(), height - lineann->point().y() );
                    QPoint b( lineann->point2().x(), height - lineann->point2().y() );
                    QRect rect = QRect( a, b ).normalized();
                    newlineann->boundary = Okular::NormalizedRect( Okular::Utils::rotateRect( rect, width, height, 0 ), width, height );
                    // line points
                    newlineann->linePoints.append( Okular::NormalizedPoint( a.x(), a.y(), width, height ) );
                    newlineann->linePoints.append( Okular::NormalizedPoint( b.x(), b.y(), width, height ) );
                    // arrow?
                    if ( lineann->isArrow() )
                        newlineann->lineEndStyle = Okular::LineAnnotation::OpenArrow;
                    // width
                    newlineann->style.width = lineann->width();
                    newann = newlineann;
                    break;
                }
            }
            if ( newann )
            {
                // setting the common parameters
                newann->style.color = ann->color();
                newann->contents = ann->comment();
                // creating an id as name for the annotation
                QString uid = QUuid::createUuid().toString();
                uid.remove( 0, 1 );
                uid.chop( 1 );
                uid.remove( QLatin1Char( '-' ) );
                newann->uniqueName = uid;

                // adding the newly converted annotation to the page
                m_request->page()->addAnnotation( newann );
            }
            // delete the annotations as soon as we process them
            delete ann;
        }
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
