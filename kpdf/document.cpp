/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
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

// local includes
#include "document.h"
#include "page.h"
#include "link.h"
#include "settings.h"
#include "generator_pdf.h"  // PDF generator
//#include "generator_ps.H" // PS generator

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
        int currentPage;

        // memory check/free timer
        QTimer * memCheckTimer;

        // observers related (note: won't delete oservers)
        QMap< int, class ObserverData* > observers;
};

struct ObserverData
{
    // public data fields
    KPDFDocumentObserver * observer;
    QMap< int, int > pageMemory;
    int totalMemory;
    // public constructor: initialize data
    ObserverData( KPDFDocumentObserver * obs ) : observer( obs ), totalMemory( 0 ) {};
};

#define foreachObserver( cmd ) {\
    QMap< int, ObserverData * >::iterator it = d->observers.begin(), end = d->observers.end();\
    for ( ; it != end ; ++ it ) { (*it)->observer-> cmd ; } }


KPDFDocument::KPDFDocument()
    : generator( 0 ), d( new KPDFDocumentPrivate )
{
    d->currentPage = -1;
    d->searchPage = -1;
    d->memCheckTimer = new QTimer( this );
    connect( d->memCheckTimer, SIGNAL( timeout() ), this, SLOT( slotCheckMemory() ) );
}

KPDFDocument::~KPDFDocument()
{
    closeDocument();
    delete d;
}


bool KPDFDocument::openDocument( const QString & docFile )
{
    // docFile is always local so we can use QFile on it
    QFile fileReadTest( docFile );
    if ( !fileReadTest.open( IO_ReadOnly ) )
        return false;
    fileReadTest.close();

    // reset internal status and frees memory
    closeDocument();

    // create the generator based on the file's mimetype
    KMimeType::Ptr mime = KMimeType::findByPath( docFile );
    QString mimeName = mime->name();
    if ( mimeName == "application/pdf" )
        generator = new PDFGenerator();
    else if ( mimeName == "application/postscript" )
        kdError() << "PS generator not available" << endl;
    else
    {
        kdWarning() << "Unknown mimetype '" << mimeName << "'." << endl;
        return false;
    }
    // get notification of completed jobs
    connect( generator, SIGNAL( contentsChanged( int, int ) ),
             this, SLOT( slotGeneratedContents( int, int ) ) );

    // ask generator to open the document and return if unsuccessfull
    documentFileName = docFile;
    bool openOk = generator->loadDocument( docFile, pages_vector );
    if ( !openOk )
        return false;

    // start memory check timer
    d->memCheckTimer->start( 1000 );

    // check local directory for an overlay xml
    // TODO import overlay layers from XML
//  QString fileName = docFile.contains('/') ? docFile.section('/', -1, -1) : docFile;
//  fileName = "kpdf/" + QString::number(fileSize) + "." + fileName + ".xml";
//  QString localFN = locateLocal( "data", fileName );
//  kdDebug() << "Using '" << localFN << "' as overlay descriptor." << endl;

    // filter pages, setup observers and set the first page as current
    if ( pages_vector.size() > 0 )
    {
        processPageList( true );
        setCurrentPage( 0 );
    }
    return true;
}

void KPDFDocument::closeDocument()
{
    // stop memory check timer
    d->memCheckTimer->stop();

    // delete pages and clear 'pages_vector' container
    for ( uint i = 0; i < pages_vector.count() ; i++ )
        delete pages_vector[i];
    pages_vector.clear();

    // send an empty list to observers (to free their data)
    foreachObserver( pageSetup( pages_vector, true ) );

    // clear memory management data
    QMap< int, ObserverData * >::iterator oIt = d->observers.begin(), oEnd = d->observers.end();
    for ( ; oIt != oEnd ; ++oIt )
    {
        ObserverData * observerData = *oIt;
        observerData->pageMemory.clear();
        observerData->totalMemory = 0;
    }

    // delete contents generator
    delete generator;
    generator = 0;

    // reset internal variables
    d->currentPage = -1;
    d->searchPage = -1;
}


