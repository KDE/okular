/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004-2005 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde/system includes
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qvaluevector.h>
#include <qtimer.h>
#include <qmap.h>
#include <kdebug.h>
#include <klocale.h>
#include <kfinddialog.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kuserprofile.h>
#include <kmimetype.h>
#include <krun.h>
#include <kstandarddirs.h>

// local includes
#include "document.h"
#include "observer.h"
#include "page.h"
#include "link.h"
#include "generator_pdf/generator_pdf.h"  // PDF generator
#include "conf/settings.h"

// structures used internally by KPDFDocument for local variables storage
class KPDFDocumentPrivate
{
    public:
        // find related
        QString searchText;
        bool searchCase;
        int searchPage;
        // filtering related
        QString filterText;
        bool filterCase;

        // cached stuff
        QString docFileName;
        QString xmlFileName;

        // viewport stuff
        QValueList< DocumentViewport > viewportHistory;
        QValueList< DocumentViewport >::iterator viewportIterator;

        // observers / requests / allocator stuff
        QMap< int, DocumentObserver * > observers;
        QValueList< PixmapRequest * > pixmapRequestsStack;
        QValueList< class AllocatedPixmap * > allocatedPixmapsFifo;
        int allocatedPixmapsTotalMemory;

        // timers (memory checking / info saver)
        QTimer * memCheckTimer;
        QTimer * saveBookmarksTimer;
};

struct AllocatedPixmap
{
    // owner of the page
    int id;
    int page;
    int memory;
    // public constructor: initialize data
    AllocatedPixmap( int i, int p, int m ) : id( i ), page( p ), memory( m ) {};
};

#define foreachObserver( cmd ) {\
    QMap< int, DocumentObserver * >::iterator it=d->observers.begin(), end=d->observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }


/** KPDFDocument **/

KPDFDocument::KPDFDocument()
    : generator( 0 ), d( new KPDFDocumentPrivate )
{
    d->searchPage = -1;
    d->allocatedPixmapsTotalMemory = 0;
    d->memCheckTimer = 0;
    d->saveBookmarksTimer = 0;
}

KPDFDocument::~KPDFDocument()
{
    // delete generator, pages, and related stuff
    closeDocument();

    // delete the private structure
    delete d;
}


bool KPDFDocument::openDocument( const QString & docFile )
{
    // docFile is always local so we can use QFile on it
    QFile fileReadTest( docFile );
    if ( !fileReadTest.open( IO_ReadOnly ) )
    {
        d->docFileName = QString::null;
        return false;
    }
    // determine the related "xml document-info" filename
    d->docFileName = docFile;
    QString fn = docFile.contains('/') ? docFile.section('/', -1, -1) : docFile;
    fn = "kpdf/" + QString::number(fileReadTest.size()) + "." + fn + ".xml";
    fileReadTest.close();
    d->xmlFileName = locateLocal( "data", fn );

    // create the generator based on the file's mimetype
    KMimeType::Ptr mime = KMimeType::findByPath( docFile );
    QString mimeName = mime->name();
    if ( mimeName == "application/pdf" )
        generator = new PDFGenerator( this );
    else if ( mimeName == "application/postscript" )
        kdError() << "PS generator not available" << endl;
    else
    {
        kdWarning() << "Unknown mimetype '" << mimeName << "'." << endl;
        return false;
    }

    // 1. load Document (and set busy cursor while loading)
    QApplication::setOverrideCursor( waitCursor );
    bool openOk = generator->loadDocument( docFile, pages_vector );
    QApplication::restoreOverrideCursor();
    if ( !openOk || pages_vector.size() <= 0 )
    {
        delete generator;
        generator = 0;
        return openOk;
    }

    // 2. load Additional Data (our bookmarks and metadata) about the document
    loadDocumentInfo();

    // 3. setup observers inernal lists and data
    processPageList( true );

    // 4. set initial page (restoring the page saved in xml if loaded)
    DocumentViewport loadedViewport = (*d->viewportIterator);
    if ( loadedViewport.pageNumber != -1 )
        (*d->viewportIterator) = DocumentViewport();
    else
        loadedViewport.pageNumber = 0;
    setViewport( loadedViewport );

    // start bookmark saver timer
    if ( !d->saveBookmarksTimer )
    {
        d->saveBookmarksTimer = new QTimer( this );
        connect( d->saveBookmarksTimer, SIGNAL( timeout() ), this, SLOT( saveDocumentInfo() ) );
    }
    d->saveBookmarksTimer->start( 5 * 60 * 1000 );

    // start memory check timer
    if ( !d->memCheckTimer )
    {
        d->memCheckTimer = new QTimer( this );
        connect( d->memCheckTimer, SIGNAL( timeout() ), this, SLOT( slotTimedMemoryCheck() ) );
    }
    d->memCheckTimer->start( 2000 );

    return true;
}

