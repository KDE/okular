/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymański <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_chm.h"

#include <QtCore/QEventLoop>
#include <QtCore/QMutex>
#include <QtGui/QPainter>

#include <kaboutdata.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <klocale.h>
#include <kurl.h>
#include <dom/html_misc.h>
#include <dom/dom_node.h>
#include <dom/dom_html.h>

#include <okular/core/action.h>
#include <okular/core/observer.h> //for PAGEVIEW_ID
#include <okular/core/page.h>
#include <okular/core/textpage.h>

#include "lib/xchmfile.h"

OKULAR_EXPORT_PLUGIN(CHMGenerator)

CHMGenerator::CHMGenerator()
    : Okular::Generator()
{
    setFeature( TextExtraction );

    KAboutData *about = new KAboutData(
         "okular_chm",
         "okular_chm",
         ki18n( "CHM Backend" ),
         "0.1",
         ki18n( "A Microsoft Windows help file renderer" ),
         KAboutData::License_GPL,
         ki18n( "© 2005-2007 Piotr Szymański" )
    );
    about->addAuthor( ki18n( "Piotr Szymański" ), KLocalizedString(), "niedakh@gmail.com" );
    setAboutData( about );

    m_syncGen=0;
    m_file=0;
    m_docInfo=0;
    m_pixmapRequestZoom=1;
    m_request = 0;
}

CHMGenerator::~CHMGenerator()
{
    delete m_syncGen;
}

bool CHMGenerator::loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector )
{
    m_fileName=fileName;
    m_file=new CHMFile (fileName);
    m_file->ParseAndFillTopicsTree (&m_docSyn);

    pagesVector.resize(m_file->m_UrlPage.count());
    m_textpageAddedList.fill(false, pagesVector.count());

    if (!m_syncGen)
    {
        m_syncGen = new KHTMLPart();
        connect (m_syncGen,SIGNAL(completed()),this,SLOT(slotCompleted()));
    }

    QMap <QString, int>::ConstIterator it=m_file->m_UrlPage.begin(), end=m_file->m_UrlPage.end();
    for (;it!=end;++it)
    {
        preparePageForSyncOperation(100,it.key());
        int i= it.value() - 1;
        pagesVector[ i ] = new Okular::Page (i, m_syncGen->view()->contentsWidth(),
            m_syncGen->view()->contentsHeight(), Okular::Rotation0 );
    }

    return true;
}

bool CHMGenerator::closeDocument()
{
    // delete the document information of the old document
    delete m_docInfo;
    m_docInfo=0;
    delete m_file;
    m_file=0;
    m_textpageAddedList.clear();
    m_docSyn.clear();
    if (m_syncGen)
    {
        m_syncGen->closeUrl();
    }

    return true;
}

void CHMGenerator::preparePageForSyncOperation( int zoom , const QString & url)
{
    KUrl pAddress= "ms-its:" + m_fileName + "::" + url;
    m_syncGen->setZoomFactor(zoom);
    m_syncGen->openUrl(pAddress);
    m_syncGen->view()->layout();

    QEventLoop loop;
    connect( m_syncGen, SIGNAL( completed() ), &loop, SLOT( quit() ) );
    loop.exec();
}

void CHMGenerator::slotCompleted()
{
    if ( !m_request )
        return;

    QImage image( m_request->width(), m_request->height(), QImage::Format_ARGB32 );
    image.fill( qRgb( 255, 255, 255 ) );

    QPainter p( &image );
    QRect r( 0, 0, m_request->width(), m_request->height() );

    bool moreToPaint;
    m_syncGen->paint( &p, r, 0, &moreToPaint );

    p.end();

    if ( m_pixmapRequestZoom > 1 )
        m_pixmapRequestZoom = 1;

    if ( !m_textpageAddedList.at( m_request->pageNumber() ) ) {
        additionalRequestData();
        m_textpageAddedList[ m_request->pageNumber() ] = true;
    }

    m_syncGen->closeUrl();

    userMutex()->unlock();

    Okular::PixmapRequest *req = m_request;
    m_request = 0;

    req->page()->setPixmap( req->id(), new QPixmap( QPixmap::fromImage( image ) ) );
    signalPixmapRequestDone( req );
}

