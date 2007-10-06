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
#include <qimage.h>
#include <qtextstream.h>
#include <qvaluevector.h>
#include <qtimer.h>
#include <qmap.h>
#include <kdebug.h>
#include <kimageio.h>
#include <klocale.h>
#include <kfinddialog.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kuserprofile.h>
#include <krun.h>
#include <kstandarddirs.h>

// local includes
#include "document.h"
#include "observer.h"
#include "page.h"
#include "link.h"
#include "generator_pdf/generator_pdf.h"  // PDF generator
#include "generator_kimgio/generator_kimgio.h"  // KIMGIO generator
#include "conf/settings.h"

// structures used internally by KPDFDocument for local variables storage
class AllocatedPixmap;
class RunningSearch;
class KPDFDocumentPrivate
{
    public:
        // find descriptors, mapped by ID (we handle multiple searches)
        QMap< int, RunningSearch * > searches;
        int m_lastSearchID;

        // needed because for remote documents docFileName is a local file and
        // we want the remote url when the document refers to relativeNames
        KURL url;

        // cached stuff
        QString docFileName;
        QString xmlFileName;

        // a list of the mimetypes qimage can understand
        QStringList kimgioMimes;

        // viewport stuff
        QValueList< DocumentViewport > viewportHistory;
        QValueList< DocumentViewport >::iterator viewportIterator;
        DocumentViewport nextDocumentViewport; // see KPDFLink::Goto for an explanation

        // observers / requests / allocator stuff
        QMap< int, DocumentObserver * > observers;
        QValueList< PixmapRequest * > pixmapRequestsStack;
        QValueList< AllocatedPixmap * > allocatedPixmapsFifo;
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

struct RunningSearch
{
    // store search properties
    int continueOnPage;
    NormalizedRect continueOnMatch;
    QValueList< int > highlightedPages;

    // fields related to previous searches (used for 'continueSearch')
    QString cachedString;
    KPDFDocument::SearchType cachedType;
    bool cachedCaseSensitive;
    bool cachedViewportMove;
    bool cachedNoDialogs;
    QColor cachedColor;
};

#define foreachObserver( cmd ) {\
    QMap< int, DocumentObserver * >::iterator it=d->observers.begin(), end=d->observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }


/** KPDFDocument **/

KPDFDocument::KPDFDocument(QWidget *widget)
    : QObject(widget), generator( 0 ), d( new KPDFDocumentPrivate )
{
    d->allocatedPixmapsTotalMemory = 0;
    d->memCheckTimer = 0;
    d->saveBookmarksTimer = 0;
    d->m_lastSearchID = -1;
    KImageIO::registerFormats();
    QStringList list = QImage::inputFormatList();
    QStringList::Iterator it = list.begin();
    while( it != list.end() )
    {
        d->kimgioMimes << KMimeType::findByPath(QString("foo.%1").arg(*it), 0, true)->name();
        ++it;
    }
}

KPDFDocument::~KPDFDocument()
{
    // delete generator, pages, and related stuff
    closeDocument();

    // delete the private structure
    delete d;
}


