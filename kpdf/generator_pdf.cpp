/***************************************************************************
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qfile.h>
#include <klocale.h>
#include <kpassdlg.h>
#include <kprinter.h>
#include <ktempfile.h>
#include <kmessagebox.h>
#include <kdebug.h>

// xpdf includes
#include "xpdf/PSOutputDev.h"
#include "xpdf/Link.h"
#include "xpdf/ErrorCodes.h"
#include "xpdf/UnicodeMap.h"
#include "xpdf/Outline.h"
#include "goo/GList.h"
//#include "xpdf/PDFDoc.h"
//#include "xpdf/GlobalParams.h"

// local includes
#include "generator_pdf.h"
#include "document.h" //for PAGEVIEW_ID
#include "page.h"
#include "settings.h"
#include "QOutputDev.h"


PDFGenerator::PDFGenerator()
    : pdfdoc( 0 ), kpdfOutputDev( 0 ), requestOnThread( 0 ),
    docInfoDirty( true ), docSynopsisDirty( true )
{
    // generate kpdfOutputDev and cache page color
    reparseConfig();
}

PDFGenerator::~PDFGenerator()
{
    // TODO wait for the end of threaded generator too and disable it
    docLock.lock();
    // remove requests in queue
    QValueList< PixmapRequest * >::iterator rIt = requestsQueue.begin(), rEnd = requestsQueue.end();
    for ( ; rIt != rEnd; rIt++ )
        delete *rIt;
    requestsQueue.clear();
    // remove other internal objects
    delete kpdfOutputDev;
    delete pdfdoc;
    docLock.unlock();
}


bool PDFGenerator::loadDocument( const QString & fileName, QValueVector<KPDFPage*> & pagesVector )
{
    // create PDFDoc for the given file
    GString *filename = new GString( QFile::encodeName( fileName ) );
    delete pdfdoc;
    pdfdoc = new PDFDoc( filename, 0, 0 );

    // if the file didn't open correctly it might be encrypted, so ask for a pass
    if ( !pdfdoc->isOk() )
    {
        if ( pdfdoc->getErrorCode() == errEncrypted )
        {
            bool first, correct;
            QString prompt;
            QCString pwd;

            first = true;
            correct = false;
            while( !correct )
            {
                if ( first )
                {
                    prompt = i18n( "Please insert the password to read the document:" );
                    first = false;
                }
                else prompt = i18n( "Incorrect password. Try again:" );
                if ( KPasswordDialog::getPassword( pwd, prompt ) == KPasswordDialog::Accepted )
                {
                    GString *pwd2;
                    pwd2 = new GString( pwd.data() );
                    pdfdoc = new PDFDoc( filename, pwd2, pwd2 );
                    delete pwd2;
                    correct = pdfdoc->isOk();
                    if ( !correct && pdfdoc->getErrorCode() != errEncrypted )
                    {
                        delete pdfdoc;
                        pdfdoc = 0;
                        return false;
                    }
                }
                else
                {
                    delete pdfdoc;
                    pdfdoc = 0;
                    return false;
                }
            }
        }
        else
        {
            delete pdfdoc;
            pdfdoc = 0;
            return false;
        }
    }

    // initialize output device for rendering current pdf
    kpdfOutputDev->startDoc( pdfdoc->getXRef() );

    // build Pages (currentPage was set -1 by deletePages)
    uint pageCount = pdfdoc->getNumPages();
    pagesVector.resize( pageCount );
    for ( uint i = 0; i < pageCount ; i++ )
        pagesVector[i] = new KPDFPage( i, pdfdoc->getPageWidth(i+1), pdfdoc->getPageHeight(i+1), pdfdoc->getPageRotate(i+1) );

    // the file has been loaded correctly
    return true;
}


const DocumentInfo * PDFGenerator::documentInfo()
{
    if ( docInfoDirty )
    {
        // compile internal structure reading properties from PDFDoc
        docInfo.author = getDocumentInfo("Author");
        docInfo.creationDate = getDocumentDate("CreationDate");
        docInfo.modificationDate = getDocumentDate("ModDate");
        docInfo.creator = getDocumentInfo("Creator");
        docInfo.keywords = getDocumentInfo("Keywords");
        docInfo.producer = getDocumentInfo("Producer");
        docInfo.subject = getDocumentInfo("Subject");
        docInfo.title = getDocumentInfo("Title");
        docInfo.mimeType = "application/pdf";
        docInfo.format = "PDF";
        if ( pdfdoc )
        {
            docInfo.formatVersion = QString::number( pdfdoc->getPDFVersion() );
            docInfo.encryption = pdfdoc->isEncrypted() ? i18n( "Encrypted" ) : i18n( "Unencrypted" );
            docInfo.optimization = pdfdoc->isLinearized() ? i18n( "Yes" ) : i18n( "No" );
        }
        else
        {
            docInfo.formatVersion = i18n( "Unknown Version" );
            docInfo.encryption = i18n( "Unknown Encryption" );
            docInfo.optimization = i18n( "Unknown Optimization" );
        }

        // if pdfdoc is valid then we cached good info -> don't cache them again
        if ( pdfdoc )
            docInfoDirty = false;
    }
    return &docInfo;
}

const DocumentSynopsis * PDFGenerator::documentSynopsis()
{
    if ( !docSynopsisDirty )
        return &docSyn;

    if ( !pdfdoc )
        return NULL;

    Outline * outline = pdfdoc->getOutline();
    if ( !outline )
        return NULL;

    GList * items = outline->getItems();
    if ( !items || items->getLength() < 1 )
        return NULL;

    docSyn = DocumentSynopsis();
    if ( items->getLength() > 0 )
        addSynopsisChildren( &docSyn, items );

    docSynopsisDirty = false;
    return &docSyn;
}

void PDFGenerator::addSynopsisChildren( QDomNode * parent, GList * items )
{
    int numItems = items->getLength();
    for ( int i = 0; i < numItems; ++i )
    {
        // iterate over every object in 'items'
        OutlineItem * outlineItem = (OutlineItem *)items->get( i );

        // create element using outlineItem's title as tagName
        QString name;
        Unicode * uniChar = outlineItem->getTitle();
        for ( int i = 0; i < outlineItem->getTitleLength(); ++i )
            name += uniChar[ i ];
        if ( name.isEmpty() )
            continue;
        QDomElement item = docSyn.createElement( name );
        parent->appendChild( item );

        // set element's attributes
        if ( kpdfOutputDev )
        {
            KPDFLink * link = kpdfOutputDev->generateLink( outlineItem->getAction() );
            if ( link && link->linkType() == KPDFLink::Goto )
            {
                KPDFLinkGoto * linkGoto = static_cast< KPDFLinkGoto * >( link );
                item.setAttribute( "Page", linkGoto->destViewport().page );
                //TODO item.setAttribute( "Position", 0 );
            }
            delete link;
        }

        // recursively descend over children
        outlineItem->open();
        GList * children = outlineItem->getKids();
        if ( children )
            addSynopsisChildren( &item, children );
    }
}


bool PDFGenerator::print( KPrinter& printer )
{
    KTempFile tf( QString::null, ".ps" );
    PSOutputDev *psOut = new PSOutputDev(tf.name().latin1(), pdfdoc->getXRef(), pdfdoc->getCatalog(), 1, pdfdoc->getNumPages(), psModePS);

    if (psOut->isOk())
    {
        std::list<int> pages;

        if (!printer.previewOnly())
        {
            QValueList<int> pageList = printer.pageList();
            QValueList<int>::const_iterator it;

            for(it = pageList.begin(); it != pageList.end(); ++it) pages.push_back(*it);
        }
        else
        {
            for(int i = 1; i <= pdfdoc->getNumPages(); i++) pages.push_back(i);
        }

        docLock.lock();
        pdfdoc->displayPages(psOut, pages, 72, 72, 0, globalParams->getPSCrop(), gFalse);
        docLock.unlock();

        // needs to be here so that the file is flushed, do not merge with the one
        // in the else
        delete psOut;
        printer.printFiles(tf.name(), true);
        return true;
    }
    else
    {
        delete psOut;
        return false;
    }
}

void PDFGenerator::requestPixmap( PixmapRequest * request, bool syncronous )
{
    //kdDebug() << "id: " << id << " is requesting pixmap for page " << page->number() << " [" << width << " x " << height << "]." << endl;
    if ( syncronous )
    {
        // in-place Pixmap generation for syncronous requests
        KPDFPage * page = request->page;
        if ( page->hasPixmap( request->id, request->width, request->height ) )
            return;

        // compute dpi used to get an image with desired width and height
        double fakeDpiX = request->width * 72.0 / page->width(),
                fakeDpiY = request->height * 72.0 / page->height();

        // setup kpdf output device: text page is generated only if we are at 72dpi.
        // since we can pre-generate the TextPage at the right res.. why not?
        bool genTextPage = !page->hasSearchPage() && (request->width == page->width()) &&
                            (request->height == page->height());
        // generate links and image rects if rendering pages on pageview
        bool genRects = request->id == PAGEVIEW_ID;
        kpdfOutputDev->setParams( request->width, request->height, genTextPage, genRects, genRects );

        docLock.lock();
        pdfdoc->displayPage( kpdfOutputDev, page->number() + 1, fakeDpiX, fakeDpiY, 0, true, genRects );
        docLock.unlock();

        page->setPixmap( request->id, kpdfOutputDev->takePixmap() );
        if ( genTextPage )
            page->setSearchPage( kpdfOutputDev->takeTextPage() );
        if ( genRects )
            page->setRects( kpdfOutputDev->takeRects() );

        // pixmap generated
        contentsChanged( request->id, request->pageNumber );

        // free memory (since we took ownership of the request)
        delete request;
    }
    else
    {
        requestsQueue.push_back( request );
        //startThreadedGeneration();
    }
}

void PDFGenerator::requestTextPage( KPDFPage * page )
{
    // build a TextPage using the lightweight KPDFTextDev generator..
    KPDFTextDev td;
    docLock.lock();
    pdfdoc->displayPage( &td, page->number()+1, 72, 72, 0, true, false );
    docLock.unlock();
    // ..and attach it to the page
    page->setSearchPage( td.takeTextPage() );
}

bool PDFGenerator::reparseConfig()
{
    // load paper color from Settings or use the white default color
    QColor color = ( (Settings::renderMode() == Settings::EnumRenderMode::Paper ) &&
                     Settings::changeColors() ) ? Settings::paperColor() : Qt::white;
    // if paper color is changed we have to rebuild every visible pixmap in addition
    // to the outputDevice. it's the 'heaviest' case, other effect are just recoloring
    // over the page rendered on 'standard' white background.
    if ( color != paperColor || !kpdfOutputDev )
    {
        paperColor = color;
        SplashColor splashCol;
        splashCol.rgb8 = splashMakeRGB8( paperColor.red(), paperColor.green(), paperColor.blue() );
        // rebuild the output device using the new paper color
        docLock.lock();
        delete kpdfOutputDev;
        kpdfOutputDev = new KPDFOutputDev( this, splashCol );
        if ( pdfdoc )
            kpdfOutputDev->startDoc( pdfdoc->getXRef() );
        docLock.unlock();
        return true;
    }
    return false;
}

KPDFLinkGoto::Viewport PDFGenerator::decodeLinkViewport( GString * namedDest, LinkDest * dest )
// note: this function is called when processing a page, when the MUTEX is already LOCKED
{
    KPDFLinkGoto::Viewport vp;
    vp.page = -1;

    if ( namedDest && !dest )
        dest = pdfdoc->findDest( namedDest );

    if ( !dest || !dest->isOk() )
        return vp;

    // get destination page number
    if ( !dest->isPageRef() )
        vp.page = dest->getPageNum() - 1;
    else
    {
        Ref ref = dest->getPageRef();
        vp.page = pdfdoc->findPage( ref.num, ref.gen ) - 1;
    }

    // get destination position (fill remaining Viewport fields)
    switch ( dest->getKind() )
    {
        case destXYZ:
/*            OD -> cvtUserToDev( dest->getLeft(), dest->getTop(), &X, &Y );
            if ( dest->getChangeLeft() )
                make hor change
            if ( dest->getChangeTop() )
                make ver change
            if ( dest->getChangeZoom() )
                make zoom change
*/          break;

        case destFit:
        case destFitB:
            vp.fitWidth = true;
            vp.fitHeight = true;
            break;

        case destFitH:
        case destFitBH:
//            read top, fit Width
            vp.fitWidth = true;
            break;

        case destFitV:
        case destFitBV:
//            read left, fit Height
            vp.fitHeight = true;
            break;

        case destFitR:
//            read and fit left,bottom,right,top
            break;
    }

    return vp;
}


QString PDFGenerator::getDocumentInfo( const QString & data ) const
{
    // [Albert] Code adapted from pdfinfo.cc on xpdf
    Object info;
    if ( !pdfdoc )
        return i18n( "Unknown" );

    pdfdoc->getDocInfo( &info );
    if ( !info.isDict() )
        return i18n( "Unknown" );

    QCString result;
    Object obj;
    GString *s1;
    GBool isUnicode;
    Unicode u;
    char buf[8];
    int i, n;
    Dict *infoDict = info.getDict();
    UnicodeMap *uMap = globalParams->getTextEncoding();

    if ( !uMap )
        return i18n( "Unknown" );

    if ( infoDict->lookup( data.latin1(), &obj )->isString() )
    {
        s1 = obj.getString();
        if ( ( s1->getChar(0) & 0xff ) == 0xfe && ( s1->getChar(1) & 0xff ) == 0xff )
        {
            isUnicode = gTrue;
            i = 2;
        }
        else
        {
            isUnicode = gFalse;
            i = 0;
        }
        while ( i < obj.getString()->getLength() )
        {
            if ( isUnicode )
            {
                u = ( ( s1->getChar(i) & 0xff ) << 8 ) | ( s1->getChar(i+1) & 0xff );
                i += 2;
            }
            else
            {
                u = s1->getChar(i) & 0xff;
                ++i;
            }
            n = uMap->mapUnicode( u, buf, sizeof( buf ) );
            result += QCString( buf, n + 1 );
        }
        obj.free();
        return result;
    }
    obj.free();
    return i18n( "Unknown" );
}