const Okular::DocumentInfo * CHMGenerator::generateDocumentInfo() 
{
    if (!m_docInfo)
    {
        m_docInfo=new Okular::DocumentInfo();

        m_docInfo->set( Okular::DocumentInfo::MimeType, "application/x-chm" );
        m_docInfo->set( Okular::DocumentInfo::Title, m_file->Title() );
    }
    return m_docInfo;
}

const Okular::DocumentSynopsis * CHMGenerator::generateDocumentSynopsis()
{
    return &m_docSyn;
}

const Okular::DocumentFonts * CHMGenerator::generateDocumentFonts() 
{
    return 0L;
}

bool CHMGenerator::canGeneratePixmap () const
{
    bool isLocked = true;
    if ( userMutex()->tryLock() ) {
        userMutex()->unlock();
        isLocked = false;
    }

    return !isLocked;
}

void CHMGenerator::generatePixmap( Okular::PixmapRequest * request ) 
{
    int requestWidth = request->width();
    int requestHeight = request->height();
    if (requestWidth<300)
    {
        m_pixmapRequestZoom=900/requestWidth;
        requestWidth*=m_pixmapRequestZoom;
        requestHeight*=m_pixmapRequestZoom;
    }

    userMutex()->lock();
    QString url= m_file->getUrlForPage ( request->pageNumber() + 1 );
    int zoom = qRound( qMax( static_cast<double>(requestWidth)/static_cast<double>(request->page()->width())
        , static_cast<double>(requestHeight)/static_cast<double>(request->page()->height())
        ) ) * 100;

    KUrl pAddress= "ms-its:" + m_fileName + "::" + url;
    m_syncGen->setZoomFactor(zoom);
    m_syncGen->view()->resize(requestWidth,requestHeight);
    m_request=request;
    // will emit openURL without problems
    m_syncGen->openUrl ( pAddress );
}