bool KPDFDocument::openDocument( const QString & docFile, const KURL & url, const KMimeType::Ptr &mime )
{
    // docFile is always local so we can use QFile on it
    QFile fileReadTest( docFile );
    if ( !fileReadTest.open( IO_ReadOnly ) )
    {
        d->docFileName = QString::null;
        return false;
    }
    // determine the related "xml document-info" filename
    d->url = url;
    d->docFileName = docFile;
    QString fn = docFile.contains('/') ? docFile.section('/', -1, -1) : docFile;
    fn = "kpdf/" + QString::number(fileReadTest.size()) + "." + fn + ".xml";
    fileReadTest.close();
    d->xmlFileName = locateLocal( "data", fn );

    // create the generator based on the file's mimetype
    if ( (*mime).is( "application/pdf" ) )
        generator = new PDFGenerator( this );
//    else if ( mimeName == "application/postscript" )
//        kdError() << "PS generator not available" << endl;
    else
    {
        QStringList::Iterator it = d->kimgioMimes.begin();
        while( it != d->kimgioMimes.end() )
        {
            kdDebug() << *it << endl;
            if ( (*mime).is( *it ) )
            {
                generator = new KIMGIOGenerator( this );
                break;
            }
            ++it;
        }
        if ( it == d->kimgioMimes.end() )
        {
            kdWarning() << "Unknown mimetype '" << mime->name() << "'." << endl;
            return false;
        }
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
    foreachObserver( notifySetup( pages_vector, true ) );

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

    if (d->nextDocumentViewport.pageNumber != -1)
    {
        setViewport(d->nextDocumentViewport);
        d->nextDocumentViewport = DocumentViewport();
    }

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

    d->url = KURL();

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

    // clear 'running searches' descriptors
    QMap< int, RunningSearch * >::iterator rIt = d->searches.begin();
    QMap< int, RunningSearch * >::iterator rEnd = d->searches.end();
    for ( ; rIt != rEnd; ++rIt )
        delete *rIt;
    d->searches.clear();

    // reset internal variables
    d->viewportHistory.clear();
    d->viewportHistory.append( DocumentViewport() );
    d->viewportIterator = d->viewportHistory.begin();
    d->allocatedPixmapsTotalMemory = 0;
}


void KPDFDocument::addObserver( DocumentObserver * pObserver )
{
    // keep the pointer to the observer in a map
    d->observers[ pObserver->observerId() ] = pObserver;

    // if the observer is added while a document is already opened, tell it
    if ( !pages_vector.isEmpty() )
    {
        pObserver->notifySetup( pages_vector, true );
        pObserver->notifyViewportChanged( false /*disables smoothMove*/ );
    }
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
    if ( KpdfSettings::memoryLevel() == KpdfSettings::EnumMemoryLevel::Low &&
         !d->allocatedPixmapsFifo.isEmpty() && !pages_vector.isEmpty() )
        cleanupPixmapMemory();
}


QWidget *KPDFDocument::widget() const
{
    return static_cast<QWidget*>(parent());
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

KURL KPDFDocument::currentDocument() const
{
    return d->url;
}

bool KPDFDocument::isAllowed( int flags ) const
{
    return generator ? generator->isAllowed( flags ) : false;
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

bool KPDFDocument::supportsSearching() const
{
    return generator ? generator->supportsSearching() : false;
}

bool KPDFDocument::hasFonts() const
{
    return generator ? generator->hasFonts() : false;
}

void KPDFDocument::putFontInfo(KListView *list)
{
    if (generator) generator->putFontInfo(list);
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
    bool threadingDisabled = !KpdfSettings::enableThreading();
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
            d->pixmapRequestsStack.append( request );
        else
        {
            // insert in stack sorted by priority
            sIt = d->pixmapRequestsStack.begin();
            sEnd = d->pixmapRequestsStack.end();
            while ( sIt != sEnd && (*sIt)->priority > request->priority )
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
void KPDFDocument::setViewportPage( int page, int excludeId, bool smoothMove )
{
    // clamp page in range [0 ... numPages-1]
    if ( page < 0 )
        page = 0;
    else if ( page > (int)pages_vector.count() )
        page = pages_vector.count() - 1;

    // make a viewport from the page and broadcast it
    setViewport( DocumentViewport( page ), excludeId, smoothMove );
}

void KPDFDocument::setViewport( const DocumentViewport & viewport, int excludeId, bool smoothMove )
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
            (*it)->notifyViewportChanged( smoothMove );

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
        foreachObserver( notifyViewportChanged( true ) );
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
        foreachObserver( notifyViewportChanged( true ) );
    }
}

void KPDFDocument::setNextDocumentViewport( const DocumentViewport & viewport )
{
    d->nextDocumentViewport = viewport;
}


bool KPDFDocument::searchText( int searchID, const QString & text, bool fromStart, bool caseSensitive,
                               SearchType type, bool moveViewport, const QColor & color, bool noDialogs )
{
    // don't perform searches on empty docs
    if ( !generator || pages_vector.isEmpty() )
        return false;

    // if searchID search not recorded, create new descriptor and init params
    if ( !d->searches.contains( searchID ) )
    {
        RunningSearch * search = new RunningSearch();
        search->continueOnPage = -1;
        d->searches[ searchID ] = search;
    }
    if (d->m_lastSearchID != searchID)
    {
        resetSearch(d->m_lastSearchID);
    }
    d->m_lastSearchID = searchID;
    RunningSearch * s = d->searches[ searchID ];

    // update search stucture
    bool newText = text != s->cachedString;
    s->cachedString = text;
    s->cachedType = type;
    s->cachedCaseSensitive = caseSensitive;
    s->cachedViewportMove = moveViewport;
    s->cachedNoDialogs = noDialogs;
    s->cachedColor = color;

    // global data for search
    bool foundAMatch = false;
    QValueList< int > pagesToNotify;

    // remove highlights from pages and queue them for notifying changes
    pagesToNotify += s->highlightedPages;
    QValueList< int >::iterator it = s->highlightedPages.begin(), end = s->highlightedPages.end();
    for ( ; it != end; ++it )
        pages_vector[ *it ]->deleteHighlights( searchID );
    s->highlightedPages.clear();

    // set hourglass cursor
    QApplication::setOverrideCursor( waitCursor );

    // 1. ALLDOC - proces all document marking pages
    if ( type == AllDoc )
    {
        // search and highlight text on all pages
        QValueVector< KPDFPage * >::iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
        {
            // get page (from the first to the last)
            KPDFPage * page = *it;
            int pageNumber = page->number();

            // request search page if needed
            if ( !page->hasSearchPage() )
                requestTextPage( pageNumber );

            // loop on a page adding highlights for all found items
            bool addedHighlights = false;
            NormalizedRect * lastMatch = 0;
            while ( 1 )
            {
                if ( lastMatch )
                    lastMatch = page->findText( text, caseSensitive, lastMatch );
                else
                    lastMatch = page->findText( text, caseSensitive );

                if ( !lastMatch )
                    break;

                // add highligh rect to the page
                page->setHighlight( searchID, lastMatch, color );
                addedHighlights = true;
            }

            // if added highlights, udpate internals and queue page for notify
            if ( addedHighlights )
            {
                foundAMatch = true;
                s->highlightedPages.append( pageNumber );
                if ( !pagesToNotify.contains( pageNumber ) )
                    pagesToNotify.append( pageNumber );
            }
        }

        // reset cursor to previous shape
        QApplication::restoreOverrideCursor();

        // send page lists if found anything new
        //if ( foundAMatch ) ?maybe?
            foreachObserver( notifySetup( pages_vector, false ) );
    }
    // 2. NEXTMATCH - find next matching item (or start from top)
    else if ( type == NextMatch )
    {
        // find out from where to start/resume search from
        int viewportPage = (*d->viewportIterator).pageNumber;
        int currentPage = fromStart ? 0 : ((s->continueOnPage != -1) ? s->continueOnPage : viewportPage);
        KPDFPage * lastPage = fromStart ? 0 : pages_vector[ currentPage ];

        // continue checking last SearchPage first (if it is the current page)
        NormalizedRect * match = 0;
        if ( lastPage && lastPage->number() == s->continueOnPage )
        {
            if ( newText )
                match = lastPage->findText( text, caseSensitive );
            else
                match = lastPage->findText( text, caseSensitive, &s->continueOnMatch );
            if ( !match )
                currentPage++;
        }

        // if no match found, loop through the whole doc, starting from currentPage
        if ( !match )
        {
            const int pageCount = pages_vector.count();
            for ( int i = 0; i < pageCount; i++ )
            {
                if ( currentPage >= pageCount )
                {
                    if ( noDialogs || KMessageBox::questionYesNo(widget(), i18n("End of document reached.\nContinue from the beginning?"), QString::null, KStdGuiItem::cont(), KStdGuiItem::cancel()) == KMessageBox::Yes )
                        currentPage = 0;
                    else
                        break;
                }
                // get page
                KPDFPage * page = pages_vector[ currentPage ];
                // request search page if needed
                if ( !page->hasSearchPage() )
                    requestTextPage( page->number() );
                // if found a match on the current page, end the loop
                if ( (match = page->findText( text, caseSensitive )) )
                    break;
                currentPage++;
            }
        }

        // reset cursor to previous shape
        QApplication::restoreOverrideCursor();

        // if a match has been found..
        if ( match )
        {
            // update the RunningSearch structure adding this match..
            foundAMatch = true;
            s->continueOnPage = currentPage;
            s->continueOnMatch = *match;
            s->highlightedPages.append( currentPage );
            // ..add highlight to the page..
            pages_vector[ currentPage ]->setHighlight( searchID, match, color );

            // ..queue page for notifying changes..
            if ( !pagesToNotify.contains( currentPage ) )
                pagesToNotify.append( currentPage );

            // ..move the viewport to show the searched word centered
            if ( moveViewport )
            {
                DocumentViewport searchViewport( currentPage );
                searchViewport.rePos.enabled = true;
                searchViewport.rePos.normalizedX = (match->left + match->right) / 2.0;
                searchViewport.rePos.normalizedY = (match->top + match->bottom) / 2.0;
                setViewport( searchViewport, -1, true );
            }
        }
        else if ( !noDialogs )
            KMessageBox::information( widget(), i18n("No matches found for '%1'.").arg( text ) );
    }
    // 3. PREVMATCH //TODO
    else if ( type == PrevMatch )
    {
    }
    // 4. GOOGLE* - process all document marking pages
    else if ( type == GoogleAll || type == GoogleAny )
    {
        // search and highlight every word in 'text' on all pages
        bool matchAll = type == GoogleAll;
        QStringList words = QStringList::split( " ", text );
        int wordsCount = words.count(),
            hueStep = (wordsCount > 1) ? (60 / (wordsCount - 1)) : 60,
            baseHue, baseSat, baseVal;
        color.getHsv( &baseHue, &baseSat, &baseVal );
        QValueVector< KPDFPage * >::iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
        {
            // get page (from the first to the last)
            KPDFPage * page = *it;
            int pageNumber = page->number();

            // request search page if needed
            if ( !page->hasSearchPage() )
                requestTextPage( pageNumber );

            // loop on a page adding highlights for all found items
            bool allMatched = wordsCount > 0,
                 anyMatched = false;
            for ( int w = 0; w < wordsCount; w++ )
            {
                QString word = words[ w ];
                int newHue = baseHue - w * hueStep;
                if ( newHue < 0 )
                    newHue += 360;
                QColor wordColor = QColor( newHue, baseSat, baseVal, QColor::Hsv );
                NormalizedRect * lastMatch = 0;
                // add all highlights for current word
                bool wordMatched = false;
                while ( 1 )
    {
                    if ( lastMatch )
                        lastMatch = page->findText( word, caseSensitive, lastMatch );
                    else
                        lastMatch = page->findText( word, caseSensitive );

                    if ( !lastMatch )
                        break;

                    // add highligh rect to the page
                    page->setHighlight( searchID, lastMatch, wordColor );
                    wordMatched = true;
                }
                allMatched = allMatched && wordMatched;
                anyMatched = anyMatched || wordMatched;
            }

            // if not all words are present in page, remove partial highlights
            if ( !allMatched && matchAll )
                page->deleteHighlights( searchID );

            // if page contains all words, udpate internals and queue page for notify
            if ( (allMatched && matchAll) || (anyMatched && !matchAll) )
            {
                foundAMatch = true;
                s->highlightedPages.append( pageNumber );
                if ( !pagesToNotify.contains( pageNumber ) )
                    pagesToNotify.append( pageNumber );
            }
        }

        // reset cursor to previous shape
        QApplication::restoreOverrideCursor();

        // send page lists to update observers (since some filter on bookmarks)
        foreachObserver( notifySetup( pages_vector, false ) );
    }

    // notify observers about highlights changes
    QValueList< int >::iterator nIt = pagesToNotify.begin(), nEnd = pagesToNotify.end();
    for ( ; nIt != nEnd; ++nIt )
        foreachObserver( notifyPageChanged( *nIt, DocumentObserver::Highlights ) );

    // return if search has found one or more matches
    return foundAMatch;
}

bool KPDFDocument::continueSearch( int searchID )
{
    // check if searchID is present in runningSearches
    if ( !d->searches.contains( searchID ) )
        return false;

    // start search with cached parameters from last search by searchID
    RunningSearch * p = d->searches[ searchID ];
    return searchText( searchID, p->cachedString, false, p->cachedCaseSensitive,
                       p->cachedType, p->cachedViewportMove, p->cachedColor,
                       p->cachedNoDialogs );
}

void KPDFDocument::resetSearch( int searchID )
{
    // check if searchID is present in runningSearches
    if ( !d->searches.contains( searchID ) )
        return;

    // get previous parameters for search
    RunningSearch * s = d->searches[ searchID ];

    // unhighlight pages and inform observers about that
    QValueList< int >::iterator it = s->highlightedPages.begin(), end = s->highlightedPages.end();
    for ( ; it != end; ++it )
    {
        int pageNumber = *it;
        pages_vector[ pageNumber ]->deleteHighlights( searchID );
        foreachObserver( notifyPageChanged( pageNumber, DocumentObserver::Highlights ) );
    }

    // send the setup signal too (to update views that filter on matches)
    foreachObserver( notifySetup( pages_vector, false ) );

    // remove serch from the runningSearches list and delete it
    d->searches.remove( searchID );
    delete s;
}

bool KPDFDocument::continueLastSearch()
{
    return continueSearch( d->m_lastSearchID );
}


void KPDFDocument::toggleBookmark( int n )
{
    KPDFPage * page = ( n < (int)pages_vector.count() ) ? pages_vector[ n ] : 0;
    if ( page )
    {
        page->setBookmark( !page->hasBookmark() );
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
            d->nextDocumentViewport = go->destViewport();

            // Explanation of why d->nextDocumentViewport is needed
            // all openRelativeFile does is launch a signal telling we
            // want to open another URL, the problem is that when the file is 
            // non local, the loading is done assynchronously so you can't
            // do a setViewport after the if as it was because you are doing the setViewport
            // on the old file and when the new arrives there is no setViewport for it and
            // it does not show anything

            // first open filename if link is pointing outside this document
            if ( go->isExternal() && !openRelativeFile( go->fileName() ) )
            {
                kdWarning() << "Link: Error opening '" << go->fileName() << "'." << endl;
                return;
            }
            else
            {
                if (d->nextDocumentViewport.pageNumber == -1) return;
                setViewport( d->nextDocumentViewport, -1, true );
                d->nextDocumentViewport = DocumentViewport();
            }

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
                        KMessageBox::information( widget(), i18n("The pdf file is trying to execute an external application and for your safety kpdf does not allow that.") );
                        return;
                    }
                }
                else
                {
                    // this case is a link pointing to an executable with no parameters
                    // core developers find unacceptable executing it even after asking the user
                    KMessageBox::information( widget(), i18n("The pdf file is trying to execute an external application and for your safety kpdf does not allow that.") );
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
                KMessageBox::information( widget(), i18n( "No application found for opening file of mimetype %1." ).arg( mime->name() ) );
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
                    emit quit();
                    break;
                case KPDFLinkAction::Presentation:
                    emit linkPresentation();
                    break;
                case KPDFLinkAction::EndPresentation:
                    emit linkEndPresentation();
                    break;
                case KPDFLinkAction::Find:
                    emit linkFind();
                    break;
                case KPDFLinkAction::GoToPage:
                    emit linkGoToPage();
                    break;
                case KPDFLinkAction::Close:
                    emit close();
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
                QString url = browse->url();

                // fix for #100366, documents with relative links that are the form of http:foo.pdf
                if (url.find("http:") == 0 && url.find("http://") == -1 && url.right(4) == ".pdf")
                {
                    openRelativeFile(url.mid(5));
                    return;
                }

                // Albert: this is not a leak!
                new KRun(url);
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

    if ( d->observers.contains( req->id ) )
    {
        // [MEM] 1.2 append memory allocation descriptor to the FIFO
        int memoryBytes = 4 * req->width * req->height;
        AllocatedPixmap * memoryPage = new AllocatedPixmap( req->id, req->pageNumber, memoryBytes );
        d->allocatedPixmapsFifo.append( memoryPage );
        d->allocatedPixmapsTotalMemory += memoryBytes;

        // 2. notify an observer that its pixmap changed
        d->observers[ req->id ]->notifyPageChanged( req->pageNumber, DocumentObserver::Pixmap );
    }
#ifndef NDEBUG
    else
        kdWarning() << "Receiving a done request for the defunct observer " << req->id << endl;
#endif

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
    switch ( KpdfSettings::memoryLevel() )
    {
        case KpdfSettings::EnumMemoryLevel::Low:
            memoryToFree = d->allocatedPixmapsTotalMemory;
            break;

        case KpdfSettings::EnumMemoryLevel::Normal:
            memoryToFree = d->allocatedPixmapsTotalMemory - getTotalMemory() / 3;
            clipValue = (d->allocatedPixmapsTotalMemory - getFreeMemory()) / 2;
            break;

        case KpdfSettings::EnumMemoryLevel::Aggressive:
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
    static QTime lastUpdate = QTime::currentTime();
    static int cachedValue = 0;

    if ( lastUpdate.secsTo( QTime::currentTime() ) <= 2 )
        return cachedValue;

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

    lastUpdate = QTime::currentTime();

    return ( cachedValue = ( 1024 * memoryFree ) );
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
                        pages_vector[ pageNumber ]->setBookmark( true );
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
    if ( !d->url.isValid() )
        return QString::null;
    
    return d->url.upURL().url() + fileName;
}

bool KPDFDocument::openRelativeFile( const QString & fileName )
{
    QString absFileName = giveAbsolutePath( fileName );
    if ( absFileName.isNull() )
        return false;

    kdDebug() << "openDocument: '" << absFileName << "'" << endl;

    emit openURL( absFileName );
    return true;
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
            if ( pages_vector[i]->hasBookmark() )
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
    if ( KpdfSettings::memoryLevel() != KpdfSettings::EnumMemoryLevel::Low &&
         d->allocatedPixmapsTotalMemory > 1024*1024 )
        cleanupPixmapMemory();
}


/** DocumentViewport **/

DocumentViewport::DocumentViewport( int n )
    : pageNumber( n )
{
    // default settings
    rePos.enabled = false;
    rePos.normalizedX = 0.5;
    rePos.normalizedY = 0.0;
    rePos.pos = Center;
    autoFit.enabled = false;
    autoFit.width = false;
    autoFit.height = false;
}

DocumentViewport::DocumentViewport( const QString & xmlDesc )
    : pageNumber( -1 )
{
    // default settings (maybe overridden below)
    rePos.enabled = false;
    rePos.normalizedX = 0.5;
    rePos.normalizedY = 0.0;
    rePos.pos = Center;
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
            rePos.enabled = true;
            rePos.normalizedX = token.section( ':', 1, 1 ).toDouble();
            rePos.normalizedY = token.section( ':', 2, 2 ).toDouble();
            rePos.pos = Center;
        }
        else if ( token.startsWith( "C2" ) )
        {
            rePos.enabled = true;
            rePos.normalizedX = token.section( ':', 1, 1 ).toDouble();
            rePos.normalizedY = token.section( ':', 2, 2 ).toDouble();
            if (token.section( ':', 3, 3 ).toInt() == 1) rePos.pos = Center;
            else rePos.pos = TopLeft;
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
    if ( rePos.enabled )
        s += QString( ";C2:" ) + QString::number( rePos.normalizedX ) +
             ':' + QString::number( rePos.normalizedY ) +
             ':' + QString::number( rePos.pos );
    // if has autofit enabled, save its state on string
    if ( autoFit.enabled )
        s += QString( ";AF1:" ) + (autoFit.width ? "T" : "F") +
             ':' + (autoFit.height ? "T" : "F");
    return s;
}

bool DocumentViewport::operator==( const DocumentViewport & vp ) const
{
    bool equal = ( pageNumber == vp.pageNumber ) &&
                 ( rePos.enabled == vp.rePos.enabled ) &&
                 ( autoFit.enabled == vp.autoFit.enabled );
    if ( !equal )
        return false;
    if ( rePos.enabled &&
         (( rePos.normalizedX != vp.rePos.normalizedX) ||
         ( rePos.normalizedY != vp.rePos.normalizedY ) || rePos.pos != vp.rePos.pos) )
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