void KPDFDocument::closeDocument()
{
    // save document info if a document is still opened
    if ( generator && pages_vector.size() > 0 )
        saveDocumentInfo();

    // stop timers
    if ( d->memCheckTimer )
        d->memCheckTimer->stop();
    if ( d->saveBookmarksTimer )
        d->saveBookmarksTimer->stop();

    // delete contents generator
    delete generator;
    generator = 0;

    // remove requests left in queue
    QValueList< PixmapRequest * >::iterator sIt = d->pixmapRequestsStack.begin();
    QValueList< PixmapRequest * >::iterator sEnd = d->pixmapRequestsStack.end();
    for ( ; sIt != sEnd; ++sIt )
        delete *sIt;
    d->pixmapRequestsStack.clear();

    // send an empty list to observers (to free their data)
    foreachObserver( notifySetup( QValueVector< KPDFPage * >(), true ) );

    // delete pages and clear 'pages_vector' container
    QValueVector< KPDFPage * >::iterator pIt = pages_vector.begin();
    QValueVector< KPDFPage * >::iterator pEnd = pages_vector.end();
    for ( ; pIt != pEnd; ++pIt )
        delete *pIt;
    pages_vector.clear();

    // clear 'memory allocation' descriptors
    QValueList< AllocatedPixmap * >::iterator aIt = d->allocatedPixmapsFifo.begin();
    QValueList< AllocatedPixmap * >::iterator aEnd = d->allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    d->allocatedPixmapsFifo.clear();

    // reset internal variables
    d->viewportHistory.clear();
    d->viewportHistory.append( DocumentViewport() );
    d->viewportIterator = d->viewportHistory.begin();
    d->searchPage = -1;
    d->allocatedPixmapsTotalMemory = 0;
}


void KPDFDocument::addObserver( DocumentObserver * pObserver )
{
    // keep the pointer to the observer in a map
    d->observers[ pObserver->observerId() ] = pObserver;

    // if the observer is added while a document is already opened, tell it
    if ( !pages_vector.isEmpty() )
        pObserver->notifySetup( pages_vector, true );
}

void KPDFDocument::removeObserver( DocumentObserver * pObserver )
{
    // remove observer from the map. it won't receive notifications anymore
    if ( d->observers.contains( pObserver->observerId() ) )
    {
        // free observer's pixmap data
        int observerId = pObserver->observerId();
        QValueVector<KPDFPage*>::iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
            (*it)->deletePixmap( observerId );

        // [MEM] free observer's allocation descriptors
        QValueList< AllocatedPixmap * >::iterator aIt = d->allocatedPixmapsFifo.begin();
        QValueList< AllocatedPixmap * >::iterator aEnd = d->allocatedPixmapsFifo.end();
        while ( aIt != aEnd )
        {
            AllocatedPixmap * p = *aIt;
            if ( p->id == observerId )
            {
                aIt = d->allocatedPixmapsFifo.remove( aIt );
                delete p;
            }
            else
                ++aIt;
        }

        // delete observer entry from the map
        d->observers.remove( observerId );
    }
}

void KPDFDocument::reparseConfig()
{
    // reparse generator config and if something changed clear KPDFPages
    if ( generator && generator->reparseConfig() )
    {
        // invalidate pixmaps
        QValueVector<KPDFPage*>::iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
            (*it)->deletePixmapsAndRects();

        // [MEM] remove allocation descriptors
        QValueList< AllocatedPixmap * >::iterator aIt = d->allocatedPixmapsFifo.begin();
        QValueList< AllocatedPixmap * >::iterator aEnd = d->allocatedPixmapsFifo.end();
        for ( ; aIt != aEnd; ++aIt )
            delete *aIt;
        d->allocatedPixmapsFifo.clear();
        d->allocatedPixmapsTotalMemory = 0;

        // send reload signals to observers
        foreachObserver( notifyContentsCleared( DocumentObserver::Pixmap ) );
    }

    // free memory if in 'low' profile
    if ( Settings::memoryLevel() == Settings::EnumMemoryLevel::Low &&
         !d->allocatedPixmapsFifo.isEmpty() && !pages_vector.isEmpty() )
        cleanupPixmapMemory();
}


bool KPDFDocument::isOpened() const
{
    return generator;
}

const DocumentInfo * KPDFDocument::documentInfo() const
{
    return generator ? generator->generateDocumentInfo() : NULL;
}

const DocumentSynopsis * KPDFDocument::documentSynopsis() const
{
    return generator ? generator->generateDocumentSynopsis() : NULL;
}

const KPDFPage * KPDFDocument::page( uint n ) const
{
    return ( n < pages_vector.count() ) ? pages_vector[n] : 0;
}

const DocumentViewport & KPDFDocument::viewport() const
{
    return (*d->viewportIterator);
}

uint KPDFDocument::currentPage() const
{
    return (*d->viewportIterator).pageNumber;
}

uint KPDFDocument::pages() const
{
    return pages_vector.size();
}

bool KPDFDocument::okToPrint() const
{
    return generator ? generator->isAllowed( Generator::Print ) : false;
}

bool KPDFDocument::historyAtBegin() const
{
    return d->viewportIterator == d->viewportHistory.begin();
}

bool KPDFDocument::historyAtEnd() const
{
    return d->viewportIterator == --(d->viewportHistory.end());
}

QString KPDFDocument::getMetaData( const QString & key, const QString & option ) const
{
    return generator ? generator->getMetaData( key, option ) : QString();
}

