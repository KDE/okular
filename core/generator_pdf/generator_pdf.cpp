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
#include <qevent.h>
#include <qimage.h>
#include <qapplication.h>
#include <klocale.h>
#include <kpassdlg.h>
#include <kprinter.h>
#include <ktempfile.h>
#include <kmessagebox.h>
#include <kdebug.h>

// xpdf includes
#include "xpdf/PSOutputDev.h"
#include "xpdf/TextOutputDev.h"
#include "xpdf/Link.h"
#include "xpdf/ErrorCodes.h"
#include "xpdf/UnicodeMap.h"
#include "xpdf/Outline.h"
#include "goo/GList.h"

// local includes
#include "generator_pdf.h"
#include "gp_outputdev.h"
#include "core/observer.h" //for PAGEVIEW_ID
#include "core/page.h"
#include "core/pagetransition.h"
#include "conf/settings.h"

// id for DATA_READY PDFPixmapGeneratorThread Event
#define TGE_DATAREADY_ID 6969

/** NOTES on threading:
 * internal: thread race prevention is done via the 'docLock' mutex. the
 *           mutex is needed only because we have the asyncronous thread; else
 *           the operations are all within the 'gui' thread, scheduled by the
 *           Qt scheduler and no mutex is needed.
 * external: dangerous operations are all locked via mutex internally, and the
 *           only needed external thing is the 'canGeneratePixmap' method
 *           that tells if the generator is free (since we don't want an
 *           internal queue to store PixmapRequests). A generatedPixmap call
 *           without the 'ready' flag set, results in undefined behavior.
 * So, as example, printing while generating a pixmap asyncronously is safe,
 * it might only block the gui thread by 1) waiting for the mutex to unlock
 * in async thread and 2) doing the 'heavy' print operation.
 */

PDFGenerator::PDFGenerator( KPDFDocument * doc )
    : Generator( doc ), pdfdoc( 0 ), kpdfOutputDev( 0 ), ready( true ),
    pixmapRequest( 0 ), docInfoDirty( true ), docSynopsisDirty( true )
{
    // generate kpdfOutputDev and cache page color
    reparseConfig();
    // generate the pixmapGeneratorThread
    generatorThread = new PDFPixmapGeneratorThread( this );
}

PDFGenerator::~PDFGenerator()
{
    // first stop and delete the generator thread
    if ( generatorThread )
    {
        generatorThread->wait();
        delete generatorThread;
    }
    // remove other internal objects
    docLock.lock();
    delete kpdfOutputDev;
    delete pdfdoc;
    docLock.unlock();
}


//BEGIN Generator inherited functions
bool PDFGenerator::loadDocument( const QString & fileName, QValueVector<KPDFPage*> & pagesVector )
{
#ifndef NDEBUG
    if ( pdfdoc )
    {
        kdDebug() << "PDFGenerator: multiple calls to loadDocument. Check it." << endl;
        return false;
    }
#endif
    // create PDFDoc for the given file
    pdfdoc = new PDFDoc( new GString( QFile::encodeName( fileName ) ), 0, 0 );

    // if the file didn't open correctly it might be encrypted, so ask for a pass
    bool firstTry = true;
    while ( !pdfdoc->isOk() && pdfdoc->getErrorCode() == errEncrypted )
    {
        QString prompt;
        if ( firstTry )
            prompt = i18n( "Please insert the password to read the document:" );
        else
            prompt = i18n( "Incorrect password. Try again:" );
        firstTry = false;

        QCString pwd;
        if ( KPasswordDialog::getPassword( pwd, prompt ) != KPasswordDialog::Accepted )
            break;
        else
        {
            GString * pwd2 = new GString( pwd.data() );
            delete pdfdoc;
            pdfdoc = new PDFDoc( new GString( QFile::encodeName( fileName ) ), pwd2, pwd2 );
            delete pwd2;
        }
    }
    if ( !pdfdoc->isOk() )
    {
        delete pdfdoc;
        pdfdoc = 0;
        return false;
    }

    // initialize output device for rendering current pdf
    kpdfOutputDev->initDevice( pdfdoc );

    // build Pages (currentPage was set -1 by deletePages)
    uint pageCount = pdfdoc->getNumPages();
    pagesVector.resize( pageCount );
    for ( uint i = 0; i < pageCount ; i++ )
    {
        KPDFPage * page = new KPDFPage( i, pdfdoc->getPageWidth(i+1),
                                        pdfdoc->getPageHeight(i+1),
                                        pdfdoc->getPageRotate(i+1) );
        addTransition( i, page );
        pagesVector[i] = page;
    }

    // the file has been loaded correctly
    return true;
}


