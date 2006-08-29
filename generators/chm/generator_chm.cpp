/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymański <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_chm.h"
#include "lib/xchmfile.h"
#include "settings.h"
#include "core/page.h"
#include "core/link.h"
#include "core/observer.h" //for PAGEVIEW_ID
#include "dom/html_misc.h"

#include <kurl.h>
#include <kglobal.h>
#include <klocale.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <qevent.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qprinter.h>
#include <qstring.h>

OKULAR_EXPORT_PLUGIN(CHMGenerator)

CHMGenerator::CHMGenerator( KPDFDocument * doc ) : Generator ( doc )
{
    m_syncGen=0;
    m_file=0;
    m_state=-1;
    m_docInfo=0;
//     m_asyncGen=0;
//     px=0;
    m_pixmapRequestZoom=1;
}

bool CHMGenerator::loadDocument( const QString & fileName, QVector< KPDFPage * > & pagesVector )
{
    m_fileName=fileName;
    m_file=new CHMFile (fileName);
    m_file->ParseAndFillTopicsTree (&m_docSyn);

    QPrinter p; 
    p.setPageSize(static_cast< QPrinter::PageSize >( KGlobal::locale()->pageSize() ));
    p.setFullPage(true);

    kDebug () << "UrlPage count " << m_file->m_UrlPage.count() << endl;
    pagesVector.resize(m_file->m_UrlPage.count());

    if (!m_syncGen)
    {
        m_syncGen = new KHTMLPart();
        connect (m_syncGen,SIGNAL(completed()),this,SLOT(slotCompleted()));
    }
    QMap <QString, int>::ConstIterator it=m_file->m_UrlPage.begin(), end=m_file->m_UrlPage.end();
    m_state=0;
    for (;it!=end;++it)
    {
        preparePageForSyncOperation(100,it.key());
        int i= it.value() - 1;
        pagesVector[ i ] = new KPDFPage (i, m_syncGen->view()->contentsWidth(),
            m_syncGen->view()->contentsHeight(),0);
        kDebug() << "W/H: " << m_syncGen->view()->contentsWidth() << "/" << m_syncGen->view()->contentsHeight() << endl;
    }
    return true;
}

bool CHMGenerator::closeDocument()
{
    // delete the document information of the old document
    delete m_docInfo;
    m_docInfo=0;

    return true;
}

void CHMGenerator::preparePageForSyncOperation( int zoom , const QString & url)
{
    KUrl pAddress= "ms-its:" + m_fileName + "::" + url;
    m_state=0;
    kDebug() << "Url: " << pAddress  << endl;
    m_syncGen->setZoomFactor(zoom);
    m_doneFlagSet=false;
    m_syncGen->openUrl(pAddress);
    m_syncGen->view()->layout();
    while (!m_doneFlagSet) { qApp->processEvents(QEventLoop::AllEvents, 50); }
}

void CHMGenerator::slotCompleted()
{
    kDebug() << "completed() " << m_state << endl;
    if (m_state==0)
    {
        m_doneFlagSet=true;
    }
    else if (m_state==1)
    {
//         kDebug() << "completed(1) " << m_request->id << endl;
        QPixmap* pix=new QPixmap (m_request->width,m_request->height);
        pix->fill();
        QPainter p (pix);
        QRect r(0,0,m_request->width,m_request->height);
        bool moreToPaint;
//                 m_syncGen->view()->layout();
        m_syncGen->paint(&p, r,0,&moreToPaint);
        p.end();
        if (m_pixmapRequestZoom>1)
        {
            QPixmap* newpix=new QPixmap();
            *newpix=pix->scaled(m_request->width/m_pixmapRequestZoom, m_request->height/m_pixmapRequestZoom);
            delete pix;
            pix=newpix;
            m_pixmapRequestZoom=1;
        }
        additionalRequestData();
        syncLock.unlock();
        m_request->page->setPixmap( m_request->id, pix );
        signalRequestDone( m_request );
    }
}

const DocumentInfo * CHMGenerator::generateDocumentInfo() 
{
    if (!m_docInfo)
    {
        m_docInfo=new DocumentInfo();

        m_docInfo->set( "mimeType", "application/x-chm" );

        m_docInfo->set( "title", m_file->Title(), i18n("Title") );
    }
    return m_docInfo;
}

const DocumentSynopsis * CHMGenerator::generateDocumentSynopsis()
{
    return &m_docSyn;
}

const DocumentFonts * CHMGenerator::generateDocumentFonts() 
{
    return 0L;
}

bool CHMGenerator::isAllowed( int /*Document::Permisison(s)*/ )
{
    return true;
}