void KPDFDocument::requestPixmaps( const QValueList< PixmapRequest * > & requests )
{
    if ( !generator )
    {
        // delete requests..
        QValueList< PixmapRequest * >::const_iterator rIt = requests.begin(), rEnd = requests.end();
        for ( ; rIt != rEnd; ++rIt )
            delete *rIt;
        // ..and return
        return;
    }

    // 1. [CLEAN STACK] remove previous requests of requesterID
    int requesterID = requests.first()->id;
    QValueList< PixmapRequest * >::iterator sIt = d->pixmapRequestsStack.begin(), sEnd = d->pixmapRequestsStack.end();
    while ( sIt != sEnd )
    {
        if ( (*sIt)->id == requesterID )
        {
            // delete request and remove it from stack
            delete *sIt;
            sIt = d->pixmapRequestsStack.remove( sIt );
        }
        else
            ++sIt;
    }

    // 2. [ADD TO STACK] add requests to stack
    bool threadingDisabled = !Settings::enableThreading();
    QValueList< PixmapRequest * >::const_iterator rIt = requests.begin(), rEnd = requests.end();
    for ( ; rIt != rEnd; ++rIt )
    {
        // set the 'page field' (see PixmapRequest) and check if it is valid
        PixmapRequest * request = *rIt;
        if ( !(request->page = pages_vector[ request->pageNumber ]) )
        {
            // skip requests referencing an invalid page (must not happen)
            delete request;
            continue;
        }

        if ( !request->async )
            request->priority = 0;

        if ( request->async && threadingDisabled )
            request->async = false;

        // add request to the 'stack' at the right place
        if ( !request->priority )
            // add priority zero requests to the top of the stack
            d->pixmapRequestsStack.push_back( request );
        else
        {
            // insert in stack sorted by priority
            sIt = d->pixmapRequestsStack.begin();
            sEnd = d->pixmapRequestsStack.end();
            while ( sIt != sEnd && (*sIt)->priority >= request->priority )
                ++sIt;
            d->pixmapRequestsStack.insert( sIt, request );
        }
    }

    // 3. [START FIRST GENERATION] if generator is ready, start a new generation,
    // or else (if gen is running) it will be started when the new contents will
    //come from generator (in requestDone())
    if ( generator->canGeneratePixmap() )
        sendGeneratorRequest();
}

void KPDFDocument::requestTextPage( uint page )
{
    KPDFPage * kp = pages_vector[ page ];
    if ( !generator || !kp )
        return;

    // Memory management for TextPages

    generator->generateSyncTextPage( kp );
}
/* REFERENCE IMPLEMENTATION: better calling setViewport from other code
void KPDFDocument::setNextPage()
{
    // advance page and set viewport on observers
    if ( (*d->viewportIterator).pageNumber < (int)pages_vector.count() - 1 )
        setViewport( DocumentViewport( (*d->viewportIterator).pageNumber + 1 ) );
}

void KPDFDocument::setPrevPage()
{
    // go to previous page and set viewport on observers
    if ( (*d->viewportIterator).pageNumber > 0 )
        setViewport( DocumentViewport( (*d->viewportIterator).pageNumber - 1 ) );
}
*/
void KPDFDocument::setViewportPage( int page, int excludeId )
{
    // clamp page in range [0 ... numPages-1]
    if ( page < 0 )
        page = 0;
    else if ( page > (int)pages_vector.count() )
        page = pages_vector.count() - 1;

    // make a viewport from the page and broadcast it
    setViewport( DocumentViewport( page ), excludeId );
}

void KPDFDocument::setViewport( const DocumentViewport & viewport, int excludeId )
{
    // if already broadcasted, don't redo it
    DocumentViewport & oldViewport = *d->viewportIterator;
    if ( viewport == oldViewport )
        kdDebug() << "setViewport with the same viewport." << endl;

    // set internal viewport taking care of history
    if ( oldViewport.pageNumber == viewport.pageNumber || oldViewport.pageNumber == -1 )
    {
        // if page is unchanged save the viewport at current position in queue
        oldViewport = viewport;
    }
    else
    {
        // remove elements after viewportIterator in queue
        d->viewportHistory.erase( ++d->viewportIterator, d->viewportHistory.end() );

        // keep the list to a reasonable size by removing head when needed
        if ( d->viewportHistory.count() >= 100 )
            d->viewportHistory.pop_front();

        // add the item at the end of the queue
        d->viewportIterator = d->viewportHistory.append( viewport );
    }

    // notify change to all other (different from id) observers
    QMap< int, DocumentObserver * >::iterator it = d->observers.begin(), end = d->observers.end();
    for ( ; it != end ; ++ it )
        if ( it.key() != excludeId )
            (*it)->notifyViewportChanged();

    // [MEM] raise position of currently viewed page in allocation queue
    if ( d->allocatedPixmapsFifo.count() > 1 )
    {
        const int page = viewport.pageNumber;
        QValueList< AllocatedPixmap * > viewportPixmaps;
        QValueList< AllocatedPixmap * >::iterator aIt = d->allocatedPixmapsFifo.begin();
        QValueList< AllocatedPixmap * >::iterator aEnd = d->allocatedPixmapsFifo.end();
        while ( aIt != aEnd )
        {
            if ( (*aIt)->page == page )
            {
                viewportPixmaps.append( *aIt );
                aIt = d->allocatedPixmapsFifo.remove( aIt );
                continue;
            }
            ++aIt;
        }
        if ( !viewportPixmaps.isEmpty() )
            d->allocatedPixmapsFifo += viewportPixmaps;
    }
}

void KPDFDocument::setPrevViewport()
// restore viewport from the history
{
    if ( d->viewportIterator != d->viewportHistory.begin() )
    {
        // restore previous viewport and notify it to observers
        --d->viewportIterator;
        foreachObserver( notifyViewportChanged() );
    }
}