QString PDFGenerator::getDocumentDate( const QString & data ) const
{
    // [Albert] Code adapted from pdfinfo.cc on xpdf
    Object info;
    if ( !pdfdoc )
        return i18n( "Unknown Date" );

    pdfdoc->getDocInfo( &info );
    if ( !info.isDict() )
        return i18n( "Unknown Date" );

    Object obj;
    char *s;
    int year, mon, day, hour, min, sec;
    Dict *infoDict = info.getDict();
    UnicodeMap *uMap = globalParams->getTextEncoding();

    if ( !uMap )
        return i18n( "Unknown Date" );

    if ( infoDict->lookup( data.latin1(), &obj )->isString() )
    {
        s = obj.getString()->getCString();
        if ( s[0] == 'D' && s[1] == ':' )
            s += 2;
        if ( sscanf( s, "%4d%2d%2d%2d%2d%2d", &year, &mon, &day, &hour, &min, &sec ) == 6 )
        {
            QDate d( year, mon, day );  //CHECK: it was mon-1, Jan->0 (??)
            QTime t( hour, min, sec );
            if ( d.isValid() && t.isValid() )
                return KGlobal::locale()->formatDateTime( QDateTime(d, t), false, true );
            else
                return s;
        }
        else
            return s;
        fputc( '\n', stdout );
        obj.free();
        //return result;
    }
    obj.free();
    return i18n( "Unknown Date" );
}