void KPDFDocument::addObserver( KPDFDocumentObserver * pObserver )
{
    // keep the pointer to the observer in a map
    d->observers[ pObserver->observerId() ] = new ObserverData( pObserver );

    // if the observer is added while a document is already opened, tell it
    if ( !pages_vector.isEmpty() )
        pObserver->pageSetup( pages_vector, true );
}

void KPDFDocument::removeObserver( KPDFDocumentObserver * pObserver )
{
    // remove observer from the map. it won't receive notifications anymore
    if ( d->observers.contains( pObserver->observerId() ) )
    {
        // free observer data
        int observerId = pObserver->observerId();
        QValueVector<KPDFPage*>::iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
            (*it)->deletePixmap( observerId );

        // delete observer
        delete d->observers[ observerId ];
        d->observers.remove( observerId );
    }
}

void KPDFDocument::reparseConfig()
{
    // reparse generator config and if something changed clear KPDFPages
    if ( generator && generator->reparseConfig() )
    {
        // invalidate pixmaps and send reload signals to observers
        QValueVector<KPDFPage*>::iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
            (*it)->deletePixmapsAndRects();
        foreachObserver( notifyPixmapsCleared() );
    }
}


const DocumentInfo * KPDFDocument::documentInfo() const
{
    return generator ? generator->documentInfo() : NULL;
}

const DocumentSynopsis * KPDFDocument::documentSynopsis() const
{
    return generator ? generator->documentSynopsis() : NULL;
}

const KPDFPage * KPDFDocument::page( uint n ) const
{
    return ( n < pages_vector.count() ) ? pages_vector[n] : 0;
}

uint KPDFDocument::currentPage() const
{
    return d->currentPage;
}

uint KPDFDocument::pages() const
{
    return pages_vector.size();
}

bool KPDFDocument::okToPrint() const
{
    return generator ? generator->allowed( Generator::Print ) : false;
}

QString KPDFDocument::getMetaData( const QString &key ) const
{
    return generator ? generator->getMetaData( key ) : QString();
}

void KPDFDocument::requestPixmaps( const QValueList< PixmapRequest * > & requests, bool async )
{
    if ( !generator )
        return;

    QValueList< PixmapRequest * >::const_iterator rIt = requests.begin(), rEnd = requests.end();
    for ( ; rIt != rEnd; ++rIt )
    {
        // set the 'page field' (see PixmapRequest) and check if request is valid
        PixmapRequest * request = *rIt;
        request->page = pages_vector[ request->pageNumber ];
        if ( !request->page || request->page->width() < 1 || request->page->height() < 1 )
            continue;

        // 1. Update statistics (pageMemory / totalMemory) adding this pixmap
        ObserverData * obs = d->observers[ request->id ];
        int pageNumber = request->pageNumber;
        if ( obs->pageMemory.contains( pageNumber ) )
            obs->totalMemory -= obs->pageMemory[ pageNumber ];
        int pixmapMemory = 4 * request->width * request->height / 1024;
        obs->pageMemory[ pageNumber ] = pixmapMemory;
        obs->totalMemory += pixmapMemory;

        // 2. Perform pre-cleaning if needed
        mCleanupMemory( request->id );

        // 3. Enqueue to Generator (that takes ownership of request)
        generator->requestPixmap( request, Settings::disableThreading() ? false : async );
    }
}

void KPDFDocument::requestTextPage( uint page )
{
    KPDFPage * kp = pages_vector[ page ];
    if ( !generator || !kp )
        return;

    // Memory management for TextPages

    generator->requestTextPage( kp );
}

void KPDFDocument::setCurrentPage( int page, const QRect & viewport )
{
    if ( page < 0 )
        page = 0;
    else if ( page > (int)pages_vector.count() )
        page = pages_vector.count() - 1;
    if ( page == d->currentPage )
        return;
    d->currentPage = page;
    foreachObserver( pageSetCurrent( page, viewport ) );
}