bool CHMGenerator::canGeneratePixmap ( bool /*async*/ )
{
/*    if (async)
    {
        kDebug() << "async is locked " << asyncLock.locked() << endl;
        return !asyncLock.locked();
    }*/
    return !syncLock.locked();
}

void CHMGenerator::generatePixmap( PixmapRequest * request ) 
{
    QString a="S";
    if (request->async) a="As";

    kDebug() << a << "ync PixmapRequest of " << request->width << "x" 
    << request->height << " size, pageNo " << request->pageNumber 
    << ", priority: " << request->priority << " id: " << request->id
    <<  endl;

    if (request->width<300)
    {
        m_pixmapRequestZoom=900/request->width;
        request->width*=m_pixmapRequestZoom;
        request->height*=m_pixmapRequestZoom;
    }

    syncLock.lock();
    QString url= m_file->getUrlForPage ( request->pageNumber + 1 );
    int zoom = qMax( static_cast<double>(request->width)/static_cast<double>(request->page->width())
        , static_cast<double>(request->height)/static_cast<double>(request->page->height())
        ) * 100;

    KUrl pAddress= "ms-its:" + m_fileName + "::" + url;
    kDebug() << "Page asked is: " << pAddress << " zoom is " << zoom << endl;
    m_syncGen->setZoomFactor(zoom);
    m_syncGen->view()->resize(request->width,request->height);
    m_request=request;
    m_state=1;
    // will emit openURL without problems
    m_syncGen->openUrl ( pAddress );
}


void CHMGenerator::recursiveExploreNodes(DOM::Node node,KPDFTextPage *tp)
{
    if (node.nodeType() == DOM::Node::TEXT_NODE)
    {
        QString nodeText=node.nodeValue().string();
        QRect r=node.getRect();
        int vWidth=m_syncGen->view()->contentsWidth();
        int vHeight=m_syncGen->view()->contentsHeight();
        NormalizedRect *nodeNormRect;
#define NOEXP
#ifndef NOEXP
        int x,y,height;
        int x_next,y_next,height_next;
        int nodeTextLength = nodeText.length();
        if (nodeTextLength==1)
        {
            nodeNormRect=new NormalizedRect (r,vWidth,vHeight);
            tp->append(nodeText,nodeNormRect,nodeNormRect->bottom,0,(nodeText=="\n"));
            kDebug() << "Norm Rect is [" << nodeNormRect->left << "x" << nodeNormRect->top
                << "] [" << nodeNormRect->right << "x" << nodeNormRect->bottom << "]" << endl;
            kDebug() << "Node Dom text(" << nodeText.length() << "): " << nodeText << endl;
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
                    kDebug() << "DL/L/R " << r.left() << "/" << x << "/" << x_next << endl;
                    nodeNormRect=new NormalizedRect (QRect(x,y,x_next-x-1,height),vWidth,vHeight);
                }
                else if ( i <nodeTextLength -1 )
                // i is between zero and the last element
                {
                    node.getCursor(i+1,x_next,y_next,height_next);
                    kDebug() << "L/R" << x << "/" << x_next << endl;
                    nodeNormRect=new NormalizedRect (QRect(x,y,x_next-x-1,height),vWidth,vHeight);
                }
                else
                // the last element use right rect boundary
                {
                    node.getCursor(i-1,x_next,y_next,height_next);
                    kDebug() << "L/R" << x_next << "/" << r.right() << endl;
                    nodeNormRect=new NormalizedRect (QRect(x,y,r.right()-x-1,height),vWidth,vHeight);
                }
                tp->append(QString(nodeText[i]),nodeNormRect,nodeNormRect->bottom,0,(nodeText[i]=='\n'));
                kDebug () << "Working with offset : " << i << endl;
                kDebug() << "Norm Rect is [" << nodeNormRect->left << "x" << nodeNormRect->top
                    << "] [" << nodeNormRect->right << "x" << nodeNormRect->bottom << "]" << endl;
                kDebug() << "Node Dom text(1): " << nodeText[i] << endl;
            }
        }