void CHMGenerator::recursiveExploreNodes(DOM::Node node,Okular::TextPage *tp)
{
    if (node.nodeType() == DOM::Node::TEXT_NODE)
    {
        QString nodeText=node.nodeValue().string();
        QRect r=node.getRect();
        int vWidth=m_syncGen->view()->contentsWidth();
        int vHeight=m_syncGen->view()->contentsHeight();
        Okular::NormalizedRect *nodeNormRect;
#define NOEXP
#ifndef NOEXP
        int x,y,height;
        int x_next,y_next,height_next;
        int nodeTextLength = nodeText.length();
        if (nodeTextLength==1)
        {
            nodeNormRect=new Okular::NormalizedRect (r,vWidth,vHeight);
            tp->append(nodeText,nodeNormRect,nodeNormRect->bottom,0,(nodeText=="\n"));
        }
        else
        {
            for (int i=0;i<nodeTextLength;i++)
            { 
                node.getCursor(i,x,y,height);
                if (i==0)
                // i is 0, use left rect boundary
                {
//                     if (nodeType[i+1]
                    node.getCursor(i+1,x_next,y_next,height_next);
                    nodeNormRect=new Okular::NormalizedRect (QRect(x,y,x_next-x-1,height),vWidth,vHeight);
                }
                else if ( i <nodeTextLength -1 )
                // i is between zero and the last element
                {
                    node.getCursor(i+1,x_next,y_next,height_next);
                    nodeNormRect=new Okular::NormalizedRect (QRect(x,y,x_next-x-1,height),vWidth,vHeight);
                }
                else
                // the last element use right rect boundary
                {
                    node.getCursor(i-1,x_next,y_next,height_next);
            }
        }
#else
        nodeNormRect=new Okular::NormalizedRect (r,vWidth,vHeight);
        tp->append(nodeText,nodeNormRect/*,0*/);
#endif
    }
    DOM::Node child = node.firstChild();
    while ( !child.isNull() )
    {
        recursiveExploreNodes(child,tp);
        child = child.nextSibling();
    }
}

void CHMGenerator::additionalRequestData() 
{
    Okular::Page * page=m_request->page();
    bool genObjectRects = m_request->id() & (PAGEVIEW_ID | PRESENTATION_ID);
    bool genTextPage = !m_request->page()->hasTextPage() && genObjectRects;

    if (genObjectRects || genTextPage )
    {
        DOM::HTMLDocument domDoc=m_syncGen->htmlDocument();
        // only generate object info when generating a full page not a thumbnail
        if ( genObjectRects )
        {
            QLinkedList< Okular::ObjectRect * > objRects;
            int xScale=qRound(page->width());
            int yScale=qRound(page->height());
            // getting links
            DOM::HTMLCollection coll=domDoc.links();
            DOM::Node n;
            QRect r;
            if (! coll.isNull() )
            {
                int size=coll.length();
                for(int i=0;i<size;i++)
                {
                    n=coll.item(i);
                    if ( !n.isNull() )
                    {
                        QString url = n.attributes().getNamedItem("href").nodeValue().string();
                        r=n.getRect();
                        // there is no way for us to support javascript properly
                        if (url.startsWith("JavaScript:"), Qt::CaseInsensitive)
                            continue;
                        else if (url.contains (":"))
                        {
                            objRects.push_back(
                                new Okular::ObjectRect ( Okular::NormalizedRect(r,xScale,yScale),
                                false,
                                Okular::ObjectRect::Action,
                                new Okular::BrowseAction ( url )));
                        }
                        else
                        {
                            Okular::DocumentViewport viewport( metaData( "NamedViewport", '/' + url ).toString() );
                            objRects.push_back(
                                new Okular::ObjectRect ( Okular::NormalizedRect(r,xScale,yScale),
                                false,
                                Okular::ObjectRect::Action,
                                new Okular::GotoAction ( QString::null, viewport)));	//krazy:exclude=nullstrassign for old broken gcc
                        }
                    }
                }
            }

            // getting images
            coll=domDoc.images();
            if (! coll.isNull() )
            {
                int size=coll.length();
                for(int i=0;i<size;i++)
                {
                    n=coll.item(i);
                    if ( !n.isNull() )
                    {
                        objRects.push_back(
                                new Okular::ObjectRect ( Okular::NormalizedRect(n.getRect(),xScale,yScale),
                                false,
                                Okular::ObjectRect::Image,
                                0));
                    }
                }
            }
            m_request->page()->setObjectRects( objRects );
        }

        if ( genTextPage )
        {
            Okular::TextPage *tp=new Okular::TextPage();
            recursiveExploreNodes(domDoc,tp);
            page->setTextPage (tp);
        }
    }
}

Okular::TextPage* CHMGenerator::textPage( Okular::Page * page )
{
    bool ok = true;
    userMutex()->lock();
    double zoomP = documentMetaData( "ZoomFactor" ).toInt( &ok );
    int zoom = ok ? qRound( zoomP * 100 ) : 100;
    m_syncGen->view()->resize(qRound( page->width() * zoomP ) , qRound( page->height() * zoomP ));
    preparePageForSyncOperation(zoom, m_file->getUrlForPage ( page->number() + 1 ));
    Okular::TextPage *tp=new Okular::TextPage();
    recursiveExploreNodes( m_syncGen->htmlDocument(), tp);
    userMutex()->unlock();
    return tp;
}

QVariant CHMGenerator::metaData( const QString &key, const QVariant &option ) const
{
    if ( key == "NamedViewport" && !option.toString().isEmpty() )
    {
        Okular::DocumentViewport viewport;
        viewport.pageNumber = m_file->getPageNum( option.toString() ) -1;
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    else if ( key == "DocumentTitle" )
    {
        return m_file->Title();
    }
    return QVariant();
}

#include "generator_chm.moc"