void KPDFDocument::setNextViewport()
// restore next viewport from the history
{
    QValueList< DocumentViewport >::iterator nextIterator = d->viewportIterator;
    ++nextIterator;
    if ( nextIterator != d->viewportHistory.end() )
    {
        // restore next viewport and notify it to observers
        ++d->viewportIterator;
        foreachObserver( notifyViewportChanged() );
    }
}

bool KPDFDocument::findText( const QString & string, bool keepCase, bool findAhead )
{
    // turn selection drawing off on filtered pages
    if ( !d->filterText.isEmpty() )
        unHilightPages();

    // save params for the 'find next' case
    if ( !string.isEmpty() )
    {
        d->searchText = string;
        d->searchCase = keepCase;
    }

    // continue checking last SearchPage first (if it is the current page)
    int currentPage = (*d->viewportIterator).pageNumber;
    int pageCount = pages_vector.count();
    KPDFPage * foundPage = 0,
             * lastPage = (d->searchPage > -1) ? pages_vector[ d->searchPage ] : 0;
    if ( lastPage && d->searchPage == currentPage )
        if ( lastPage->hasText( d->searchText, d->searchCase, findAhead ) )
            foundPage = lastPage;
        else
        {
            lastPage->clearAttribute( KPDFPage::Highlight );
            currentPage++;
            pageCount--;
        }

    if ( !foundPage )
        // loop through the whole document
        for ( int i = 0; i < pageCount; i++ )
        {
            if ( currentPage >= pageCount )
            {
                if ( !findAhead && KMessageBox::questionYesNo(0, i18n("End of document reached.\nContinue from the beginning?")) == KMessageBox::Yes )
                    currentPage = 0;
                else
                    break;
            }
            KPDFPage * page = pages_vector[ currentPage ];
            if ( !page->hasSearchPage() )
                requestTextPage( page->number() );
            if ( page->hasText( d->searchText, d->searchCase, true ) )
            {
                foundPage = page;
                break;
            }
            currentPage++;
        }

    if ( foundPage )
    {
        int pageNumber = foundPage->number();
        d->searchPage = pageNumber;
        foundPage->setAttribute( KPDFPage::Highlight );
        // move the viewport to show the searched word centered
        DocumentViewport searchViewport( pageNumber );
        const QPoint & center = foundPage->getLastSearchCenter();
        searchViewport.reCenter.enabled = true;
        searchViewport.reCenter.normalizedCenterX = (double)center.x() / foundPage->width();
        searchViewport.reCenter.normalizedCenterY = (double)center.y() / foundPage->height();
        setViewport( searchViewport );
        // notify all observers about hilighting chages
        foreachObserver( notifyPageChanged( pageNumber, DocumentObserver::Highlights ) );
    }
    else if ( !findAhead )
        KMessageBox::information( 0, i18n("No matches found for '%1'.").arg(d->searchText) );
    return foundPage;
}

void KPDFDocument::findTextAll( const QString & pattern, bool keepCase )
{
    // if pattern is null, clear 'hilighted' attribute in all pages
    if ( pattern.isEmpty() )
        unHilightPages();

    // cache search pattern
    d->filterText = pattern;
    d->filterCase = keepCase;
    // set the busy cursor globally on the application
    QApplication::setOverrideCursor( waitCursor );
    // perform a linear search/mark
    processPageList( false );
    // reset cursor to previous shape
    QApplication::restoreOverrideCursor();
}

void KPDFDocument::toggleBookmark( int n )
{
    KPDFPage * page = ( n < (int)pages_vector.count() ) ? pages_vector[ n ] : 0;
    if ( page )
    {
        page->toggleAttribute( KPDFPage::Bookmark );
        foreachObserver( notifyPageChanged( n, DocumentObserver::Bookmark ) );
    }
}