void KPDFDocument::findText( const QString & string, bool keepCase )
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
    int currentPage = d->currentPage;
    int pageCount = pages_vector.count();
    KPDFPage * foundPage = 0,
             * lastPage = (d->searchPage > -1) ? pages_vector[ d->searchPage ] : 0;
    if ( lastPage && d->searchPage == currentPage )
        if ( lastPage->hasText( d->searchText, d->searchCase, false ) )
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
                if ( KMessageBox::questionYesNo(0, i18n("End of document reached.\nContinue from the beginning?")) == KMessageBox::Yes )
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
        setCurrentPage( pageNumber );
        foreachObserver( notifyPixmapChanged( pageNumber ) );
    }
    else
        KMessageBox::information( 0, i18n("No matches found for '%1'.").arg(d->searchText) );
}

void KPDFDocument::findTextAll( const QString & pattern, bool keepCase )
{
    // if pattern is null, clear 'hilighted' attribute in all pages
    if ( pattern.isEmpty() )
        unHilightPages();

    // cache search pattern and perform a linear search/mark
    d->filterText = pattern;
    d->filterCase = keepCase;
    processPageList( false );
}

void KPDFDocument::toggleBookmark( int n )
{
    KPDFPage * page = ( n < (int)pages_vector.count() ) ? pages_vector[ n ] : 0;
    if ( page )
    {
        page->toggleAttribute( KPDFPage::Bookmark );
        foreachObserver( notifyPixmapChanged( n ) );
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
            KPDFLinkGoto::Viewport destVp = go->destViewport();

            // first open filename if link is pointing outside this document
            if ( go->isExternal() && !openRelativeFile( go->fileName() ) )
            {
                kdWarning() << "Link: Error opening '" << go->fileName() << "'." << endl;
                return;
            }

            // note: if external file is opened, 'link' doesn't exist anymore!
            setCurrentPage( destVp.page );  //TODO implement and use Viewport
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
                    setCurrentPage( 0 );
                    break;
                case KPDFLinkAction::PagePrev:
                    if ( d->currentPage > 0 )
                        setCurrentPage( d->currentPage - 1 );
                    break;
                case KPDFLinkAction::PageNext:
                    if ( d->currentPage < (int)pages_vector.count() - 1 )
                        setCurrentPage( d->currentPage + 1 );
                    break;
                case KPDFLinkAction::PageLast:
                    setCurrentPage( pages_vector.count() - 1 );
                    break;
                case KPDFLinkAction::HistoryBack:
                    {} //TODO
                    break;
                case KPDFLinkAction::HistoryForward:
                    {} //TODO
                    break;
                case KPDFLinkAction::Quit:
                    kapp->quit();
                    break;
            }
            } break;

        case KPDFLink::Browse: {
            const KPDFLinkBrowse * browse = static_cast< const KPDFLinkBrowse * >( link );
            // get service for web browsing TODO: check for "mailto:" links
            KService::Ptr ptr = KServiceTypeProfile::preferredService("text/html", "Application");
            KURL::List lst;
            // append 'url' parameter to the service and run it
            lst.append( browse->url() );
            KRun::run( *ptr, lst );
            } break;

        case KPDFLink::Movie:
            // TODO this
            break;
    }
}

bool KPDFDocument::print( KPrinter &printer )
{
    return generator ? generator->print( printer ) : false;
}


