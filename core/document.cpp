/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *   Copyright (C) 2004-2005 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde/system includes
#include <QtCore/QtAlgorithms>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtGui/QApplication>

#include <kconfigdialog.h>
#include <kdebug.h>
#include <kfinddialog.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <ktoolinvocation.h>

// local includes
#include "chooseenginedialog.h"
#include "document.h"
#include "generator.h"
#include "link.h"
#include "observer.h"
#include "page.h"
#include "settings.h"

using namespace Okular;

// structures used internally by Document for local variables storage
class AllocatedPixmap;
class RunningSearch;

class Okular::DocumentPrivate
{
    public:
        DocumentPrivate()
          : m_lastSearchID( -1 ),
            allocatedPixmapsTotalMemory( 0 ),
            warnedOutOfMemory( false ),
            rotation( 0 ),
            memCheckTimer( 0 ),
            saveBookmarksTimer( 0 )
        {}

        // find descriptors, mapped by ID (we handle multiple searches)
        QMap< int, RunningSearch * > searches;
        int m_lastSearchID;

        // needed because for remote documents docFileName is a local file and
        // we want the remote url when the document refers to relativeNames
        KUrl url;

        // cached stuff
        QString docFileName;
        QString xmlFileName;

        // viewport stuff
        QLinkedList< DocumentViewport > viewportHistory;
        QLinkedList< DocumentViewport >::iterator viewportIterator;
        DocumentViewport nextDocumentViewport; // see Link::Goto for an explanation

        // observers / requests / allocator stuff
        QMap< int, DocumentObserver * > observers;
        QLinkedList< PixmapRequest * > pixmapRequestsStack;
        QLinkedList< AllocatedPixmap * > allocatedPixmapsFifo;
        int allocatedPixmapsTotalMemory;
        bool warnedOutOfMemory;

        // the rotation applied to the document, 0,1,2,3 * 90 degrees
        int rotation;

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
    RegularAreaRect continueOnMatch;
    QLinkedList< int > highlightedPages;

    // fields related to previous searches (used for 'continueSearch')
    QString cachedString;
    Document::SearchType cachedType;
    bool cachedCaseSensitive;
    bool cachedViewportMove;
    bool cachedNoDialogs;
    QColor cachedColor;
};

#define foreachObserver( cmd ) {\
    QMap< int, DocumentObserver * >::const_iterator it=d->observers.begin(), end=d->observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }


/** Document **/

Document::Document( QHash<QString, Generator*> * genList )
    : m_loadedGenerators ( genList ), generator( 0 ),  d( new DocumentPrivate )
{
    m_usingCachedGenerator = false;
}

Document::~Document()
{
    // delete generator, pages, and related stuff
    closeDocument();

    // delete the private structure
    delete d;
}

static bool kserviceMoreThan( const KService::Ptr &s1, const KService::Ptr &s2 )
{
    return s1->property( "X-KDE-Priority" ).toInt() > s2->property( "X-KDE-Priority" ).toInt();
}