#else
        nodeNormRect=new NormalizedRect (r,vWidth,vHeight);
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
    KPDFPage * page=m_request->page;
    bool genObjectRects = m_request->id & (PAGEVIEW_ID | PRESENTATION_ID);
    bool genTextPage = !m_request->page->hasSearchPage() && genObjectRects;

    if (genObjectRects || genTextPage )
    {
        DOM::HTMLDocument domDoc=m_syncGen->htmlDocument();
    // only generate object info when generating a full page not a thumbnail
    if ( genObjectRects )
    {
        kDebug() << "Generating ObjRects - start" << endl;
        QLinkedList< ObjectRect * > objRects;
        int xScale=m_request->width;
        int yScale=m_request->height;
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
                    DocumentViewport viewport(getMetaData( url, QString::number(page->number() - 1) ));
                    r=n.getRect();
                    kDebug() << "Adding rect: " << url << " "  << r << endl;
                    // there is no way for us to support javascript properly
                    if (url.startsWith("JavaScript:"), Qt::CaseInsensitive)
                        continue;
                    else if (url.contains (":"))
                    {
                        objRects.push_back(
                            new ObjectRect ( NormalizedRect(r,xScale,yScale),
                            false,
                            ObjectRect::Link,
                            new KPDFLinkBrowse ( url )));
                    }
                    else
                    {
                        objRects.push_back(
                            new ObjectRect ( NormalizedRect(r,xScale,yScale),
                            false,
                            ObjectRect::Link,
                            new KPDFLinkGoto ( QString::null, viewport)));
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
                            new ObjectRect ( NormalizedRect(n.getRect(),xScale,yScale),
                            false,
                            ObjectRect::Image,
                            0));
                }
            }
        }
        m_request->page->setObjectRects( objRects );
    }

    if ( genTextPage )
    {
        kDebug() << "Generating text page - start" << endl;
        KPDFTextPage *tp=new KPDFTextPage();
        recursiveExploreNodes(domDoc,tp);
        page->setSearchPage (tp);
    }
    }
}

bool CHMGenerator::canGenerateTextPage() 
{
    return true;
}

void CHMGenerator::generateSyncTextPage( KPDFPage * page )
{
    syncLock.lock();
    double zoomP=KpdfSettings::zoomFactor();
    int zoom = zoomP * 100;
    m_syncGen->view()->resize(page->width() * zoomP , page->height() * zoomP);
    preparePageForSyncOperation(zoom, m_file->getUrlForPage ( page->number() + 1 ));
    KPDFTextPage *tp=new KPDFTextPage();
    recursiveExploreNodes( m_syncGen->htmlDocument(), tp);
    page->setSearchPage(tp);
    syncLock.unlock();
}

QString CHMGenerator::getXMLFile()
{
    return QString::null;
}

void CHMGenerator::setupGUI(KActionCollection  * /*ac*/ , QToolBox * /*tBox*/ )
{ ; }

bool CHMGenerator::supportsSearching()
{
    return true;
}

bool CHMGenerator::prefersInternalSearching()
{
    return false;
}

bool CHMGenerator::supportsRotation()
{
    return false;
}

RegularAreaRect * CHMGenerator::findText( const QString & /*text*/, SearchDir /*dir*/, const bool /*strictCase*/,
    const RegularAreaRect * /*lastRect*/, KPDFPage * /*page*/)
{
    return 0L;
}

QString CHMGenerator::getText( const RegularAreaRect * /*area*/, KPDFPage * /*page*/ )
{
    return QString();
}

void CHMGenerator::setOrientation(QVector<KPDFPage*> & /*pagesVector*/, int /*orientation*/)
{ ; }

bool CHMGenerator::canConfigurePrinter( )
{
    return false;
}

bool CHMGenerator::print( KPrinter& /*printer*/ )
{
    return false;
}

QString CHMGenerator::getMetaData( const QString &key, const QString &option )
{
    if ( key == "NamedViewport" && !option.isEmpty() )
    {
        DocumentViewport viewport;
        viewport.pageNumber = m_file->getPageNum( option ) -1;
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    else if ( key == "DocumentTitle" )
    {
        return m_file->Title();
    }
    return QString::null;
}

bool CHMGenerator::reparseConfig()
{
    return false;
}

void CHMGenerator::addPages( KConfigDialog* /*dlg*/)
{ ; }

bool CHMGenerator::handleEvent (QEvent * /*event*/ )
{
    return true;
}
/*
void PixmapThreader::run()
{
    kDebug() << "starting thread\n";
    m_pix = m_gen->renderPixmap(m_req);
    QCustomEvent * readyEvent = new QCustomEvent( CHM_DATAREADY_ID );
    readyEvent->setData(m_req);
    QApplication::postEvent( m_gen , readyEvent );
}

void CHMGenerator::customEvent( QCustomEvent * e )
{
    if (e->type() == CHM_DATAREADY_ID )
    {
        PixmapRequest* request=(PixmapRequest*) e->data();
        asyncLock.unlock();
        kDebug() << "got pixmap\n";
        request->page->setPixmap( request->id, px->takePixmap() );
        signalRequestDone( request );
    }
}*/

#include "generator_chm.moc"