void KPDFDocument::processLink( const KPDFLink * link )
{
    if ( !link )
        return;

    switch( link->linkType() )
    {
        case KPDFLink::Goto: {
            const KPDFLinkGoto * go = static_cast< const KPDFLinkGoto * >( link );
            DocumentViewport destVp = go->destViewport();

            // first open filename if link is pointing outside this document
            if ( go->isExternal() && !openRelativeFile( go->fileName() ) )
            {
                kdWarning() << "Link: Error opening '" << go->fileName() << "'." << endl;
                return;
            }

            // note: if external file is opened, 'link' doesn't exist anymore!
            setViewport( destVp );
            } break;

        case KPDFLink::Execute: {
            const KPDFLinkExecute * exe  = static_cast< const KPDFLinkExecute * >( link );
            QString fileName = exe->fileName();
            if ( fileName.endsWith( ".pdf" ) || fileName.endsWith( ".PDF" ) )
            {
                openRelativeFile( fileName );
                return;
            }

            // Albert: the only pdf i have that has that kind of link don't define
            // an application and use the fileName as the file to open
            fileName = giveAbsolutePath( fileName );
            KMimeType::Ptr mime = KMimeType::findByPath( fileName );
            // Check executables
            if ( KRun::isExecutableFile( fileName, mime->name() ) )
            {
                // Don't have any pdf that uses this code path, just a guess on how it should work
                if ( !exe->parameters().isEmpty() )
                {
                    fileName = giveAbsolutePath( exe->parameters() );
                    mime = KMimeType::findByPath( fileName );
                    if ( KRun::isExecutableFile( fileName, mime->name() ) )
                    {
                        // this case is a link pointing to an executable with a parameter
                        // that also is an executable, possibly a hand-crafted pdf
                        KMessageBox::information( 0, i18n("The pdf file is trying to execute an external application and for your safety kpdf does not allow that.") );
                        return;
                    }
                }
                else
                {
                    // this case is a link pointing to an executable with no parameters
                    // core developers find unacceptable executing it even after asking the user
                    KMessageBox::information( 0, i18n("The pdf file is trying to execute an external application and for your safety kpdf does not allow that.") );
                    return;
                }
            }

            KService::Ptr ptr = KServiceTypeProfile::preferredService( mime->name(), "Application" );
            if ( ptr )
            {
                KURL::List lst;
                lst.append( fileName );
                KRun::run( *ptr, lst );
            }
            else
                KMessageBox::information( 0, i18n( "No application found for opening file of mimetype %1." ).arg( mime->name() ) );
            } break;

        case KPDFLink::Action: {
            const KPDFLinkAction * action = static_cast< const KPDFLinkAction * >( link );
            switch( action->actionType() )
            {
                case KPDFLinkAction::PageFirst:
                    setViewportPage( 0 );
                    break;
                case KPDFLinkAction::PagePrev:
                    if ( (*d->viewportIterator).pageNumber > 0 )
                        setViewportPage( (*d->viewportIterator).pageNumber - 1 );
                    break;
                case KPDFLinkAction::PageNext:
                    if ( (*d->viewportIterator).pageNumber < (int)pages_vector.count() - 1 )
                        setViewportPage( (*d->viewportIterator).pageNumber + 1 );
                    break;
                case KPDFLinkAction::PageLast:
                    setViewportPage( pages_vector.count() - 1 );
                    break;
                case KPDFLinkAction::HistoryBack:
                    setPrevViewport();
                    break;
                case KPDFLinkAction::HistoryForward:
                    setNextViewport();
                    break;
                case KPDFLinkAction::Quit:
                    kapp->quit();
                    break;
                case KPDFLinkAction::Find:
                    emit linkFind();
                    break;
                case KPDFLinkAction::GoToPage:
                    emit linkGoToPage();
                    break;
            }
            } break;

        case KPDFLink::Browse: {
            const KPDFLinkBrowse * browse = static_cast< const KPDFLinkBrowse * >( link );
            // if the url is a mailto one, invoke mailer
            if ( browse->url().startsWith( "mailto:", false ) )
                kapp->invokeMailer( browse->url() );
            else
            {
                // get service for web browsing
                KService::Ptr ptr = KServiceTypeProfile::preferredService("text/html", "Application");
                KURL::List lst;
                // append 'url' parameter to the service and run it
                lst.append( browse->url() );
                KRun::run( *ptr, lst );
            }
            } break;

        case KPDFLink::Movie:
            //const KPDFLinkMovie * browse = static_cast< const KPDFLinkMovie * >( link );
            // TODO this (Movie link)
            break;
    }
}

bool KPDFDocument::print( KPrinter &printer )
{
    return generator ? generator->print( printer ) : false;
}

void KPDFDocument::requestDone( PixmapRequest * req )
{
#ifndef NDEBUG
    if ( !generator->canGeneratePixmap() )
        kdDebug() << "requestDone with generator not in READY state." << endl;
#endif

    // [MEM] 1.1 find and remove a previous entry for the same page and id
    QValueList< AllocatedPixmap * >::iterator aIt = d->allocatedPixmapsFifo.begin();
    QValueList< AllocatedPixmap * >::iterator aEnd = d->allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        if ( (*aIt)->page == req->pageNumber && (*aIt)->id == req->id )
        {
            AllocatedPixmap * p = *aIt;
            d->allocatedPixmapsFifo.remove( aIt );
            d->allocatedPixmapsTotalMemory -= p->memory;
            delete p;
            break;
        }

    // [MEM] 1.2 append memory allocation descriptor to the FIFO
    int memoryBytes = 4 * req->width * req->height;
    AllocatedPixmap * memoryPage = new AllocatedPixmap( req->id, req->pageNumber, memoryBytes );
    d->allocatedPixmapsFifo.append( memoryPage );
    d->allocatedPixmapsTotalMemory += memoryBytes;

    // 2. notify an observer that its pixmap changed
    if ( d->observers.contains( req->id ) )
        d->observers[ req->id ]->notifyPageChanged( req->pageNumber, DocumentObserver::Pixmap );

    // 3. delete request
    delete req;

    // 4. start a new generation if some is pending
    if ( !d->pixmapRequestsStack.isEmpty() )
        sendGeneratorRequest();
}

void KPDFDocument::sendGeneratorRequest()
{
    // find a request
    PixmapRequest * request = 0;
    while ( !d->pixmapRequestsStack.isEmpty() && !request )
    {
        PixmapRequest * r = d->pixmapRequestsStack.last();
        d->pixmapRequestsStack.pop_back();
        // request only if page isn't already present
        if ( !r->page->hasPixmap( r->id, r->width, r->height ) )
            request = r;
        else
            delete r;
    }

    // if no request found (or already generated), return
    if ( !request )
        return;

    // [MEM] preventive memory freeing
    int pixmapBytes = 4 * request->width * request->height;
    if ( pixmapBytes > (1024 * 1024) )
        cleanupPixmapMemory( pixmapBytes );

    // submit the request to the generator
    generator->generatePixmap( request );
}