bool Document::openDocument( const QString & docFile, const KUrl& url, const KMimeType::Ptr &mime )
{
    // docFile is always local so we can use QFile on it
    QFile fileReadTest( docFile );
    if ( !fileReadTest.open( QIODevice::ReadOnly ) )
    {
        d->docFileName.clear();
        return false;
    }
    // determine the related "xml document-info" filename
    d->url = url;
    d->docFileName = docFile;
    QString fn = docFile.contains('/') ? docFile.section('/', -1, -1) : docFile;
    fn = "kpdf/" + QString::number(fileReadTest.size()) + '.' + fn + ".xml";
    fileReadTest.close();
    d->xmlFileName = KStandardDirs::locateLocal( "data", fn );

    if (mime.count()<=0)
	return false;
    
    // 0. load Generator
    // request only valid non-disabled plugins suitable for the mimetype
    QString constraint("([X-KDE-Priority] > 0) and (exist Library)") ;
    KService::List offers = KMimeTypeTrader::self()->query(mime->name(),"okular/Generator",constraint);
    if (offers.isEmpty())
    {
        kWarning() << "No plugin for mimetype '" << mime->name() << "'." << endl;
	   return false;
    }
    int hRank=0;
    // order the offers: the offers with an higher priority come before
    qStableSort( offers.begin(), offers.end(), kserviceMoreThan );

    // best ranked offer search
    if (offers.count() > 1 && Settings::chooseGenerators() )
    {
        QStringList list;
        int count=offers.count();
        for (int i=0;i<count;++i)
        {
            list << offers.at(i)->name();
        }
        ChooseEngineDialog choose( list, mime, 0 );

        int retval = choose.exec();
        int index = choose.selectedGenerator();
        switch( retval )
        {
            case QDialog::Accepted:
                hRank=index;
                break;
            case QDialog::Rejected:
                return false;
                break;
        }
    }

    QString propName = offers.at(hRank)->name();
    m_usingCachedGenerator=false;
    generator=m_loadedGenerators->take(propName);
    if (!generator)
    {
        KLibLoader *loader = KLibLoader::self();
        if (!loader)
        {
            kWarning() << "Could not start library loader: '" << loader->lastErrorMessage() << "'." << endl;
            return false;
        }
        KLibrary *lib = loader->globalLibrary( QFile::encodeName( offers.at(hRank)->library() ) );
        if (!lib) 
        {
            kWarning() << "Could not load '" << offers.at(hRank)->library() << "' library." << endl;
            return false;
        }

        Generator* (*create_plugin)() = ( Generator* (*)() ) lib->symbol( "create_plugin" );
        generator = create_plugin();

        if ( !generator )
        {
            kWarning() << "Sth broke." << endl;
            return false;
        }
        if ( offers.at(hRank)->property( "X-KDE-okularHasInternalSettings" ).toBool() )
        {
            m_loadedGenerators->insert(propName,generator);
            m_usingCachedGenerator=true;
        }
        // end 
    }
    else
    {
        m_usingCachedGenerator=true;
    }
    generator->setDocument( this );

    // connect error reporting signals
    connect( generator, SIGNAL( error( const QString&, int ) ), this, SIGNAL( error( const QString&, int ) ) );
    connect( generator, SIGNAL( warning( const QString&, int ) ), this, SIGNAL( warning( const QString&, int ) ) );
    connect( generator, SIGNAL( notice( const QString&, int ) ), this, SIGNAL( notice( const QString&, int ) ) );

    // 1. load Document (and set busy cursor while loading)
    QApplication::setOverrideCursor( Qt::WaitCursor );
    bool openOk = generator->loadDocument( docFile, pages_vector );

    for ( int i = 0; i < pages_vector.count(); ++i )
        connect( pages_vector[ i ], SIGNAL( rotationFinished( int ) ),
                 this, SLOT( rotationFinished( int ) ) );

    QApplication::restoreOverrideCursor();
    if ( !openOk || pages_vector.size() <= 0 )
    {
        if (!m_usingCachedGenerator)
        {
            delete generator;
        }
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
    {
        (*d->viewportIterator) = DocumentViewport();
        if ( loadedViewport.pageNumber >= (int)pages_vector.size() )
            loadedViewport.pageNumber = pages_vector.size() - 1;
    }
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


QString Document::getXMLFile()
{
    if (generator)
        return generator->getXMLFile();
   
    return QString();
}

void Document::setupGUI(KActionCollection* ac, QToolBox* tBox )
{
    if (generator)
        generator->setupGUI(ac,tBox);
}

void Document::closeDocument()
{
    // close the current document and save document info if a document is still opened
    if ( generator && pages_vector.size() > 0 )
    {
        generator->closeDocument();
        saveDocumentInfo();
    }

    // stop timers
    if ( d->memCheckTimer )
        d->memCheckTimer->stop();
    if ( d->saveBookmarksTimer )
        d->saveBookmarksTimer->stop();

    if (generator)
        generator->freeGUI();
    if (!m_usingCachedGenerator)
    {
        // delete contents generator
        delete generator;
    }
    generator = 0;
    d->url = KUrl();
    // remove requests left in queue
    QLinkedList< PixmapRequest * >::const_iterator sIt = d->pixmapRequestsStack.begin();
    QLinkedList< PixmapRequest * >::const_iterator sEnd = d->pixmapRequestsStack.end();
    for ( ; sIt != sEnd; ++sIt )
        delete *sIt;
    d->pixmapRequestsStack.clear();

    // send an empty list to observers (to free their data)
    foreachObserver( notifySetup( QVector< Page * >(), true ) );

    // delete pages and clear 'pages_vector' container
    QVector< Page * >::const_iterator pIt = pages_vector.begin();
    QVector< Page * >::const_iterator pEnd = pages_vector.end();
    for ( ; pIt != pEnd; ++pIt )
        delete *pIt;
    pages_vector.clear();

    // clear 'memory allocation' descriptors
    QLinkedList< AllocatedPixmap * >::const_iterator aIt = d->allocatedPixmapsFifo.begin();
    QLinkedList< AllocatedPixmap * >::const_iterator aEnd = d->allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    d->allocatedPixmapsFifo.clear();

    // clear 'running searches' descriptors
    QMap< int, RunningSearch * >::const_iterator rIt = d->searches.begin();
    QMap< int, RunningSearch * >::const_iterator rEnd = d->searches.end();
    for ( ; rIt != rEnd; ++rIt )
        delete *rIt;
    d->searches.clear();

    // clear the visible areas and notify the observers
    QVector< VisiblePageRect * >::const_iterator vIt = page_rects.begin();
    QVector< VisiblePageRect * >::const_iterator vEnd = page_rects.end();
    for ( ; vIt != vEnd; ++vIt )
        delete *vIt;
    page_rects.clear();
    foreachObserver( notifyVisibleRectsChanged() );

    // reset internal variables

    d->viewportHistory.clear();
    d->viewportHistory.append( DocumentViewport() );
    d->viewportIterator = d->viewportHistory.begin();
    d->allocatedPixmapsTotalMemory = 0;
}

void Document::addObserver( DocumentObserver * pObserver )
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

void Document::notifyObservers (NotifyRequest * request)
{
    switch (request->type)
    {
        case DocumentObserver::Setup:
            foreachObserver( notifySetup( pages_vector, request->toggle ) );
            break;
        case DocumentObserver::Viewport:
            foreachObserver( notifyViewportChanged( request->toggle ) );
            break;
        case DocumentObserver::Page:
            foreachObserver( notifyPageChanged( request->page, request->flags ) );
            break;
        case DocumentObserver::Contents:
            foreachObserver( notifyContentsCleared( request->flags ) );
            break;
        case DocumentObserver::VisibleAreas:
            qDeleteAll(page_rects);
            page_rects = request->rects;
            foreachObserver( notifyVisibleRectsChanged() );
            break;
    }
}

void Document::removeObserver( DocumentObserver * pObserver )
{
    // remove observer from the map. it won't receive notifications anymore
    if ( d->observers.contains( pObserver->observerId() ) )
    {
        // free observer's pixmap data
        int observerId = pObserver->observerId();
        QVector<Page*>::const_iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
            (*it)->deletePixmap( observerId );

        // [MEM] free observer's allocation descriptors
        QLinkedList< AllocatedPixmap * >::iterator aIt = d->allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::iterator aEnd = d->allocatedPixmapsFifo.end();
        while ( aIt != aEnd )
        {
            AllocatedPixmap * p = *aIt;
            if ( p->id == observerId )
            {
                aIt = d->allocatedPixmapsFifo.erase( aIt );
                delete p;
            }
            else
                ++aIt;
        }

        // delete observer entry from the map
        d->observers.remove( observerId );
    }
}

void Document::reparseConfig()
{
    // reparse generator config and if something changed clear Pages
    if ( generator && generator->reparseConfig() )
    {
        // invalidate pixmaps
        QVector<Page*>::const_iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it ) {
            (*it)->deletePixmaps();
            (*it)->deleteRects();
        }

        // [MEM] remove allocation descriptors
        QLinkedList< AllocatedPixmap * >::const_iterator aIt = d->allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::const_iterator aEnd = d->allocatedPixmapsFifo.end();
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


bool Document::isOpened() const
{
    return generator;
}

bool Document::canConfigurePrinter( ) const
{
    return generator ? generator->canConfigurePrinter() : false;
}

const DocumentInfo * Document::documentInfo() const
{
    if (generator)
    {
        DocumentInfo *info = const_cast<DocumentInfo *>(generator->generateDocumentInfo());
        QString pagesSize = pagesSizeString();
        if (!pagesSize.isEmpty())
        {
            info->set( "pagesSize", pagesSize, i18n("Pages Size") );
        }
        return info;
    }
    else return NULL;
}

const DocumentSynopsis * Document::documentSynopsis() const
{
    return generator ? generator->generateDocumentSynopsis() : NULL;
}

const DocumentFonts * Document::documentFonts() const
{
    return generator ? generator->generateDocumentFonts() : NULL;
}

const QList<EmbeddedFile*> *Document::embeddedFiles() const
{
    return generator ? generator->embeddedFiles() : NULL;
}

const Page * Document::page( int n ) const
{
    return ( n < pages_vector.count() ) ? pages_vector[n] : 0;
}

const DocumentViewport & Document::viewport() const
{
    return (*d->viewportIterator);
}

const QVector< VisiblePageRect * > & Document::visiblePageRects() const
{
    return page_rects;
}

void Document::setVisiblePageRects( const QVector< VisiblePageRect * > & visiblePageRects, int excludeId )
{
    QVector< VisiblePageRect * >::const_iterator vIt = page_rects.begin();
    QVector< VisiblePageRect * >::const_iterator vEnd = page_rects.end();
    for ( ; vIt != vEnd; ++vIt )
        delete *vIt;
    page_rects = visiblePageRects;
    // notify change to all other (different from id) observers
    QMap< int, DocumentObserver * >::const_iterator it = d->observers.begin(), end = d->observers.end();
    for ( ; it != end ; ++ it )
        if ( it.key() != excludeId )
            (*it)->notifyVisibleRectsChanged();
}

uint Document::currentPage() const
{
    return (*d->viewportIterator).pageNumber;
}

uint Document::pages() const
{
    return pages_vector.size();
}

KUrl Document::currentDocument() const
{
    return d->url;
}

bool Document::isAllowed( int flags ) const
{
    return generator ? generator->isAllowed( flags ) : false;
}

bool Document::supportsSearching() const
{
    return generator ? generator->supportsSearching() : false;
}

bool Document::supportsRotation() const
{
    return generator ? generator->supportsRotation() : false;
}

bool Document::supportsPaperSizes() const
{
    return generator ? generator->supportsPaperSizes() : false;
}

QStringList Document::paperSizes() const
{
    return generator ? generator->paperSizes() : QStringList();
}

bool Document::canExportToText() const
{
    if ( !generator )
        return false;

    const ExportFormat::List formats = generator->exportFormats();
    for ( int i = 0; i < formats.count(); ++i ) {
        if ( formats[ i ].mimeType()->name() == QLatin1String( "text/plain" ) )
            return true;
    }

    return false;
}

bool Document::exportToText( const QString& fileName ) const
{
    if ( !generator )
        return false;

    const ExportFormat::List formats = generator->exportFormats();
    for ( int i = 0; i < formats.count(); ++i ) {
        if ( formats[ i ].mimeType()->name() == QLatin1String( "text/plain" ) )
            return generator->exportTo( fileName, formats[ i ] );
    }

    return false;
}

ExportFormat::List Document::exportFormats() const
{
    return generator ? generator->exportFormats() : ExportFormat::List();
}

bool Document::exportTo( const QString& fileName, const ExportFormat& format ) const
{
    return generator ? generator->exportTo( fileName, format ) : false;
}

bool Document::historyAtBegin() const
{
    return d->viewportIterator == d->viewportHistory.begin();
}

bool Document::historyAtEnd() const
{
    return d->viewportIterator == --(d->viewportHistory.end());
}

QVariant Document::getMetaData( const QString & key, const QVariant & option ) const
{
    return generator ? generator->metaData( key, option ) : QVariant();
}

int Document::rotation() const
{
    return d->rotation;
}

QSizeF Document::allPagesSize() const
{
    bool allPagesSameSize = true;
    QSizeF size;
    for (int i = 0; allPagesSameSize && i < pages_vector.count(); ++i)
    {
        Page *p = pages_vector[i];
        if (i == 0) size = QSizeF(p->width(), p->height());
        else
        {
            allPagesSameSize = (size == QSizeF(p->width(), p->height()));
        }
    }
    if (allPagesSameSize) return size;
    else return QSizeF();
}

QString Document::pageSizeString(int page) const
{
    if (generator)
    {
        if (generator->pagesSizeMetric() != Generator::None)
        {
            Page *p = pages_vector[page];
            return localizedSize(QSizeF(p->width(), p->height()));
        }
    }
    return QString();
}

void Document::requestPixmaps( const QLinkedList< PixmapRequest * > & requests )
{
    if ( !generator )
    {
        // delete requests..
        QLinkedList< PixmapRequest * >::const_iterator rIt = requests.begin(), rEnd = requests.end();
        for ( ; rIt != rEnd; ++rIt )
            delete *rIt;
        // ..and return
        return;
    }

    // 1. [CLEAN STACK] remove previous requests of requesterID
    int requesterID = requests.first()->id();
    QLinkedList< PixmapRequest * >::iterator sIt = d->pixmapRequestsStack.begin(), sEnd = d->pixmapRequestsStack.end();
    while ( sIt != sEnd )
    {
        if ( (*sIt)->id() == requesterID )
        {
            // delete request and remove it from stack
            delete *sIt;
            sIt = d->pixmapRequestsStack.erase( sIt );
        }
        else
            ++sIt;
    }

    // 2. [ADD TO STACK] add requests to stack
    bool threadingDisabled = !Settings::enableThreading();
    QLinkedList< PixmapRequest * >::const_iterator rIt = requests.begin(), rEnd = requests.end();
    for ( ; rIt != rEnd; ++rIt )
    {
        // set the 'page field' (see PixmapRequest) and check if it is valid
        PixmapRequest * request = *rIt;
        kWarning() << "request id=" << request->id() << " " <<request->width() << "x" << request->height() << "@" << request->pageNumber() << endl;
        if ( pages_vector.value( request->pageNumber() ) == 0 )
        {
            // skip requests referencing an invalid page (must not happen)
            delete request;
            continue;
        }

        request->setPage( pages_vector.value( request->pageNumber() ) );

        if ( !request->asynchronous() )
            request->setPriority( 0 );

        if ( request->asynchronous() && threadingDisabled )
            request->setAsynchronous( false );

        // add request to the 'stack' at the right place
        if ( !request->priority() )
            // add priority zero requests to the top of the stack
            d->pixmapRequestsStack.append( request );
        else
        {
            // insert in stack sorted by priority
            sIt = d->pixmapRequestsStack.begin();
            sEnd = d->pixmapRequestsStack.end();
            while ( sIt != sEnd && (*sIt)->priority() >= request->priority() )
                ++sIt;
            d->pixmapRequestsStack.insert( sIt, request );
        }
    }

    // 3. [START FIRST GENERATION] if <NO>generator is ready, start a new generation,
    // or else (if gen is running) it will be started when the new contents will
    //come from generator (in requestDone())</NO>
    // all handling of requests put into sendGeneratorRequest
    //    if ( generator->canGeneratePixmap() )
        sendGeneratorRequest();
}

void Document::requestTextPage( uint page )
{
    Page * kp = pages_vector[ page ];
    if ( !generator || !kp )
        return;

    // Memory management for TextPages

    generator->generateSyncTextPage( kp );
}

void Document::addPageAnnotation( int page, Annotation * annotation )
{
    // find out the page to attach annotation
    Page * kp = pages_vector[ page ];
    if ( !generator || !kp )
        return;

    // add annotation to the page
    kp->addAnnotation( annotation );

    // notify observers about the change
    foreachObserver( notifyPageChanged( page, DocumentObserver::Annotations ) );
}

void Document::modifyPageAnnotation( int page, Annotation * newannotation )
{
    //TODO: modify annotations
    
    // find out the page
    Page * kp = pages_vector[ page ];
    if ( !generator || !kp )
        return;
    
    kp->modifyAnnotation( newannotation );
    // notify observers about the change
    foreachObserver( notifyPageChanged( page, DocumentObserver::Annotations ) );
}


void Document::removePageAnnotation( int page, Annotation * annotation )
{
    // find out the page
    Page * kp = pages_vector[ page ];
    if ( !generator || !kp )
        return;

    // try to remove the annotation
    if ( kp->removeAnnotation( annotation ) )
    {
        // in case of success, notify observers about the change
        foreachObserver( notifyPageChanged( page, DocumentObserver::Annotations ) );
    }
}

void Document::removePageAnnotations( int page, QList< Annotation * > annotations )
{
    // find out the page
    Page * kp = pages_vector[ page ];
    if ( !generator || !kp )
        return;

    bool changed = false;
    foreach ( Annotation * annotation, annotations )
    {
        // try to remove the annotation
        if ( kp->removeAnnotation( annotation ) )
        {
            changed = true;
        }
    }
    if ( changed )
    {
        // in case we removed even only one annotation, notify observers about the change
        foreachObserver( notifyPageChanged( page, DocumentObserver::Annotations ) );
    }
}

void Document::setPageTextSelection( int page, RegularAreaRect * rect, const QColor & color )
{
    Page * kp = pages_vector[ page ];
    if ( !generator || !kp )
        return;

    // add or remove the selection basing whether rect is null or not
    if ( rect )
        kp->setTextSelections( rect, color );
    else
        kp->deleteTextSelections();

    // notify observers about the change
    foreachObserver( notifyPageChanged( page, DocumentObserver::TextSelection ) );
}

/* REFERENCE IMPLEMENTATION: better calling setViewport from other code
void Document::setNextPage()
{
    // advance page and set viewport on observers
    if ( (*d->viewportIterator).pageNumber < (int)pages_vector.count() - 1 )
        setViewport( DocumentViewport( (*d->viewportIterator).pageNumber + 1 ) );
}

void Document::setPrevPage()
{
    // go to previous page and set viewport on observers
    if ( (*d->viewportIterator).pageNumber > 0 )
        setViewport( DocumentViewport( (*d->viewportIterator).pageNumber - 1 ) );
}
*/
void Document::setViewportPage( int page, int excludeId, bool smoothMove )
{
    // clamp page in range [0 ... numPages-1]
    if ( page < 0 )
        page = 0;
    else if ( page > (int)pages_vector.count() )
        page = pages_vector.count() - 1;

    // make a viewport from the page and broadcast it
    setViewport( DocumentViewport( page ), excludeId, smoothMove );
}

void Document::setViewport( const DocumentViewport & viewport, int excludeId, bool smoothMove )
{
    // if already broadcasted, don't redo it
    DocumentViewport & oldViewport = *d->viewportIterator;
    // disabled by enrico on 2005-03-18 (less debug output)
    //if ( viewport == oldViewport )
    //    kDebug() << "setViewport with the same viewport." << endl;

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
        d->viewportIterator = d->viewportHistory.insert( d->viewportHistory.end(), viewport );
    }

    // notify change to all other (different from id) observers
    QMap< int, DocumentObserver * >::const_iterator it = d->observers.begin(), end = d->observers.end();
    for ( ; it != end ; ++ it )
        if ( it.key() != excludeId )
            (*it)->notifyViewportChanged( smoothMove );

    // [MEM] raise position of currently viewed page in allocation queue
    if ( d->allocatedPixmapsFifo.count() > 1 )
    {
        const int page = viewport.pageNumber;
        QLinkedList< AllocatedPixmap * > viewportPixmaps;
        QLinkedList< AllocatedPixmap * >::iterator aIt = d->allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::iterator aEnd = d->allocatedPixmapsFifo.end();
        while ( aIt != aEnd )
        {
            if ( (*aIt)->page == page )
            {
                viewportPixmaps.append( *aIt );
                aIt = d->allocatedPixmapsFifo.erase( aIt );
                continue;
            }
            ++aIt;
        }
        if ( !viewportPixmaps.isEmpty() )
            d->allocatedPixmapsFifo += viewportPixmaps;
    }
}

void Document::setPrevViewport()
// restore viewport from the history
{
    if ( d->viewportIterator != d->viewportHistory.begin() )
    {
        // restore previous viewport and notify it to observers
        --d->viewportIterator;
        foreachObserver( notifyViewportChanged( true ) );
    }
}

void Document::setNextViewport()
// restore next viewport from the history
{
    QLinkedList< DocumentViewport >::const_iterator nextIterator = d->viewportIterator;
    ++nextIterator;
    if ( nextIterator != d->viewportHistory.end() )
    {
        // restore next viewport and notify it to observers
        ++d->viewportIterator;
        foreachObserver( notifyViewportChanged( true ) );
    }
}

void Document::setNextDocumentViewport( const DocumentViewport & viewport )
{
    d->nextDocumentViewport = viewport;
}

bool Document::searchText( int searchID, const QString & text, bool fromStart, bool caseSensitive,
                               SearchType type, bool moveViewport, const QColor & color, bool noDialogs )
{
    // safety checks: don't perform searches on empty or unsearchable docs
    if ( !generator || !generator->supportsSearching() || pages_vector.isEmpty() )
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
    QLinkedList< int > pagesToNotify;

    // remove highlights from pages and queue them for notifying changes
    pagesToNotify += s->highlightedPages;
    QLinkedList< int >::const_iterator it = s->highlightedPages.begin(), end = s->highlightedPages.end();
    for ( ; it != end; ++it )
        pages_vector[ *it ]->deleteHighlights( searchID );
    s->highlightedPages.clear();

    // set hourglass cursor
    QApplication::setOverrideCursor( Qt::WaitCursor );

    // 1. ALLDOC - proces all document marking pages
    if ( type == AllDoc )
    {
        // search and highlight 'text' (as a solid phrase) on all pages
        QVector< Page * >::const_iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
        {
            // get page (from the first to the last)
            Page * page = *it;
            int pageNumber = page->number();

            // request search page if needed
            if ( !page->hasSearchPage() )
                requestTextPage( pageNumber );

            // loop on a page adding highlights for all found items
            bool addedHighlights = false;
            RegularAreaRect * lastMatch = 0;
            while ( 1 )
            {
                if ( lastMatch )
                    lastMatch = page->findText( searchID, text, NextRes, caseSensitive, lastMatch );
                else
                    lastMatch = page->findText( searchID, text, FromTop, caseSensitive );

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
        Page * lastPage = fromStart ? 0 : pages_vector[ currentPage ];

        // continue checking last SearchPage first (if it is the current page)
        RegularAreaRect * match = 0;
        if ( lastPage && lastPage->number() == s->continueOnPage )
        {
            if ( newText )
                match = lastPage->findText( searchID, text, FromTop, caseSensitive );
            else
                match = lastPage->findText( searchID, text, NextRes, caseSensitive, &s->continueOnMatch );
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
                    if ( noDialogs || KMessageBox::questionYesNo(0, i18n("End of document reached.\nContinue from the beginning?"), QString::null, KStdGuiItem::cont(), KStdGuiItem::cancel()) == KMessageBox::Yes )
                        currentPage = 0;
                    else
                        break;
                }
                // get page
                Page * page = pages_vector[ currentPage ];
                // request search page if needed
                if ( !page->hasSearchPage() )
                    requestTextPage( page->number() );
                // if found a match on the current page, end the loop
                if ( ( match = page->findText( searchID, text, FromTop, caseSensitive ) ) )
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

            // ..move the viewport to show the first of the searched word sequence centered
            if ( moveViewport )
            {
                DocumentViewport searchViewport( currentPage );
                searchViewport.rePos.enabled = true;
                searchViewport.rePos.normalizedX = (match->first()->left + match->first()->right) / 2.0;
                searchViewport.rePos.normalizedY = (match->first()->top + match->first()->bottom) / 2.0;
                setViewport( searchViewport, -1, true );
            }
        }
        else if ( !noDialogs )
            KMessageBox::information( 0, i18n( "No matches found for '%1'.", text ) );
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
        QStringList words = text.split( " ", QString::SkipEmptyParts );
        int wordsCount = words.count(),
            hueStep = (wordsCount > 1) ? (60 / (wordsCount - 1)) : 60,
            baseHue, baseSat, baseVal;
        color.getHsv( &baseHue, &baseSat, &baseVal );
        QVector< Page * >::const_iterator it = pages_vector.begin(), end = pages_vector.end();
        for ( ; it != end; ++it )
        {
            // get page (from the first to the last)
            Page * page = *it;
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
                QColor wordColor = QColor::fromHsv( newHue, baseSat, baseVal );
                RegularAreaRect * lastMatch = 0;
                // add all highlights for current word
                bool wordMatched = false;
                while ( 1 )
                {
                    if ( lastMatch )
                        lastMatch = page->findText( searchID, word, NextRes, caseSensitive, lastMatch );
                    else
                        lastMatch = page->findText( searchID, word, FromTop, caseSensitive);

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
    QLinkedList< int >::const_iterator nIt = pagesToNotify.begin(), nEnd = pagesToNotify.end();
    for ( ; nIt != nEnd; ++nIt )
        foreachObserver( notifyPageChanged( *nIt, DocumentObserver::Highlights ) );

    // return if search has found one or more matches
    return foundAMatch;
}

bool Document::continueSearch( int searchID )
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

void Document::resetSearch( int searchID )
{
    // check if searchID is present in runningSearches
    if ( !d->searches.contains( searchID ) )
        return;

    // get previous parameters for search
    RunningSearch * s = d->searches[ searchID ];

    // unhighlight pages and inform observers about that
    QLinkedList< int >::const_iterator it = s->highlightedPages.begin(), end = s->highlightedPages.end();
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

bool Document::continueLastSearch()
{
    return continueSearch( d->m_lastSearchID );
}


void Document::toggleBookmark( int n )
{
    Page * page = ( n < (int)pages_vector.count() ) ? pages_vector[ n ] : 0;
    if ( page )
    {
        page->setBookmark( !page->hasBookmark() );
        foreachObserver( notifyPageChanged( n, DocumentObserver::Bookmark ) );
    }
}

void Document::processLink( const Link * link )
{
    if ( !link )
        return;

    switch( link->linkType() )
    {
        case Link::Goto: {
            const LinkGoto * go = static_cast< const LinkGoto * >( link );
            d->nextDocumentViewport = go->destViewport();

            // Explanation of why d->nextDocumentViewport is needed:
            // all openRelativeFile does is launch a signal telling we
            // want to open another URL, the problem is that when the file is 
            // non local, the loading is done assynchronously so you can't
            // do a setViewport after the if as it was because you are doing the setViewport
            // on the old file and when the new arrives there is no setViewport for it and
            // it does not show anything

            // first open filename if link is pointing outside this document
            if ( go->isExternal() && !openRelativeFile( go->fileName() ) )
            {
                kWarning() << "Link: Error opening '" << go->fileName() << "'." << endl;
                return;
            }
            else
            {
                // skip local links that point to nowhere (broken ones)
                if (d->nextDocumentViewport.pageNumber == -1)
                    return;

                setViewport( d->nextDocumentViewport, -1, true );
                d->nextDocumentViewport = DocumentViewport();
            }

            } break;

        case Link::Execute: {
            const LinkExecute * exe  = static_cast< const LinkExecute * >( link );
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
                        KMessageBox::information( 0, i18n("The pdf file is trying to execute an external application and for your safety okular does not allow that.") );
                        return;
                    }
                }
                else
                {
                    // this case is a link pointing to an executable with no parameters
                    // core developers find unacceptable executing it even after asking the user
                    KMessageBox::information( 0, i18n("The pdf file is trying to execute an external application and for your safety okular does not allow that.") );
                    return;
                }
            }

            KService::Ptr ptr = KMimeTypeTrader::self()->preferredService( mime->name(), "Application" );
            if ( ptr )
            {
                KUrl::List lst;
                lst.append( fileName );
                KRun::run( *ptr, lst, 0 );
            }
            else
                KMessageBox::information( 0, i18n( "No application found for opening file of mimetype %1.", mime->name() ) );
            } break;

        case Link::Action: {
            const LinkAction * action = static_cast< const LinkAction * >( link );
            switch( action->actionType() )
            {
                case LinkAction::PageFirst:
                    setViewportPage( 0 );
                    break;
                case LinkAction::PagePrev:
                    if ( (*d->viewportIterator).pageNumber > 0 )
                        setViewportPage( (*d->viewportIterator).pageNumber - 1 );
                    break;
                case LinkAction::PageNext:
                    if ( (*d->viewportIterator).pageNumber < (int)pages_vector.count() - 1 )
                        setViewportPage( (*d->viewportIterator).pageNumber + 1 );
                    break;
                case LinkAction::PageLast:
                    setViewportPage( pages_vector.count() - 1 );
                    break;
                case LinkAction::HistoryBack:
                    setPrevViewport();
                    break;
                case LinkAction::HistoryForward:
                    setNextViewport();
                    break;
                case LinkAction::Quit:
                    emit quit();
                    break;
                case LinkAction::Presentation:
                    emit linkPresentation();
                    break;
                case LinkAction::EndPresentation:
                    emit linkEndPresentation();
                    break;
                case LinkAction::Find:
                    emit linkFind();
                    break;
                case LinkAction::GoToPage:
                    emit linkGoToPage();
                    break;
                case LinkAction::Close:
                    emit close();
                    break;
            }
            } break;

        case Link::Browse: {
            const LinkBrowse * browse = static_cast< const LinkBrowse * >( link );
            // if the url is a mailto one, invoke mailer
            if ( browse->url().startsWith( "mailto:", Qt::CaseInsensitive ) )
                KToolInvocation::invokeMailer( browse->url() );
            else
            {
                QString url = browse->url();

                // fix for #100366, documents with relative links that are the form of http:foo.pdf
                if (url.indexOf("http:") == 0 && url.indexOf("http://") == -1 && url.right(4) == ".pdf")
                {
                    openRelativeFile(url.mid(5));
                    return;
                }

                // Albert: this is not a leak! 
                // TODO: find a widget to pass as second parameter
                new KRun( KUrl(url), 0 ); 
            }
            } break;

        case Link::Movie:
            //const LinkMovie * browse = static_cast< const LinkMovie * >( link );
            // TODO this (Movie link)
            break;
    }
}

void Document::processSourceReference( const SourceReference * ref )
{
    if ( !ref )
        return;

    if ( !QFile::exists( ref->fileName() ) )
    {
        kDebug() << "No such file: '" << ref->fileName() << "'" << endl;
        return;
    }

    static QHash< int, QString > editors;
    // init the editors table if empty (on first run, usually)
    if ( editors.isEmpty() )
    {
        editors[ Settings::EnumExternalEditor::Kate ] =
            QLatin1String( "kate --use --line %l --column %c" );
        editors[ Settings::EnumExternalEditor::Scite ] =
            QLatin1String( "scite %f \"-goto:%l,%c\"" );
    }

    QHash< int, QString >::const_iterator it = editors.find( Settings::externalEditor() );
    QString p;
    if ( it != editors.end() )
        p = *it;
    else
        p = Settings::externalEditorCommand();
    // custom editor not yet configured
    if ( p.isEmpty() )
        return;

    // replacing the placeholders
    p.replace( QLatin1String( "%l" ), QString::number( ref->row() ) );
    p.replace( QLatin1String( "%c" ), QString::number( ref->column() ) );
    if ( p.indexOf( QLatin1String( "%f" ) ) > -1 )
      p.replace( QLatin1String( "%f" ), ref->fileName() );
    else
      p.append( QLatin1String( " " ) + ref->fileName() );

    // paranoic checks
    if ( p.isEmpty() || p.trimmed() == ref->fileName() )
        return;

    QProcess::startDetached( p );
}

bool Document::print( KPrinter &printer )
{
    return generator ? generator->print( printer ) : false;
}

void Document::requestDone( PixmapRequest * req )
{
#ifndef NDEBUG
    if ( !generator->canGeneratePixmap( req->asynchronous() ) )
        kDebug() << "requestDone with generator not in READY state." << endl;
#endif

    // [MEM] 1.1 find and remove a previous entry for the same page and id
    QLinkedList< AllocatedPixmap * >::iterator aIt = d->allocatedPixmapsFifo.begin();
    QLinkedList< AllocatedPixmap * >::iterator aEnd = d->allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        if ( (*aIt)->page == req->pageNumber() && (*aIt)->id == req->id() )
        {
            AllocatedPixmap * p = *aIt;
            d->allocatedPixmapsFifo.erase( aIt );
            d->allocatedPixmapsTotalMemory -= p->memory;
            delete p;
            break;
        }

    // [MEM] 1.2 append memory allocation descriptor to the FIFO
    int memoryBytes = 4 * req->width() * req->height();
    AllocatedPixmap * memoryPage = new AllocatedPixmap( req->id(), req->pageNumber(), memoryBytes );
    d->allocatedPixmapsFifo.append( memoryPage );
    d->allocatedPixmapsTotalMemory += memoryBytes;

    // 2. notify an observer that its pixmap changed
    if ( d->observers.contains( req->id() ) )
        d->observers[ req->id() ]->notifyPageChanged( req->pageNumber(), DocumentObserver::Pixmap );

    // 3. delete request
    delete req;

    // 4. start a new generation if some is pending
    if ( !d->pixmapRequestsStack.isEmpty() )
        sendGeneratorRequest();
}

void Document::sendGeneratorRequest()
{
    // find a request
    PixmapRequest * request = 0;
    while ( !d->pixmapRequestsStack.isEmpty() && !request )
    {
        PixmapRequest * r = d->pixmapRequestsStack.last();
        if (!r)
        {
            d->pixmapRequestsStack.pop_back();
        }
        // request only if page isn't already present or request has invalid id
        else if ( r->page()->hasPixmap( r->id(), r->width(), r->height() ) || r->id() <= 0 || r->id() >= MAX_OBSERVER_ID)
        {
            d->pixmapRequestsStack.pop_back();
            delete r;
        }
        else if ( (long)r->width() * (long)r->height() > 20000000L )
        {
            d->pixmapRequestsStack.pop_back();
            if ( !d->warnedOutOfMemory )
            {
                kWarning() << "Running out of memory on page " << r->pageNumber()
                    << " (" << r->width() << "x" << r->height() << " px);" << endl;
                kWarning() << "this message will be reported only once." << endl;
                d->warnedOutOfMemory = true;
            }
            delete r;
        }
        else
            request = r;
    }

    // if no request found (or already generated), return
    if ( !request )
        return;

    // [MEM] preventive memory freeing
    int pixmapBytes = 4 * request->width() * request->height();
    if ( pixmapBytes > (1024 * 1024) )
        cleanupPixmapMemory( pixmapBytes );

    // submit the request to the generator
    if ( generator->canGeneratePixmap( request->asynchronous() ) )
    {
        kWarning() << "sending request id=" << request->id() << " " <<request->width() << "x" << request->height() << "@" << request->pageNumber() << " async == " << request->asynchronous() << endl;
        d->pixmapRequestsStack.removeAll ( request );

        if ( d->rotation % 2 )
            request->swap();

        generator->generatePixmap ( request );
    }
    else
        // pino (7/4/2006): set the polling interval from 10 to 30
        QTimer::singleShot( 30, this, SLOT(sendGeneratorRequest()) );
}

QString Document::pagesSizeString() const
{
    if (generator)
    {
        if (generator->pagesSizeMetric() != Generator::None)
        {
            QSizeF size = allPagesSize();
            if (size.isValid()) return localizedSize(size);
            else return QString();
        }
        else return QString();
    }
    else return QString();
}

QString Document::localizedSize(const QSizeF &size) const
{
    double inchesWidth, inchesHeight;
    switch (generator->pagesSizeMetric())
    {
        case Generator::Points:
            inchesWidth = size.width() / 72.0;
            inchesHeight = size.height() / 72.0;
        break;

        case Generator::None:
        break;
    }
    if (KGlobal::locale()->measureSystem() == KLocale::Imperial)
    {
        return i18n("%1 x %2 in", inchesWidth, inchesHeight);
    }
    else
    {
        return i18n("%1 x %2 mm", inchesWidth * 25.4, inchesHeight * 25.4);
    }
}

void Document::cleanupPixmapMemory( int /*sure? bytesOffset*/ )
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
        QLinkedList< AllocatedPixmap * >::iterator pIt = d->allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::iterator pEnd = d->allocatedPixmapsFifo.end();
        while ( (pIt != pEnd) && (memoryToFree > 0) )
        {
            AllocatedPixmap * p = *pIt;
            if ( d->observers[ p->id ]->canUnloadPixmap( p->page ) )
            {
                // update internal variables
                pIt = d->allocatedPixmapsFifo.erase( pIt );
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

int Document::getTotalMemory()
{
    static int cachedValue = 0;
    if ( cachedValue )
        return cachedValue;

#ifdef __linux__
    // if /proc/meminfo doesn't exist, return 128MB
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( QIODevice::ReadOnly ) )
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

int Document::getFreeMemory()
{
    static QTime lastUpdate = QTime::currentTime();
    static int cachedValue = 0;

    if ( lastUpdate.secsTo( QTime::currentTime() ) <= 2 )
        return cachedValue;

#ifdef __linux__
    // if /proc/meminfo doesn't exist, return MEMORY FULL
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( QIODevice::ReadOnly ) )
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

    return ( cachedValue = (1024 * memoryFree) );
#else
    // tell the memory is full.. will act as in LOW profile
    return 0;
#endif
}

void Document::loadDocumentInfo()
// note: load data and stores it internally (document or pages). observers
// are still uninitialized at this point so don't access them
{
    //kDebug() << "Using '" << d->xmlFileName << "' as document info file." << endl;
    QFile infoFile( d->xmlFileName );
    if ( !infoFile.exists() || !infoFile.open( QIODevice::ReadOnly ) )
        return;

    // Load DOM from XML file
    QDomDocument doc( "documentInfo" );
    if ( !doc.setContent( &infoFile ) )
    {
        kDebug() << "Can't load XML pair! Check for broken xml." << endl;
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

        // Restore page attributes (bookmark, annotations, ...) from the DOM
        if ( catName == "pageList" )
        {
            QDomNode pageNode = topLevelNode.firstChild();
            while ( pageNode.isElement() )
            {
                QDomElement pageElement = pageNode.toElement();
                if ( pageElement.hasAttribute( "number" ) )
                {
                    // get page number (node's attribute)
                    bool ok;
                    int pageNumber = pageElement.attribute( "number" ).toInt( &ok );

                    // pass the domElement to the right page, to read config data from
                    if ( ok && pageNumber >= 0 && pageNumber < (int)pages_vector.count() )
                        pages_vector[ pageNumber ]->restoreLocalContents( pageElement );
                }
                pageNode = pageNode.nextSibling();
            }
        }

        // Restore 'general info' from the DOM
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
                            d->viewportIterator = d->viewportHistory.insert( d->viewportHistory.end(), 
                                    DocumentViewport( vpString ) );
                        }
                        historyNode = historyNode.nextSibling();
                    }
                    // consistancy check
                    if ( d->viewportHistory.isEmpty() )
                        d->viewportIterator = d->viewportHistory.insert( d->viewportHistory.end(), DocumentViewport() );
                }
                infoNode = infoNode.nextSibling();
            }
        }

        topLevelNode = topLevelNode.nextSibling();
    } // </documentInfo>
}

