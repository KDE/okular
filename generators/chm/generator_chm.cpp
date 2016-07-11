/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymański <niedakh@gmail.com>             *
 *   Copyright (C) 2008 by Albert Astals Cid <aacid@kde.org>               *
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
#include <QtXml/QDomElement>

#include <KAboutData>
#include <khtml_part.h>
#include <khtmlview.h>
#include <KLocalizedString>
#include <QUrl>
#include <dom/html_misc.h>
#include <dom/dom_node.h>
#include <dom/dom_html.h>

#include <core/action.h>
#include <core/page.h>
#include <core/textpage.h>
#include <core/utils.h>

OKULAR_EXPORT_PLUGIN(CHMGenerator, "libokularGenerator_chmlib.json")

static QString absolutePath( const QString &baseUrl, const QString &path )
{
    QString absPath;
    if ( path.startsWith(QLatin1Char( '/' )) )
    {
        // already absolute
        absPath = path;
    }
    else
    {
        QUrl url = QUrl::fromLocalFile( baseUrl ).adjusted(QUrl::RemoveFilename);
        url.setPath( url.path() + path );
        absPath = url.toLocalFile();
    }
    return absPath;
}

CHMGenerator::CHMGenerator( QObject *parent, const QVariantList &args )
    : Okular::Generator( parent, args )
{
    setFeature( TextExtraction );

    m_syncGen=0;
    m_file=0;
    m_pixmapRequestZoom=1;
    m_request = 0;
}

CHMGenerator::~CHMGenerator()
{
    delete m_syncGen;
}

bool CHMGenerator::loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector )
{
    m_file = new LCHMFile();
    if (!m_file->loadFile(fileName))
    {
        delete m_file;
        m_file = 0;
        return false;
    }
    m_fileName=fileName;
    QVector< LCHMParsedEntry > topics;
    m_file->parseTableOfContents(&topics);
    
    // fill m_docSyn
    QMap<int, QDomElement> lastIndentElement;
    foreach(const LCHMParsedEntry &e, topics)
    {
        QDomElement item = m_docSyn.createElement(e.name);
        if (!e.urls.isEmpty())
        {
            QString url = e.urls.first();
            if (url.contains(QChar::fromLatin1('%')))
                url = QString::fromUtf8(QByteArray::fromPercentEncoding(url.toUtf8()));
            item.setAttribute(QStringLiteral("ViewportName"), url);
        }
        item.setAttribute(QStringLiteral("Icon"), e.imageid);
        if (e.indent == 0) m_docSyn.appendChild(item);
        else lastIndentElement[e.indent - 1].appendChild(item);
        lastIndentElement[e.indent] = item;
    }
    
    // fill m_urlPage and m_pageUrl
    int pageNum = 0;
    QStringList pageList;
    m_file->enumerateFiles(&pageList);
    const QString home = m_file->homeUrl();
    if (home != QLatin1String("/"))
        pageList.prepend(home);
    foreach (const QString &url, pageList)
    {
        const QString urlLower = url.toLower();
        if (!urlLower.endsWith(QLatin1String(".html")) && !urlLower.endsWith(QLatin1String(".htm")))
            continue;

        int pos = url.indexOf (QLatin1Char(('#')));
        QString tmpUrl = pos == -1 ? url : url.left(pos);

        // url already there, abort insertion
        if (m_urlPage.contains(tmpUrl)) continue;

        // insert the url into the maps, but insert always the variant without the #ref part
        m_urlPage.insert(tmpUrl, pageNum);
        m_pageUrl.append(tmpUrl);
        pageNum++;
    }

    pagesVector.resize(m_pageUrl.count());
    m_textpageAddedList.fill(false, pagesVector.count());
    m_rectsGenerated.fill(false, pagesVector.count());

    if (!m_syncGen)
    {
        m_syncGen = new KHTMLPart();
    }
    disconnect( m_syncGen, 0, this, 0 );

    for (int i = 0; i < m_pageUrl.count(); ++i)
    {
        preparePageForSyncOperation(100, m_pageUrl.at(i));
        pagesVector[ i ] = new Okular::Page (i, m_syncGen->view()->contentsWidth(),
            m_syncGen->view()->contentsHeight(), Okular::Rotation0 );
    }

    connect( m_syncGen, SIGNAL(completed()), this, SLOT(slotCompleted()) );
    connect( m_syncGen, &KParts::ReadOnlyPart::canceled, this, &CHMGenerator::slotCompleted );

    return true;
}

bool CHMGenerator::doCloseDocument()
{
    // delete the document information of the old document
    delete m_file;
    m_file=0;
    m_textpageAddedList.clear();
    m_rectsGenerated.clear();
    m_urlPage.clear();
    m_pageUrl.clear();
    m_docSyn.clear();
    if (m_syncGen)
    {
        m_syncGen->closeUrl();
    }

    return true;
}