const DocumentInfo * PDFGenerator::generateDocumentInfo()
{
    if ( docInfoDirty )
    {
        docLock.lock();
        // compile internal structure reading properties from PDFDoc
        docInfo.set( "title", getDocumentInfo("Title"), i18n("Title") );
        docInfo.set( "subject", getDocumentInfo("Subject"), i18n("Subject") );
        docInfo.set( "author", getDocumentInfo("Author"), i18n("Author") );
        docInfo.set( "keywords", getDocumentInfo("Keywords"), i18n("Keywords") );
        docInfo.set( "creator", getDocumentInfo("Creator"), i18n("Creator") );
        docInfo.set( "producer", getDocumentInfo("Producer"), i18n("Producer") );
        docInfo.set( "creationDate", getDocumentDate("CreationDate"), i18n("Created") );
        docInfo.set( "modificationDate", getDocumentDate("ModDate"), i18n("Modified") );
        docInfo.set( "mimeType", "application/pdf" );
        if ( pdfdoc )
        {
            docInfo.set( "format", i18n( "PDF v. <version>", "PDF v. %1" )
                         .arg( QString::number( pdfdoc->getPDFVersion() ) ), i18n( "Format" ) );
            docInfo.set( "encryption", pdfdoc->isEncrypted() ? i18n( "Encrypted" ) : i18n( "Unencrypted" ),
                         i18n("Security") );
            docInfo.set( "optimization", pdfdoc->isLinearized() ? i18n( "Yes" ) : i18n( "No" ),
                         i18n("Optimized") );
        }
        else
        {
            docInfo.set( "format", "PDF", i18n( "Format" ) );
            docInfo.set( "encryption", i18n( "Unknown Encryption" ), i18n( "Security" ) );
            docInfo.set( "optimization", i18n( "Unknown Optimization" ), i18n( "Optimized" ) );
        }
        docInfo.set( "pages", QString::number( pdfdoc->getCatalog()->getNumPages() ), i18n("Pages") );
        docLock.unlock();

        // if pdfdoc is valid then we cached good info -> don't cache them again
        if ( pdfdoc )
            docInfoDirty = false;
    }
    return &docInfo;
}

const DocumentSynopsis * PDFGenerator::generateDocumentSynopsis()
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

    docLock.lock();
    docSyn = DocumentSynopsis();
    if ( items->getLength() > 0 )
        addSynopsisChildren( &docSyn, items );
    docLock.unlock();

    docSynopsisDirty = false;
    return &docSyn;
}

bool PDFGenerator::canGeneratePixmap()
{
    return ready;
}

void PDFGenerator::generatePixmap( PixmapRequest * request )
{
#ifndef NDEBUG
    if ( !ready )
        kdDebug() << "calling generatePixmap() when not in READY state!" << endl;
#endif
    // update busy state (not really needed here, because the flag needs to
    // be set only to prevent asking a pixmap while the thread is running)
    ready = false;

    // debug requests to this (xpdf) generator
    //kdDebug() << "id: " << request->id << " is requesting " << (request->async ? "ASYNC" : "sync") <<  " pixmap for page " << request->page->number() << " [" << request->width << " x " << request->height << "]." << endl;

    /** asyncronous requests (generation in PDFPixmapGeneratorThread::run() **/
    if ( request->async )
    {
        // start the generation into the thread
        generatorThread->startGeneration( request );
        return;
    }

    /** syncronous request: in-place generation **/
    // compute dpi used to get an image with desired width and height
    KPDFPage * page = request->page;
    double fakeDpiX = request->width * 72.0 / page->width(),
           fakeDpiY = request->height * 72.0 / page->height();

    // setup kpdf output device: text page is generated only if we are at 72dpi.
    // since we can pre-generate the TextPage at the right res.. why not?
    bool genTextPage = !page->hasSearchPage() && (request->width == page->width()) &&
                       (request->height == page->height());
    // generate links and image rects if rendering pages on pageview
    bool genRects = request->id == PAGEVIEW_ID;

    // 0. LOCK [waits for the thread end]
    docLock.lock();

    // 1. Set OutputDev parameters and Generate contents
    // note: thread safety is set on 'false' for the GUI (this) thread
    kpdfOutputDev->setParams( request->width, request->height, genTextPage, genRects, genRects, false );
    pdfdoc->displayPage( kpdfOutputDev, page->number() + 1, fakeDpiX, fakeDpiY, 0, true, genRects );

    // 2. Take data from outputdev and attach it to the Page
    page->setPixmap( request->id, kpdfOutputDev->takePixmap() );
    if ( genTextPage )
        page->setSearchPage( kpdfOutputDev->takeTextPage() );
    if ( genRects )
        page->setRects( kpdfOutputDev->takeRects() );

    // 3. UNLOCK [re-enables shared access]
    docLock.unlock();

    // update ready state
    ready = true;

    // notify the new generation
    signalRequestDone( request );
}