QString Document::giveAbsolutePath( const QString & fileName )
{
    if ( !d->url.isValid() )
        return QString();

    return d->url.upUrl().url() + fileName;
}

bool Document::openRelativeFile( const QString & fileName )
{
    QString absFileName = giveAbsolutePath( fileName );
    if ( absFileName.isEmpty() )
        return false;

    kDebug() << "openDocument: '" << absFileName << "'" << endl;

    emit openUrl( absFileName );
    return true;
}


void Document::saveDocumentInfo() const
{
    if ( d->docFileName.isEmpty() )
        return;

    QFile infoFile( d->xmlFileName );
    if (infoFile.open( QIODevice::WriteOnly | QIODevice::Truncate) )
    {
        // 1. Create DOM
        QDomDocument doc( "documentInfo" );
        QDomElement root = doc.createElement( "documentInfo" );
        doc.appendChild( root );

        // 2.1. Save page attributes (bookmark state, annotations, ... ) to DOM
        QDomElement pageList = doc.createElement( "pageList" );
        root.appendChild( pageList );
        // <page list><page number='x'>.... </page> save pages that hold data
        QVector< Page * >::const_iterator pIt = pages_vector.begin(), pEnd = pages_vector.end();
        for ( ; pIt != pEnd; ++pIt )
            (*pIt)->saveLocalContents( pageList, doc );

        // 2.2. Save document info (current viewport, history, ... ) to DOM
        QDomElement generalInfo = doc.createElement( "generalInfo" );
        root.appendChild( generalInfo );
        // <general info><history> ... </history> save history up to 10 viewports
        QLinkedList< DocumentViewport >::const_iterator backIterator = d->viewportIterator;
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
            QLinkedList< DocumentViewport >::const_iterator endIt = d->viewportIterator;
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

        // 3. Save DOM to XML file
        QString xml = doc.toString();
        QTextStream os( &infoFile );
        os << xml;
    }
    infoFile.close();
}

