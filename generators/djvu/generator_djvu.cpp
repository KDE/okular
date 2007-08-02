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
#include <okular/core/action.h>
#include <okular/core/annotations.h>
#include <okular/core/area.h>
#include <okular/core/document.h>
#include <okular/core/page.h>
#include <okular/core/textpage.h>
#include <okular/core/utils.h>

#include <qdom.h>
#include <qpixmap.h>
#include <qstring.h>
#include <quuid.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <klocale.h>
#include <kprinter.h>
#include <ktemporaryfile.h>

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
    setFeature( TextExtraction );
    m_djvu = new KDjVu();
    connect( m_djvu, SIGNAL( imageGenerated( int, const QImage & ) ), this, SLOT( djvuImageGenerated( int, const QImage & ) ) );

    KAboutData *about = new KAboutData(
         "generator_djvu",
         "generator_djvu",
         ki18n( "DjVu Backend" ),
         "0.1",
         ki18n( "A DjVu backend" ),
         KAboutData::License_GPL,
         ki18n( "© 2006-2007 Pino Toscano" )
    );
    about->addAuthor( ki18n( "Pino Toscano" ), KLocalizedString(), "pino@kde.org" );
    setAboutData( about );
}

DjVuGenerator::~DjVuGenerator()
{
    delete m_djvu;
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

bool DjVuGenerator::canGeneratePixmap() const
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

    m_docInfo->set( Okular::DocumentInfo::MimeType, "image/vnd.djvu" );

    if ( m_djvu )
    {
        // compile internal structure reading properties from KDjVu
        QString title = m_djvu->metaData( "title" ).toString();
        m_docInfo->set( Okular::DocumentInfo::Title, title.isEmpty() ? i18n( "Unknown" ) : title );
        QString author = m_djvu->metaData( "author" ).toString();
        m_docInfo->set( Okular::DocumentInfo::Author, author.isEmpty() ? i18n( "Unknown" ) : author );
        QString editor = m_djvu->metaData( "editor" ).toString();
        m_docInfo->set( "editor", editor.isEmpty() ? i18n( "Unknown" ) : editor, i18n( "Editor" ) );
        QString publisher = m_djvu->metaData( "publisher" ).toString();
        m_docInfo->set( "publisher", publisher.isEmpty() ? i18n( "Unknown" ) : publisher, i18n( "Publisher" ) );
        QString year = m_djvu->metaData( "year" ).toString();
        m_docInfo->set( Okular::DocumentInfo::CreationDate, year.isEmpty() ? i18n( "Unknown" ) : year );
        QString volume = m_djvu->metaData( "volume" ).toString();
        m_docInfo->set( "volume", volume.isEmpty() ? i18n( "Unknown" ) : volume, i18n( "Volume" ) );
        QString doctype = m_djvu->metaData( "documentType" ).toString();
        m_docInfo->set( "documentType", doctype.isEmpty() ? i18n( "Unknown" ) : doctype, i18n( "Type of document" ) );
        QVariant numcomponents = m_djvu->metaData( "componentFile" );
        m_docInfo->set( "componentFile", numcomponents.type() != QVariant::Int ? i18n( "Unknown" ) : numcomponents.toString(), i18n( "Component Files" ) );
    }
    else
    {
        m_docInfo->set( Okular::DocumentInfo::Title, i18n( "Unknown" ) );
        m_docInfo->set( Okular::DocumentInfo::Author, i18n( "Unknown" ) );
        m_docInfo->set( "editor", i18n( "Unknown" ), i18n( "Editor" ) );
        m_docInfo->set( "publisher", i18n( "Unknown" ), i18n( "Publisher" ) );
        m_docInfo->set( Okular::DocumentInfo::CreationDate, i18n( "Unknown" ) );
        m_docInfo->set( "volume", i18n( "Unknown" ), i18n( "Volume" ) );
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

bool DjVuGenerator::print( KPrinter& printer )
{
    QList<int> pageList;
    if ( !printer.previewOnly() )
        pageList = printer.pageList();
    else
    {
        int pages = m_djvu->pages().count();
        for( int i = 1; i <= pages; ++i )
            pageList.push_back(i);
    }

    KTemporaryFile tf;
    tf.setSuffix( ".ps" );
    if ( !tf.open() )
        return false;
    QString tempfilename = tf.fileName();
    tf.close();

    if ( m_djvu->exportAsPostScript( tempfilename, pageList ) )
    {
        return printer.printFiles( QStringList( tempfilename ), true );
    }
    return false;
}

Okular::TextPage* DjVuGenerator::textPage( Okular::Page *page )
{
#if 0
    QList<KDjVu::TextEntity> te = m_djvu->textEntities( page->number(), "char" );
    if ( te.isEmpty() )
        te = m_djvu->textEntities( page->number(), "word" );
#else
    QList<KDjVu::TextEntity> te = m_djvu->textEntities( page->number(), "word" );
#endif
    QList<KDjVu::TextEntity>::ConstIterator it = te.constBegin();
    QList<KDjVu::TextEntity>::ConstIterator itEnd = te.constEnd();
    QList<Okular::TextEntity*> words;
    const KDjVu::Page* djvupage = m_djvu->pages().at( page->number() );
    for ( ; it != itEnd; ++it )
    {
        const KDjVu::TextEntity& cur = *it;
        words.append( new Okular::TextEntity( cur.text(), new Okular::NormalizedRect( cur.rect(), djvupage->width(), djvupage->height() ) ) );
    }
    Okular::TextPage *textpage = new Okular::TextPage( words );
    return textpage;
}


void DjVuGenerator::djvuImageGenerated( int page, const QImage & img )
{
    (void)page;
    m_request->page()->setPixmap( m_request->id(), new QPixmap( QPixmap::fromImage( img ) ) );

    ready = true;
    signalPixmapRequestDone( m_request );
    m_request = 0;
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
        Okular::Page *page = new Okular::Page( i, w, h, (Okular::Rotation)( p->orientation() + rotation ) );
        pagesVector[i] = page;

        QList<KDjVu::Annotation*> annots;
        QList<KDjVu::Link*> links;
        m_djvu->linksAndAnnotationsForPage( i, &links, &annots );
        if ( !links.isEmpty() )
        {
            QLinkedList<Okular::ObjectRect *> rects;
            QList<KDjVu::Link*>::ConstIterator it = links.constBegin();
            QList<KDjVu::Link*>::ConstIterator itEnd = links.constEnd();
            for ( ; it != itEnd; ++it )
            {
                KDjVu::Link *curlink = (*it);
                Okular::ObjectRect *newrect = convertKDjVuLink( i, curlink );
                if ( newrect )
                    rects.append( newrect );
                // delete the links as soon as we process them
                delete curlink;
            }
            if ( rects.count() > 0 )
                page->setObjectRects( rects );
        }
        if ( !annots.isEmpty() )
        {
            QList<KDjVu::Annotation*>::ConstIterator it = annots.constBegin();
            QList<KDjVu::Annotation*>::ConstIterator itEnd = annots.constEnd();
            for ( ; it != itEnd; ++it )
            {
                KDjVu::Annotation *ann = (*it);
                Okular::Annotation *newann = convertKDjVuAnnotation( w, h, ann );
                if ( newann )
                    page->addAnnotation( newann );
                // delete the annotations as soon as we process them
                delete ann;
            }
        }
    }
}

Okular::ObjectRect* DjVuGenerator::convertKDjVuLink( int page, KDjVu::Link * link ) const
{
    int newpage = -1;
    Okular::Action *newlink = 0;
    Okular::ObjectRect *newrect = 0;
    switch ( link->type() )
    {
        case KDjVu::Link::PageLink:
        {
            KDjVu::PageLink* l = static_cast<KDjVu::PageLink*>( link );
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
                newlink = new Okular::GotoAction( QString(), vp );
            }
            break;
        }
        case KDjVu::Link::UrlLink:
        {
            KDjVu::UrlLink* l = static_cast<KDjVu::UrlLink*>( link );
            QString url = l->url();
            newlink = new Okular::BrowseAction( url );
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
        switch ( link->areaType() )
        {
            case KDjVu::Link::RectArea:
            case KDjVu::Link::EllipseArea:
            {
                QRect r( QPoint( link->point().x(), p->height() - link->point().y() - link->size().height() ), link->size() );
                bool ellipse = ( link->areaType() == KDjVu::Link::EllipseArea );
                newrect = new Okular::ObjectRect( Okular::NormalizedRect( Okular::Utils::rotateRect( r, width, height, 0 ), width, height ), ellipse, Okular::ObjectRect::Action, newlink );
                break;
            }
            case KDjVu::Link::PolygonArea:
            {
                QPolygon poly = link->polygon();
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
                    newrect = new Okular::ObjectRect( newpoly, Okular::ObjectRect::Action, newlink );
                }
                break;
            }
            default: ;
        }
        if ( !newrect )
        {
            delete newlink;
        }
    }
    return newrect;
}