void CHMGenerator::preparePageForSyncOperation( int zoom , const QString & url)
{
    QString pAddress= QStringLiteral("ms-its:") + m_fileName + QStringLiteral("::") + url;
    m_chmUrl = url;
    m_syncGen->setZoomFactor(zoom);
    m_syncGen->openUrl(QUrl(pAddress));
    m_syncGen->view()->layout();

    QEventLoop loop;
    connect( m_syncGen, SIGNAL(completed()), &loop, SLOT(quit()) );
    connect( m_syncGen, &KParts::ReadOnlyPart::canceled, &loop, &QEventLoop::quit );
    // discard any user input, otherwise it breaks the "synchronicity" of this
    // function
    loop.exec( QEventLoop::ExcludeUserInputEvents );
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
    m_chmUrl = QString();

    userMutex()->unlock();

    Okular::PixmapRequest *req = m_request;
    m_request = 0;

    if ( !req->page()->isBoundingBoxKnown() )
        updatePageBoundingBox( req->page()->number(), Okular::Utils::imageBoundingBox( &image ) );
    req->page()->setPixmap( req->observer(), new QPixmap( QPixmap::fromImage( image ) ) );
    signalPixmapRequestDone( req );
}

Okular::DocumentInfo CHMGenerator::generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const
{
    Okular::DocumentInfo docInfo;
    if ( keys.contains( Okular::DocumentInfo::MimeType ) )
        docInfo.set( Okular::DocumentInfo::MimeType, QStringLiteral("application/x-chm") );
    if ( keys.contains( Okular::DocumentInfo::Title ) )
        docInfo.set( Okular::DocumentInfo::Title, m_file->title() );
    return docInfo;
}

const Okular::DocumentSynopsis * CHMGenerator::generateDocumentSynopsis()
{
    return &m_docSyn;
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
    QString url= m_pageUrl[request->pageNumber()];
    int zoom = qRound( qMax( static_cast<double>(requestWidth)/static_cast<double>(request->page()->width())
        , static_cast<double>(requestHeight)/static_cast<double>(request->page()->height())
        ) ) * 100;

    QString pAddress= QStringLiteral("ms-its:") + m_fileName + QStringLiteral("::") + url;
    m_chmUrl = url;
    m_syncGen->setZoomFactor(zoom);
    m_syncGen->view()->resize(requestWidth,requestHeight);
    m_request=request;
    // will emit openURL without problems
    m_syncGen->openUrl ( QUrl(pAddress) );
}


void CHMGenerator::recursiveExploreNodes(DOM::Node node,Okular::TextPage *tp)
{
    if (node.nodeType() == DOM::Node::TEXT_NODE && !node.getRect().isNull())
    {
        QString nodeText=node.nodeValue().string();
        QRect r=node.getRect();
        int vWidth=m_syncGen->view()->width();
        int vHeight=m_syncGen->view()->height();
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
    const bool genObjectRects = !m_rectsGenerated.at( m_request->page()->number() );
    const bool genTextPage = !m_request->page()->hasTextPage() && genObjectRects;

    if (genObjectRects || genTextPage )
    {
        DOM::HTMLDocument domDoc=m_syncGen->htmlDocument();
        // only generate object info when generating a full page not a thumbnail
        if ( genObjectRects )
        {
            QLinkedList< Okular::ObjectRect * > objRects;
            int xScale=m_syncGen->view()->width();
            int yScale=m_syncGen->view()->height();
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
                        if (url.startsWith(QLatin1String("JavaScript:")), Qt::CaseInsensitive)
                            continue;
                        else if (url.contains (QStringLiteral(":")))
                        {
                            objRects.push_back(
                                new Okular::ObjectRect ( Okular::NormalizedRect(r,xScale,yScale),
                                false,
                                Okular::ObjectRect::Action,
                                new Okular::BrowseAction ( QUrl(url) )));
                        }
                        else
                        {
                            Okular::DocumentViewport viewport( metaData( QStringLiteral("NamedViewport"), absolutePath( m_chmUrl, url ) ).toString() );
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
            m_rectsGenerated[ m_request->page()->number() ] = true;
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
    userMutex()->lock();
    const int zoom = 100;
    m_syncGen->view()->resize(page->width(), page->height());
    
    preparePageForSyncOperation(zoom, m_pageUrl[page->number()]);
    Okular::TextPage *tp=new Okular::TextPage();
    recursiveExploreNodes( m_syncGen->htmlDocument(), tp);
    userMutex()->unlock();
    return tp;
}

QVariant CHMGenerator::metaData( const QString &key, const QVariant &option ) const
{
    if ( key == QLatin1String("NamedViewport") && !option.toString().isEmpty() )
    {
        const int pos = option.toString().indexOf(QLatin1Char('#'));
        QString tmpUrl = pos == -1 ? option.toString() : option.toString().left(pos);
        Okular::DocumentViewport viewport;
        QMap<QString,int>::const_iterator it = m_urlPage.find(tmpUrl);
        if (it != m_urlPage.end())
        {
            viewport.pageNumber = it.value();
            return viewport.toString();
        }
        
    }
    else if ( key == QLatin1String("DocumentTitle") )
    {
        return m_file->title();
    }
    return QVariant();
}

/* kate: replace-tabs on; tab-width 4; */

#include "generator_chm.moc"