void KPDFDocument::cleanupPixmapMemory( int /*sure? bytesOffset*/ )
{
    // [MEM] choose memory parameters based on configuration profile
    int clipValue = -1;
    int memoryToFree = -1;
    switch ( Settings::memoryLevel() )
    {
        case Settings::EnumMemoryLevel::Low:
            memoryToFree = d->allocatedPixmapsTotalMemory;
            break;

        case Settings::EnumMemoryLevel::Normal:
            memoryToFree = d->allocatedPixmapsTotalMemory - getTotalMemory() / 3;
            clipValue = (d->allocatedPixmapsTotalMemory - getFreeMemory()) / 2;
            break;

        case Settings::EnumMemoryLevel::Aggressive:
            clipValue = (d->allocatedPixmapsTotalMemory - getFreeMemory()) / 2;
            break;
    }

    if ( clipValue > memoryToFree )
        memoryToFree = clipValue;

    if ( memoryToFree > 0 )
    {
        // [MEM] free memory starting from older pixmaps
        int pagesFreed = 0;
        QValueList< AllocatedPixmap * >::iterator pIt = d->allocatedPixmapsFifo.begin();
        QValueList< AllocatedPixmap * >::iterator pEnd = d->allocatedPixmapsFifo.end();
        while ( (pIt != pEnd) && (memoryToFree > 0) )
        {
            AllocatedPixmap * p = *pIt;
            if ( d->observers[ p->id ]->canUnloadPixmap( p->page ) )
            {
                // update internal variables
                pIt = d->allocatedPixmapsFifo.remove( pIt );
                d->allocatedPixmapsTotalMemory -= p->memory;
                memoryToFree -= p->memory;
                pagesFreed++;
                // delete pixmap
                pages_vector[ p->page ]->deletePixmap( p->id );
                // delete allocation descriptor
                delete p;
            } else
                ++pIt;
        }
        //p--rintf("freeMemory A:[%d -%d = %d] \n", d->allocatedPixmapsFifo.count() + pagesFreed, pagesFreed, d->allocatedPixmapsFifo.count() );
    }
}

int KPDFDocument::getTotalMemory()
{
    static int cachedValue = 0;
    if ( cachedValue )
        return cachedValue;

#ifdef __linux__
    // if /proc/meminfo doesn't exist, return 128MB
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( IO_ReadOnly ) )
        return (cachedValue = 134217728);

    // read /proc/meminfo and sum up the contents of 'MemFree', 'Buffers'
    // and 'Cached' fields. consider swapped memory as used memory.
    QTextStream readStream( &memFile );
    while ( !readStream.atEnd() )
    {
        QString entry = readStream.readLine();
        if ( entry.startsWith( "MemTotal:" ) )
            return (cachedValue = (1024 * entry.section( ' ', -2, -2 ).toInt()));
    }
#endif
    return (cachedValue = 134217728);
}

int KPDFDocument::getFreeMemory()
{
#ifdef __linux__
    // if /proc/meminfo doesn't exist, return MEMORY FULL
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( IO_ReadOnly ) )
        return 0;

    // read /proc/meminfo and sum up the contents of 'MemFree', 'Buffers'
    // and 'Cached' fields. consider swapped memory as used memory.
    int memoryFree = 0;
    QString entry;
    QTextStream readStream( &memFile );
    while ( !readStream.atEnd() )
    {
        entry = readStream.readLine();
        if ( entry.startsWith( "MemFree:" ) ||
                entry.startsWith( "Buffers:" ) ||
                entry.startsWith( "Cached:" ) ||
                entry.startsWith( "SwapFree:" ) )
            memoryFree += entry.section( ' ', -2, -2 ).toInt();
        if ( entry.startsWith( "SwapTotal:" ) )
            memoryFree -= entry.section( ' ', -2, -2 ).toInt();
    }
    memFile.close();
    return 1024 * memoryFree;
#else
    // tell the memory is full.. will act as in LOW profile
    return 0;
#endif
}