Okular::Annotation* DjVuGenerator::convertKDjVuAnnotation( int w, int h, KDjVu::Annotation * ann ) const
{
    Okular::Annotation *newann = 0;
    switch ( ann->type() )
    {
        case KDjVu::Annotation::TextAnnotation:
        {
            KDjVu::TextAnnotation* txtann = static_cast<KDjVu::TextAnnotation*>( ann );
            Okular::TextAnnotation * newtxtann = new Okular::TextAnnotation();
            // boundary
            QRect rect( QPoint( txtann->point().x(), h - txtann->point().y() - txtann->size().height() ), txtann->size() );
            newtxtann->setBoundingRectangle( Okular::NormalizedRect( Okular::Utils::rotateRect( rect, w, h, 0 ), w, h ) );
            // type
            newtxtann->setTextType( txtann->inlineText() ? Okular::TextAnnotation::InPlace : Okular::TextAnnotation::Linked );
            newtxtann->style().setOpacity( txtann->color().alphaF() );
            // FIXME remove once the annotation text handling is fixed
            newtxtann->setInplaceText( ann->comment() );
            newann = newtxtann;
            break;
        }
        case KDjVu::Annotation::LineAnnotation:
        {
            KDjVu::LineAnnotation* lineann = static_cast<KDjVu::LineAnnotation*>( ann );
            Okular::LineAnnotation * newlineann = new Okular::LineAnnotation();
            // boundary
            QPoint a( lineann->point().x(), h - lineann->point().y() );
            QPoint b( lineann->point2().x(), h - lineann->point2().y() );
            QRect rect = QRect( a, b ).normalized();
            newlineann->setBoundingRectangle( Okular::NormalizedRect( Okular::Utils::rotateRect( rect, w, h, 0 ), w, h ) );
            // line points
            QLinkedList<Okular::NormalizedPoint> points;
            points.append( Okular::NormalizedPoint( a.x(), a.y(), w, h ) );
            points.append( Okular::NormalizedPoint( b.x(), b.y(), w, h ) );
            newlineann->setLinePoints( points );
            // arrow?
            if ( lineann->isArrow() )
                newlineann->setLineEndStyle( Okular::LineAnnotation::OpenArrow );
            // width
            newlineann->style().setWidth( lineann->width() );
            newann = newlineann;
            break;
        }
    }
    if ( newann )
    {
        // setting the common parameters
        newann->style().setColor( ann->color() );
        newann->setContents( ann->comment() );
        // creating an id as name for the annotation
        QString uid = QUuid::createUuid().toString();
        uid.remove( 0, 1 );
        uid.chop( 1 );
        uid.remove( QLatin1Char( '-' ) );
        newann->setUniqueName( uid );
    }
    return newann;
}


#include "generator_djvu.moc"