void Document::slotTimedMemoryCheck()
{
    // [MEM] clean memory (for 'free mem dependant' profiles only)
    if ( Settings::memoryLevel() != Settings::EnumMemoryLevel::Low &&
         d->allocatedPixmapsTotalMemory > 1024*1024 )
        cleanupPixmapMemory();
}

void Document::slotRotation( int rotation )
{
    // tell the pages to rotate
    QVector< Okular::Page * >::const_iterator pIt = pages_vector.begin();
    QVector< Okular::Page * >::const_iterator pEnd = pages_vector.end();
    for ( ; pIt != pEnd; ++pIt )
        (*pIt)->rotateAt( rotation );
    // clear 'memory allocation' descriptors
    QLinkedList< AllocatedPixmap * >::const_iterator aIt = d->allocatedPixmapsFifo.begin();
    QLinkedList< AllocatedPixmap * >::const_iterator aEnd = d->allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    d->allocatedPixmapsFifo.clear();
    d->allocatedPixmapsTotalMemory = 0;
    // notify the generator that the current rotation has changed
    generator->rotationChanged( rotation, d->rotation );
    // set the new rotation
    d->rotation = rotation;

    foreachObserver( notifySetup( pages_vector, true ) );
    foreachObserver( notifyContentsCleared (DocumentObserver::Pixmap | DocumentObserver::Highlights | DocumentObserver::Annotations));
    kDebug() << "Rotated: " << rotation << endl;

/*
    if ( generator->supportsRotation() )
    {
        // tell the pages to rotate
        QVector< Okular::Page * >::const_iterator pIt = pages_vector.begin();
        QVector< Okular::Page * >::const_iterator pEnd = pages_vector.end();
        for ( ; pIt != pEnd; ++pIt )
            (*pIt)->rotateAt( rotation );
        // clear 'memory allocation' descriptors
        QLinkedList< AllocatedPixmap * >::const_iterator aIt = d->allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::const_iterator aEnd = d->allocatedPixmapsFifo.end();
        for ( ; aIt != aEnd; ++aIt )
            delete *aIt;
        d->allocatedPixmapsFifo.clear();
        d->allocatedPixmapsTotalMemory = 0;
        // notify the generator that the current rotation has changed
        generator->rotationChanged( rotation, d->rotation );
        // set the new rotation
        d->rotation = rotation;

        foreachObserver( notifySetup( pages_vector, true ) );
        foreachObserver( notifyContentsCleared (DocumentObserver::Pixmap | DocumentObserver::Highlights | DocumentObserver::Annotations));
//         foreachObserver( notifyViewportChanged( false ));
//         foreachObserver( notifyPageChanged( ) );
        kDebug() << "Rotated: " << rotation << endl;
    }
*/
}

void Document::slotPaperSizes( int newsize )
{
    if (generator->supportsPaperSizes())
    {
        generator->setPaperSize(pages_vector,newsize);
        foreachObserver( notifySetup( pages_vector, true ) );
        kDebug() << "PaperSize no: " << newsize << endl;
    }
}

void Document::rotationFinished( int page )
{
    foreachObserver( notifyPageChanged( page, DocumentObserver::Pixmap | DocumentObserver::Annotations ) );
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

DocumentSynopsis::DocumentSynopsis( const QDomDocument &document )
  : QDomDocument( document )
{
}

/** DocumentFonts **/

DocumentFonts::DocumentFonts()
  : QDomDocument( "DocumentFonts" )
{
    // void implementation, only subclassed for naming
}

/** EmbeddedFile **/

EmbeddedFile::EmbeddedFile()
{
}

EmbeddedFile::~EmbeddedFile()
{
}

#include "document.moc"