void KPDFDocument::loadDocumentInfo()
// note: load data and stores it internally (document or pages). observers
// are still uninitialized at this point so don't access them
{
    //kdDebug() << "Using '" << d->xmlFileName << "' as document info file." << endl;
    QFile infoFile( d->xmlFileName );
    if ( !infoFile.exists() || !infoFile.open( IO_ReadOnly ) )
        return;

    // Load DOM from XML file
    QDomDocument doc( "documentInfo" );
    if ( !doc.setContent( &infoFile ) )
    {
        kdDebug() << "Could not set content" << endl;
        infoFile.close();
        return;
    }
    infoFile.close();

    QDomElement root = doc.documentElement();
    if ( root.tagName() != "documentInfo" )
        return;

    // Parse the DOM tree
    QDomNode topLevelNode = root.firstChild();
    while ( topLevelNode.isElement() )
    {
        QString catName = topLevelNode.toElement().tagName();

        // Get bookmarks list from DOM
        if ( catName == "bookmarkList" )
        {
            QDomNode n = topLevelNode.firstChild();
            QDomElement e;
            int pageNumber;
            bool ok;
            while ( n.isElement() )
            {
                e = n.toElement();
                if (e.tagName() == "page")
                {
                    pageNumber = e.text().toInt(&ok);
                    if ( ok && pageNumber >= 0 && pageNumber < (int)pages_vector.count() )
                        pages_vector[ pageNumber ]->setAttribute( KPDFPage::Bookmark );
                }
                n = n.nextSibling();
            }
        } // </bookmarkList>
        // Get 'general info' from the DOM
        else if ( catName == "generalInfo" )
        {
            QDomNode infoNode = topLevelNode.firstChild();
            while ( infoNode.isElement() )
            {
                QDomElement infoElement = infoNode.toElement();

                // compatibility: [pre-3.4 viewport storage] @remove after 3.4 relase
                if ( infoElement.tagName() == "activePage" )
                {
                    if ( infoElement.hasAttribute( "viewport" ) )
                        *d->viewportIterator = DocumentViewport( infoElement.attribute( "viewport" ) );
                }

                // restore viewports history
                if ( infoElement.tagName() == "history" )
                {
                    // clear history
                    d->viewportHistory.clear();
                    // append old viewports
                    QDomNode historyNode = infoNode.firstChild();
                    while ( historyNode.isElement() )
                    {
                        QDomElement historyElement = historyNode.toElement();
                        if ( historyElement.hasAttribute( "viewport" ) )
                        {
                            QString vpString = historyElement.attribute( "viewport" );
                            d->viewportIterator = d->viewportHistory.append(
                                    DocumentViewport( vpString ) );
                        }
                        historyNode = historyNode.nextSibling();
                    }
                    // consistancy check
                    if ( d->viewportHistory.isEmpty() )
                        d->viewportIterator = d->viewportHistory.append( DocumentViewport() );
                }
                infoNode = infoNode.nextSibling();
            }
        } // </generalInfo>
        topLevelNode = topLevelNode.nextSibling();
    } // </documentInfo>
}

QString KPDFDocument::giveAbsolutePath( const QString & fileName )
{
    if ( d->docFileName.isEmpty() )
        return QString::null;

    // convert the pdf fileName to absolute using current pdf path
    QFileInfo currentInfo( d->docFileName );
    return currentInfo.dir().absFilePath( fileName );
}

bool KPDFDocument::openRelativeFile( const QString & fileName )
{
    QString absFileName = giveAbsolutePath( fileName );
    if ( absFileName.isNull() )
        return false;

    kdDebug() << "openDocument: '" << absFileName << "'" << endl;

    // open the absolute filename
    return openDocument( absFileName );
}

void KPDFDocument::processPageList( bool documentChanged )
{
    if ( d->filterText.length() < 3 )
        unHilightPages();
    else
    {
        uint pageCount = pages_vector.count();
        for ( uint i = 0; i < pageCount ; i++ )
        {
            KPDFPage * page = pages_vector[ i ];
            page->clearAttribute( KPDFPage::Highlight );
            if ( d->filterText.length() > 2 )
            {
                if ( !page->hasSearchPage() )
                    requestTextPage( i );
                if ( page->hasText( d->filterText, d->filterCase, true ) )
                    page->setAttribute( KPDFPage::Highlight );
            }
        }
    }

    // send the list to observers
    foreachObserver( notifySetup( pages_vector, documentChanged ) );
}

void KPDFDocument::unHilightPages(bool filteredOnly)
{
    if ( filteredOnly && d->filterText.isEmpty() )
        return;

    d->filterText = QString::null;
    QValueVector<KPDFPage*>::iterator it = pages_vector.begin(), end = pages_vector.end();
    for ( ; it != end; ++it )
    {
        KPDFPage * page = *it;
        if ( page->attributes() & KPDFPage::Highlight )
        {
            page->clearAttribute( KPDFPage::Highlight );
            foreachObserver( notifyPageChanged( page->number(), DocumentObserver::Highlights ) );
        }
    }
}


void KPDFDocument::saveDocumentInfo() const
{
    if ( d->docFileName.isNull() )
        return;

    QFile infoFile( d->xmlFileName );
    if (infoFile.open( IO_WriteOnly | IO_Truncate) )
    {
        // Create DOM
        QDomDocument doc( "documentInfo" );
        QDomElement root = doc.createElement( "documentInfo" );
        doc.appendChild( root );

        // Add bookmark list to DOM
        QDomElement bookmarkList = doc.createElement( "bookmarkList" );
        root.appendChild( bookmarkList );

        for ( uint i = 0; i < pages_vector.count() ; i++ )
        {
            if (pages_vector[i]->attributes() & KPDFPage::Bookmark)
            {
                QDomElement page = doc.createElement( "page" );
                page.appendChild( doc.createTextNode( QString::number(i) ) );

                bookmarkList.appendChild( page );
            }
        }

        // Add general info to DOM
        QDomElement generalInfo = doc.createElement( "generalInfo" );
        root.appendChild( generalInfo );

        // <general info><history> ... </history> saves history up to 10 viewports
        QValueList< DocumentViewport >::iterator backIterator = d->viewportIterator;
        if ( backIterator != d->viewportHistory.end() )
        {
            // go back up to 10 steps from the current viewportIterator
            int backSteps = 10;
            while ( backSteps-- && backIterator != d->viewportHistory.begin() )
                --backIterator;

            // create history root node
            QDomElement historyNode = doc.createElement( "history" );
            generalInfo.appendChild( historyNode );

            // add old[backIterator] and present[viewportIterator] items
            QValueList< DocumentViewport >::iterator endIt = d->viewportIterator;
            ++endIt;
            while ( backIterator != endIt )
            {
                QString name = (backIterator == d->viewportIterator) ? "current" : "oldPage";
                QDomElement historyEntry = doc.createElement( name );
                historyEntry.setAttribute( "viewport", (*backIterator).toString() );
                historyNode.appendChild( historyEntry );
                ++backIterator;
            }
        }

        // Save DOM to XML file
        QString xml = doc.toString();
        QTextStream os( &infoFile );
        os << xml;
    }
    infoFile.close();
}