void PDFGenerator::generateSyncTextPage( KPDFPage * page )
{
    // build a TextPage using the lightweight KPDFTextDev generator..
    KPDFTextDev td;
    docLock.lock();
    pdfdoc->displayPage( &td, page->number()+1, 72, 72, 0, true, false );
    // ..and attach it to the page
    page->setSearchPage( td.takeTextPage() );
    docLock.unlock();
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

QString PDFGenerator::getMetaData( const QString & key, const QString & option )
{
    if ( key == "StartFullScreen" )
    {
        // asking for the 'start in fullscreen mode' (pdf property)
        if ( pdfdoc->getCatalog()->getPageMode() == Catalog::FullScreen )
            return "yes";
    }
    else if ( key == "NamedViewport" && !option.isEmpty() )
    {
        // asking for the page related to a 'named link destination'. the
        // option is the link name. @see addSynopsisChildren.
        DocumentViewport viewport;
        GString * namedDest = new GString( option.latin1() );
        docLock.lock();
        LinkDest * destination = pdfdoc->findDest( namedDest );
        if ( destination )
        {
            if ( !destination->isPageRef() )
                viewport.pageNumber = destination->getPageNum() - 1;
            else
            {
                Ref ref = destination->getPageRef();
                viewport.pageNumber = pdfdoc->findPage( ref.num, ref.gen ) - 1;
            }
        }
        docLock.unlock();
        delete namedDest;
        if ( viewport.pageNumber >= 0 )
            return viewport.toString();
    }
    return QString();
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
        // rebuild the output device using the new paper color and initialize it
        docLock.lock();
        delete kpdfOutputDev;
        kpdfOutputDev = new KPDFOutputDev( splashCol );
        if ( pdfdoc )
            kpdfOutputDev->initDevice( pdfdoc );
        docLock.unlock();
        return true;
    }
    return false;
}
//END Generator inherited functions