/*
#include <qapplication.h>
#include <qevent.h>
#include <qimage.h>

ThumbnailGenerator::ThumbnailGenerator(PDFDoc *doc, QMutex *docMutex, int page, double ppp, QObject *o) : m_doc(doc), m_docMutex(docMutex), m_page(page), m_o(o), m_ppp(ppp)
{
}

int ThumbnailGenerator::getPage() const
{
    return m_page;
}

void ThumbnailGenerator::run()
{
    QCustomEvent *ce;
    //QImage *i;
    
    SplashColor paperColor;
    paperColor.rgb8 = splashMakeRGB8(0xff, 0xff, 0xff);
    KPDFOutputDev odev(paperColor);
    odev.startDoc(m_doc->getXRef());
    m_docMutex->lock();
    m_doc -> displayPage(&odev, m_page, m_ppp, m_ppp, 0, true, false);
    m_docMutex->unlock();
    ce = new QCustomEvent(65432);
    //i = new QImage(odev.getImage());
    //i -> detach();
    //ce -> setData(i);
    QApplication::postEvent(m_o, ce);
}
*/
/** TO BE IMPORTED:
       void generateThumbnails(PDFDoc *doc);
       void stopThumbnailGeneration();
protected slots:
       void customEvent(QCustomEvent *e);
private slots:
       void changeSelected(int i);
       void emitClicked(int i);
signals:
       void clicked(int);
private:
       void generateNextThumbnail();
       ThumbnailGenerator *m_tg;

       void resizeThumbnails();
       int m_nextThumbnail;
       bool m_ignoreNext;

DELETE:
if (m_tg)
{
       m_tg->wait();
       delete m_tg;
}

void ThumbnailList::generateThumbnails(PDFDoc *doc)
{
       m_nextThumbnail = 1;
       m_doc = doc;
       generateNextThumbnail();
}

void ThumbnailList::generateNextThumbnail()
{
       if (m_tg)
       {
              m_tg->wait();
              delete m_tg;
       }
       m_tg = new ThumbnailGenerator(m_doc, m_docMutex, m_nextThumbnail, QPaintDevice::x11AppDpiX(), this);
       m_tg->start();
}

void ThumbnailList::stopThumbnailGeneration()
{
       if (m_tg)
       {
              m_ignoreNext = true;
              m_tg->wait();
              delete m_tg;
              m_tg = 0;
       }
}

void ThumbnailList::customEvent(QCustomEvent *e)
{
       if (e->type() == 65432 && !m_ignoreNext)
       {
              QImage *i =  (QImage*)(e -> data());

              setThumbnail(m_nextThumbnail, i);
              m_nextThumbnail++;
              if (m_nextThumbnail <= m_doc->getNumPages()) generateNextThumbnail();
              else
              {
                     m_tg->wait();
                     delete m_tg;
                     m_tg = 0;
              }
       }
       m_ignoreNext = false;
}
*/