void KPDFDocument::mCleanupMemory( int observerId  )
{
    // get observer data for given id
    ObserverData * obs = d->observers[ observerId ];

    // choose memory parameters based on configuration profile
    int clipValue = 0;
    int memoryToFree = 0;
    switch ( Settings::memoryLevel() )
    {
        case Settings::EnumMemoryLevel::Low:
            memoryToFree = obs->totalMemory;
            break;

        case Settings::EnumMemoryLevel::Normal:
            clipValue = obs->totalMemory - mFreeMemory() / 3;
            if ( observerId == THUMBNAILS_ID )
                memoryToFree = obs->totalMemory - mTotalMemory() / 20;
            else
                memoryToFree = obs->totalMemory - mTotalMemory() / 5;
            break;

        case Settings::EnumMemoryLevel::Aggressive:
            clipValue = obs->totalMemory - mFreeMemory() / 2;
            break;
    }

    if ( clipValue > memoryToFree )
        memoryToFree = clipValue;

    // free memory. remove older data until we free enough memory
    int freed = 0;
    if ( memoryToFree > 0 )
    {
        QMap< int, int >::iterator it = obs->pageMemory.begin(), end = obs->pageMemory.end();
        while ( (it != end) && (memoryToFree > 0) )
        {
            int freeNumber = it.key();
            if ( obs->observer->canUnloadPixmap( freeNumber ) )
            {
                // update mem stats
                memoryToFree -= it.data();
                obs->totalMemory -= it.data();
                obs->pageMemory.remove( it );
                // delete pixmap
                pages_vector[ freeNumber ]->deletePixmap( observerId );
                freed++;
            }
            ++it;
        }
    }
    //kdDebug() << "Id:" << observerId << " [" << obs->totalMemory << "kB] Removed " << freed << " pages. " << obs->pageMemory.count() << " pages kept in memory." << endl;
}

int KPDFDocument::mTotalMemory()
{
    static int cachedValue = 0;
    if ( cachedValue )
        return cachedValue;

#ifdef __linux__
    // if /proc/meminfo doesn't exist, return 128MB
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( IO_ReadOnly ) )
        return (cachedValue = 131072);

    // read /proc/meminfo and sum up the contents of 'MemFree', 'Buffers'
    // and 'Cached' fields. consider swapped memory as used memory.
    QTextStream readStream( &memFile );
    while ( !readStream.atEnd() )
    {
        QString entry = readStream.readLine();
        if ( entry.startsWith( "MemTotal:" ) )
            return (cachedValue = entry.section( ' ', -2, -2 ).toInt());
    }
#endif
    return (cachedValue = 131072);
}

int KPDFDocument::mFreeMemory()
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
    return memoryFree;
#else
    // tell the memory is full.. will act as in LOW profile
    return 0;
#endif
}

QString KPDFDocument::giveAbsolutePath( const QString & fileName )
{
    if ( documentFileName.isEmpty() )
        return QString::null;

    // convert the pdf fileName to absolute using current pdf path
    QFileInfo currentInfo( documentFileName );
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
    foreachObserver( pageSetup( pages_vector, documentChanged ) );
}

void KPDFDocument::unHilightPages()
{
    if ( d->filterText.isEmpty() )
        return;

    d->filterText = QString::null;
    QValueVector<KPDFPage*>::iterator it = pages_vector.begin(), end = pages_vector.end();
    for ( ; it != end; ++it )
    {
        KPDFPage * page = *it;
        if ( page->attributes() & KPDFPage::Highlight )
        {
            page->clearAttribute( KPDFPage::Highlight );
            foreachObserver( notifyPixmapChanged( page->number() ) );
        }
    }
}


void KPDFDocument::slotCheckMemory()
{
    // perform the memory check for 'free mem dependant' profiles only
    if ( Settings::memoryLevel() == Settings::EnumMemoryLevel::Low )
        return;

    // for each observer going over 1MB of memory, invoke the manager
    QMap< int, ObserverData * >::iterator it = d->observers.begin(), end = d->observers.end();
    for ( ; it != end ; ++ it )
        if ( (*it)->totalMemory > 1024 )
            mCleanupMemory( it.key() /*observerId*/ );
}

void KPDFDocument::slotGeneratedContents( int id, int pageNumber )
{
    // notify an observer that its pixmap changed
    if ( d->observers.contains( id ) )
        d->observers[ id ]->observer->notifyPixmapChanged( pageNumber );
}

#include "document.moc"