QString PDFGenerator::getDocumentInfo( const QString & data ) const
// note: MUTEX is LOCKED while calling this
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
// note: MUTEX is LOCKED while calling this
{
    // [Albert] Code adapted from pdfinfo.cc on xpdf
    if ( !pdfdoc )
        return i18n( "Unknown Date" );

    Object info;
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

void PDFGenerator::addSynopsisChildren( QDomNode * parent, GList * items )
{
    int numItems = items->getLength();
    for ( int i = 0; i < numItems; ++i )
    {
        // iterate over every object in 'items'
        OutlineItem * outlineItem = (OutlineItem *)items->get( i );

        // 1. create element using outlineItem's title as tagName
        QString name;
        Unicode * uniChar = outlineItem->getTitle();
        int titleLength = outlineItem->getTitleLength();
        for ( int j = 0; j < titleLength; ++j )
            name += uniChar[ j ];
        if ( name.isEmpty() )
            continue;
        QDomElement item = docSyn.createElement( name );
        parent->appendChild( item );

        // 2. find the page the link refers to
        LinkAction * a = outlineItem->getAction();
        if ( a && a->getKind() == actionGoTo )
        {
            // page number is contained/referenced in a LinkGoTo
            LinkGoTo * g = static_cast< LinkGoTo * >( a );
            LinkDest * destination = g->getDest();
            if ( !destination && g->getNamedDest() )
            {
                // no 'destination' but an internal 'named reference'. we could
                // get the destination for the page now, but it's VERY time consuming,
                // so better storing the reference and provide the viewport as metadata
                // on demand
                item.setAttribute( "ViewportName", g->getNamedDest()->getCString() );
            }
            else if ( destination->isOk() )
            {
                // we have valid 'destination' -> get page number
                int pageNumber = destination->getPageNum() - 1;
                if ( destination->isPageRef() )
                {
                    Ref ref = destination->getPageRef();
                    pageNumber = pdfdoc->findPage( ref.num, ref.gen ) - 1;
                }
                // set page as attribute to node
                // TODO add other attributes to the viewport (taken from link)
                item.setAttribute( "Viewport", DocumentViewport( pageNumber ).toString() );
            }
        }

        // 3. recursively descend over children
        outlineItem->open();
        GList * children = outlineItem->getKids();
        if ( children )
            addSynopsisChildren( &item, children );
    }
}

void PDFGenerator::addTransition( int pageNumber, KPDFPage * page )
{
    Page *pdfPage = pdfdoc->getCatalog()->getPage( pageNumber + 1 );
    if ( !pdfPage )
        return;

    PageTransition *pdfTransition = pdfPage->getTransition();
    if ( !pdfTransition || pdfTransition->getType() == PageTransition::Replace )
        return;

    KPDFPageTransition *transition = new KPDFPageTransition();
    switch ( pdfTransition->getType() ) {
        case PageTransition::Replace:
            // won't get here, added to avoid warning
            break;
        case PageTransition::Split:
            transition->setType( KPDFPageTransition::Split );
            break;
        case PageTransition::Blinds:
            transition->setType( KPDFPageTransition::Blinds );
            break;
        case PageTransition::Box:
            transition->setType( KPDFPageTransition::Box );
            break;
        case PageTransition::Wipe:
            transition->setType( KPDFPageTransition::Wipe );
            break;
        case PageTransition::Dissolve:
            transition->setType( KPDFPageTransition::Dissolve );
            break;
        case PageTransition::Glitter:
            transition->setType( KPDFPageTransition::Glitter );
            break;
        case PageTransition::Fly:
            transition->setType( KPDFPageTransition::Fly );
            break;
        case PageTransition::Push:
            transition->setType( KPDFPageTransition::Push );
            break;
        case PageTransition::Cover:
            transition->setType( KPDFPageTransition::Cover );
            break;
        case PageTransition::Uncover:
            transition->setType( KPDFPageTransition::Uncover );
            break;
        case PageTransition::Fade:
            transition->setType( KPDFPageTransition::Fade );
            break;
    }

    transition->setDuration( pdfTransition->getDuration() );

    switch ( pdfTransition->getAlignment() ) {
        case PageTransition::Horizontal:
            transition->setAlignment( KPDFPageTransition::Horizontal );
            break;
        case PageTransition::Vertical:
            transition->setAlignment( KPDFPageTransition::Vertical );
            break;
    }

    switch ( pdfTransition->getDirection() ) {
        case PageTransition::Inward:
            transition->setDirection( KPDFPageTransition::Inward );
            break;
        case PageTransition::Outward:
            transition->setDirection( KPDFPageTransition::Outward );
            break;
    }

    transition->setAngle( pdfTransition->getAngle() );
    transition->setScale( pdfTransition->getScale() );
    transition->setIsRectangular( pdfTransition->isRectangular() == gTrue );

    page->setTransition( transition );
}



void PDFGenerator::customEvent( QCustomEvent * event )
{
    // catch generator 'ready events' only
    if ( event->type() != TGE_DATAREADY_ID )
        return;

#if 0
    // check if thread is running (has to be stopped now)
    if ( generatorThread->running() )
    {
        // if so, wait for effective thread termination
        if ( !generatorThread->wait( 9999 /*10s timeout*/ ) )
        {
            kdWarning() << "PDFGenerator: thread sent 'data available' "
                        << "signal but had problems ending." << endl;
            return;
        }
}
#endif

    // 1. the mutex must be unlocked now
    if ( docLock.locked() )
    {
        kdWarning() << "PDFGenerator: 'data available' but mutex still "
                    << "held. Recovering." << endl;
        // syncronize GUI thread (must not happen)
        docLock.lock();
        docLock.unlock();
    }

    // 2. put thread's generated data into the KPDFPage
    PixmapRequest * request = static_cast< PixmapRequest * >( event->data() );
    QImage * outImage = generatorThread->takeImage();
    TextPage * outTextPage = generatorThread->takeTextPage();
    QValueList< KPDFPageRect * > outRects = generatorThread->takeRects();

    request->page->setPixmap( request->id, new QPixmap( *outImage ) );
    delete outImage;
    if ( outTextPage )
        request->page->setSearchPage( outTextPage );
    if ( !outRects.isEmpty() )
        request->page->setRects( outRects );

    // 3. tell generator that data has been taken
    generatorThread->endGeneration();

    // update ready state
    ready = true;
    // notify the new generation
    signalRequestDone( request );
}



/** The  PDF Pixmap Generator Thread  **/

struct PPGThreadPrivate
{
    // reference to main objects
    PDFGenerator * generator;
    PixmapRequest * currentRequest;

    // internal temp stored items. don't delete this.
    QImage * m_image;
    TextPage * m_textPage;
    QValueList< KPDFPageRect * > m_rects;
    bool m_rectsTaken;
};

PDFPixmapGeneratorThread::PDFPixmapGeneratorThread( PDFGenerator * gen )
    : d( new PPGThreadPrivate() )
{
    d->generator = gen;
    d->currentRequest = 0;
    d->m_image = 0;
    d->m_textPage = 0;
    d->m_rectsTaken = true;
}

PDFPixmapGeneratorThread::~PDFPixmapGeneratorThread()
{
    // delete internal objects if the class is deleted before the gui thread
    // takes the data
    delete d->m_image;
    delete d->m_textPage;
    if ( !d->m_rectsTaken && d->m_rects.count() )
    {
        QValueList< KPDFPageRect * >::iterator it = d->m_rects.begin(), end = d->m_rects.end();
        for ( ; it != end; ++it )
            delete *it;
    }
    // delete internal storage structure
    delete d;
}

void PDFPixmapGeneratorThread::startGeneration( PixmapRequest * request )
{
#ifndef NDEBUG
    // check if a generation is already running
    if ( d->currentRequest )
    {
        kdDebug() << "PDFPixmapGeneratorThread: requesting a pixmap "
                  << "when another is being generated." << endl;
        delete request;
        return;
    }

    // check if the mutex is already held
    if ( d->generator->docLock.locked() )
    {
        kdDebug() << "PDFPixmapGeneratorThread: requesting a pixmap "
                  << "with the mutex already held." << endl;
        delete request;
        return;
    }
#endif
    // set generation parameters and run thread
    d->currentRequest = request;
    start( QThread::InheritPriority );
}

void PDFPixmapGeneratorThread::endGeneration()
{
#ifndef NDEBUG
    // check if a generation is already running
    if ( !d->currentRequest )
    {
        kdDebug() << "PDFPixmapGeneratorThread: 'end generation' called "
                  << "but generation was not started." << endl;
        return;
    }
#endif
    // reset internal members preparing for a new generation
    d->currentRequest = 0;
}

QImage * PDFPixmapGeneratorThread::takeImage() const
{
    QImage * img = d->m_image;
    d->m_image = 0;
    return img;
}

TextPage * PDFPixmapGeneratorThread::takeTextPage() const
{
    TextPage * tp = d->m_textPage;
    d->m_textPage = 0;
    return tp;
}

QValueList< KPDFPageRect * > PDFPixmapGeneratorThread::takeRects() const
{
    d->m_rectsTaken = true;
    return d->m_rects;
}

void PDFPixmapGeneratorThread::run()
// perform contents generation, when the MUTEX is already LOCKED
// @see PDFGenerator::generatePixmap( .. ) (and be aware to sync the code)
{
    // compute dpi used to get an image with desired width and height
    KPDFPage * page = d->currentRequest->page;
    int width = d->currentRequest->width,
        height = d->currentRequest->height;
    double fakeDpiX = width * 72.0 / page->width(),
           fakeDpiY = height * 72.0 / page->height();

    // setup kpdf output device: text page is generated only if we are at 72dpi.
    // since we can pre-generate the TextPage at the right res.. why not?
    bool genTextPage = !page->hasSearchPage() &&
                       ( width == page->width() ) &&
                       ( height == page->height() );

    // generate links and image rects if rendering pages on pageview
    bool genPageRects = d->currentRequest->id == PAGEVIEW_ID;

    // 0. LOCK s[tart locking XPDF thread unsafe classes]
    d->generator->docLock.lock();

    // 1. set OutputDev parameters and Generate contents
    d->generator->kpdfOutputDev->setParams( width, height, genTextPage,
                                           genPageRects, genPageRects, TRUE /*thread safety*/ );
    d->generator->pdfdoc->displayPage( d->generator->kpdfOutputDev, page->number() + 1,
                                      fakeDpiX, fakeDpiY, 0, true, genPageRects );

    // 2. grab data from the OutputDev and store it locally (note takeIMAGE)
#ifndef NDEBUG
    if ( d->m_image )
        kdDebug() << "PDFPixmapGeneratorThread: previous image not taken" << endl;
    if ( d->m_textPage )
        kdDebug() << "PDFPixmapGeneratorThread: previous textpage not taken" << endl;
#endif
    d->m_image = d->generator->kpdfOutputDev->takeImage();
    d->m_textPage = d->generator->kpdfOutputDev->takeTextPage();
    d->m_rects = d->generator->kpdfOutputDev->takeRects();
    d->m_rectsTaken = false;

    // 3. [UNLOCK] mutex
    d->generator->docLock.unlock();

    // notify the GUI thread that data is pending and can be read
    QCustomEvent * readyEvent = new QCustomEvent( TGE_DATAREADY_ID );
    readyEvent->setData( d->currentRequest );
    QApplication::postEvent( d->generator, readyEvent );
}