void KPDFDocument::slotTimedMemoryCheck()
{
    // [MEM] clean memory (for 'free mem dependant' profiles only)
    if ( Settings::memoryLevel() != Settings::EnumMemoryLevel::Low &&
         d->allocatedPixmapsTotalMemory > 1024*1024 )
        cleanupPixmapMemory();
}


/** DocumentViewport **/

DocumentViewport::DocumentViewport( int n )
    : pageNumber( n )
{
    // default settings
    reCenter.enabled = false;
    reCenter.normalizedCenterX = 0.5;
    reCenter.normalizedCenterY = 0.0;
    autoFit.enabled = false;
    autoFit.width = false;
    autoFit.height = false;
}

DocumentViewport::DocumentViewport( const QString & xmlDesc )
    : pageNumber( -1 )
{
    // default settings (maybe overridden below)
    reCenter.enabled = false;
    reCenter.normalizedCenterX = 0.5;
    reCenter.normalizedCenterY = 0.0;
    autoFit.enabled = false;
    autoFit.width = false;
    autoFit.height = false;

    // check for string presence
    if ( xmlDesc.isEmpty() )
        return;

    // decode the string
    bool ok;
    int field = 0;
    QString token = xmlDesc.section( ';', field, field );
    while ( !token.isEmpty() )
    {
        // decode the current token
        if ( field == 0 )
        {
            pageNumber = token.toInt( &ok );
            if ( !ok )
                return;
        }
        else if ( token.startsWith( "C1" ) )
        {
            reCenter.enabled = true;
            reCenter.normalizedCenterX = token.section( ':', 1, 1 ).toDouble();
            reCenter.normalizedCenterY = token.section( ':', 2, 2 ).toDouble();
        }
        else if ( token.startsWith( "AF1" ) )
        {
            autoFit.enabled = true;
            autoFit.width = token.section( ':', 1, 1 ) == "T";
            autoFit.height = token.section( ':', 2, 2 ) == "T";
        }
        // proceed tokenizing string
        field++;
        token = xmlDesc.section( ';', field, field );
    }
}

QString DocumentViewport::toString() const
{
    // start string with page number
    QString s = QString::number( pageNumber );
    // if has center coordinates, save them on string
    if ( reCenter.enabled )
        s += QString( ";C1:" ) + QString::number( reCenter.normalizedCenterX ) +
             ':' + QString::number( reCenter.normalizedCenterY );
    // if has autofit enabled, save its state on string
    if ( autoFit.enabled )
        s += QString( ";AF1:" ) + (autoFit.width ? "T" : "F") +
             ':' + (autoFit.height ? "T" : "F");
    return s;
}

bool DocumentViewport::operator==( const DocumentViewport & vp ) const
{
    bool equal = ( pageNumber == vp.pageNumber ) &&
                 ( reCenter.enabled == vp.reCenter.enabled ) &&
                 ( autoFit.enabled == vp.autoFit.enabled );
    if ( !equal )
        return false;
    if ( reCenter.enabled &&
         (( reCenter.normalizedCenterX != vp.reCenter.normalizedCenterX ) ||
         ( reCenter.normalizedCenterY != vp.reCenter.normalizedCenterY )) )
        return false;
    if ( autoFit.enabled &&
         (( autoFit.width != vp.autoFit.width ) ||
         ( autoFit.height != vp.autoFit.height )) )
        return false;
    return true;
}


/** DocumentInfo **/

DocumentInfo::DocumentInfo()
  : QDomDocument( "DocumentInformation" )
{
    QDomElement docElement = createElement( "DocumentInfo" );
    appendChild( docElement );
}

void DocumentInfo::set( const QString &key, const QString &value,
                        const QString &title )
{
    QDomElement docElement = documentElement();
    QDomElement element;

    // check whether key already exists
    QDomNodeList list = docElement.elementsByTagName( key );
    if ( list.count() > 0 )
        element = list.item( 0 ).toElement();
    else
        element = createElement( key );

    element.setAttribute( "value", value );
    element.setAttribute( "title", title );

    if ( list.count() == 0 )
        docElement.appendChild( element );
}

QString DocumentInfo::get( const QString &key ) const
{
    QDomElement docElement = documentElement();
    QDomElement element;

    // check whether key already exists
    QDomNodeList list = docElement.elementsByTagName( key );
    if ( list.count() > 0 )
        return list.item( 0 ).toElement().attribute( "value" );
    else
        return QString();
}


/** DocumentSynopsis **/

DocumentSynopsis::DocumentSynopsis()
  : QDomDocument( "DocumentSynopsis" )
{
    // void implementation, only subclassed for naming
}

#include "document.moc"
