/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *   Copyright (C) 2004-2005 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "document.h"

// qt/kde/system includes
#include <QtCore/QtAlgorithms>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtGui/QApplication>

#include <kaboutdata.h>
#include <kauthorized.h>
#include <kcomponentdata.h>
#include <kconfigdialog.h>
#include <kdebug.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <krun.h>
#include <kservicetypetrader.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <ktoolinvocation.h>

// local includes
#include "action.h"
#include "audioplayer.h"
#include "audioplayer_p.h"
#include "bookmarkmanager.h"
#include "chooseenginedialog_p.h"
#include "generator.h"
#include "generator_p.h"
#include "interfaces/configinterface.h"
#include "interfaces/guiinterface.h"
#include "interfaces/printinterface.h"
#include "observer.h"
#include "page.h"
#include "page_p.h"
#include "pagecontroller_p.h"
#include "settings.h"
#include "sourcereference.h"

#include <config-okular.h>

using namespace Okular;

static int OkularDebug = 4650;

struct AllocatedPixmap
{
    // owner of the page
    int id;
    int page;
    qulonglong memory;
    // public constructor: initialize data
    AllocatedPixmap( int i, int p, qulonglong m ) : id( i ), page( p ), memory( m ) {}
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
    Qt::CaseSensitivity cachedCaseSensitivity;
    bool cachedViewportMove;
    bool cachedNoDialogs;
    QColor cachedColor;
};

struct GeneratorInfo
{
    GeneratorInfo()
        : generator( 0 ), library( 0 ), hasConfig( false )
    {}

    Generator * generator;
    KLibrary * library;
    QString catalogName;
    bool hasConfig;
};

#define foreachObserver( cmd ) {\
    QMap< int, DocumentObserver * >::const_iterator it=d->m_observers.begin(), end=d->m_observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

#define foreachObserverD( cmd ) {\
    QMap< int, DocumentObserver * >::const_iterator it = m_observers.begin(), end = m_observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

/***** Document ******/

class Okular::DocumentPrivate
{
    public:
        DocumentPrivate( Document *parent )
          : m_parent( parent ),
            m_lastSearchID( -1 ),
            m_tempFile( 0 ),
            m_docSize( -1 ),
            m_allocatedPixmapsTotalMemory( 0 ),
            m_warnedOutOfMemory( false ),
            m_rotation( Rotation0 ),
            m_exportCached( false ),
            m_bookmarkManager( 0 ),
            m_memCheckTimer( 0 ),
            m_saveBookmarksTimer( 0 ),
            m_generator( 0 ),
            m_generatorsLoaded( false ),
            m_fontsCached( false )
        {
        }

        // private methods
        QString pagesSizeString() const;
        QString localizedSize(const QSizeF &size) const;
        void cleanupPixmapMemory( qulonglong bytesOffset = 0 );
        qulonglong getTotalMemory();
        qulonglong getFreeMemory();
        void loadDocumentInfo();
        QString giveAbsolutePath( const QString & fileName ) const;
        bool openRelativeFile( const QString & fileName );
        Generator * loadGeneratorLibrary( const QString& name, const QString& libname );
        void loadAllGeneratorLibraries();
        void loadServiceList( const KService::List& offers );
        void unloadGenerator( const GeneratorInfo& info );
        void cacheExportFormats();

        // private slots
        void saveDocumentInfo() const;
        void slotTimedMemoryCheck();
        void sendGeneratorRequest();
        void rotationFinished( int page );
        void fontReadingProgress( int page );
        void fontReadingGotFont( const Okular::FontInfo& font );
        void slotGeneratorConfigChanged( const QString& );

        // member variables
        Document *m_parent;

        // find descriptors, mapped by ID (we handle multiple searches)
        QMap< int, RunningSearch * > m_searches;
        int m_lastSearchID;

        // needed because for remote documents docFileName is a local file and
        // we want the remote url when the document refers to relativeNames
        KUrl m_url;

        // cached stuff
        QString m_docFileName;
        QString m_xmlFileName;
        KTemporaryFile *m_tempFile;
        int m_docSize;

        // viewport stuff
        QLinkedList< DocumentViewport > m_viewportHistory;
        QLinkedList< DocumentViewport >::iterator m_viewportIterator;
        DocumentViewport m_nextDocumentViewport; // see Link::Goto for an explanation

        // observers / requests / allocator stuff
        QMap< int, DocumentObserver * > m_observers;
        QLinkedList< PixmapRequest * > m_pixmapRequestsStack;
        QMutex m_pixmapRequestsMutex;
        QLinkedList< AllocatedPixmap * > m_allocatedPixmapsFifo;
        qulonglong m_allocatedPixmapsTotalMemory;
        bool m_warnedOutOfMemory;

        // the rotation applied to the document
        Rotation m_rotation;

        // the current size of the pages (if available), and the cache of the
        // available page sizes
        PageSize m_pageSize;
        PageSize::List m_pageSizes;

        // cache of the export formats
        bool m_exportCached;
        ExportFormat::List m_exportFormats;
        ExportFormat m_exportToText;

        // our bookmark manager
        BookmarkManager *m_bookmarkManager;

        // timers (memory checking / info saver)
        QTimer *m_memCheckTimer;
        QTimer *m_saveBookmarksTimer;

        QHash<QString, GeneratorInfo> m_loadedGenerators;
        Generator * m_generator;
        QString m_generatorName;
        bool m_generatorsLoaded;
        QVector< Page * > m_pagesVector;
        QVector< VisiblePageRect * > m_pageRects;

        // cache of the mimetype we support
        QStringList m_supportedMimeTypes;

        QPointer< FontExtractionThread > m_fontThread;
        bool m_fontsCached;
        FontInfo::List m_fontsCache;
};

QString DocumentPrivate::pagesSizeString() const
{
    if (m_generator)
    {
        if (m_generator->pagesSizeMetric() != Generator::None)
        {
            QSizeF size = m_parent->allPagesSize();
            if (size.isValid()) return localizedSize(size);
            else return QString();
        }
        else return QString();
    }
    else return QString();
}

QString DocumentPrivate::localizedSize(const QSizeF &size) const
{
    double inchesWidth = 0, inchesHeight = 0;
    switch (m_generator->pagesSizeMetric())
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

void DocumentPrivate::cleanupPixmapMemory( qulonglong /*sure? bytesOffset*/ )
{
    // [MEM] choose memory parameters based on configuration profile
    qulonglong clipValue = -1;
    qulonglong memoryToFree = -1;
    switch ( Settings::memoryLevel() )
    {
        case Settings::EnumMemoryLevel::Low:
            memoryToFree = m_allocatedPixmapsTotalMemory;
            break;

        case Settings::EnumMemoryLevel::Normal:
            memoryToFree = m_allocatedPixmapsTotalMemory - getTotalMemory() / 3;
            clipValue = (m_allocatedPixmapsTotalMemory - getFreeMemory()) / 2;
            break;

        case Settings::EnumMemoryLevel::Aggressive:
            clipValue = (m_allocatedPixmapsTotalMemory - getFreeMemory()) / 2;
            break;
    }

    if ( clipValue > memoryToFree )
        memoryToFree = clipValue;

    if ( memoryToFree > 0 )
    {
        // [MEM] free memory starting from older pixmaps
        int pagesFreed = 0;
        QLinkedList< AllocatedPixmap * >::iterator pIt = m_allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::iterator pEnd = m_allocatedPixmapsFifo.end();
        while ( (pIt != pEnd) && (memoryToFree > 0) )
        {
            AllocatedPixmap * p = *pIt;
            if ( m_observers.value( p->id )->canUnloadPixmap( p->page ) )
            {
                // update internal variables
                pIt = m_allocatedPixmapsFifo.erase( pIt );
                m_allocatedPixmapsTotalMemory -= p->memory;
                memoryToFree -= p->memory;
                pagesFreed++;
                // delete pixmap
                m_pagesVector.at( p->page )->deletePixmap( p->id );
                // delete allocation descriptor
                delete p;
            } else
                ++pIt;
        }
        //p--rintf("freeMemory A:[%d -%d = %d] \n", m_allocatedPixmapsFifo.count() + pagesFreed, pagesFreed, m_allocatedPixmapsFifo.count() );
    }
}

qulonglong DocumentPrivate::getTotalMemory()
{
    static qulonglong cachedValue = 0;
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
     while ( true )
    {
        QString entry = readStream.readLine();
        if ( entry.isNull() ) break;
        if ( entry.startsWith( "MemTotal:" ) )
            return (cachedValue = (1024 * entry.section( ' ', -2, -2 ).toInt()));
    }
#endif
    return (cachedValue = 134217728);
}

qulonglong DocumentPrivate::getFreeMemory()
{
    static QTime lastUpdate = QTime::currentTime();
    static qulonglong cachedValue = 0;

    if ( lastUpdate.secsTo( QTime::currentTime() ) <= 2 )
        return cachedValue;

#ifdef __linux__
    // if /proc/meminfo doesn't exist, return MEMORY FULL
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( QIODevice::ReadOnly ) )
        return 0;

    // read /proc/meminfo and sum up the contents of 'MemFree', 'Buffers'
    // and 'Cached' fields. consider swapped memory as used memory.
    qulonglong memoryFree = 0;
    QString entry;
    QTextStream readStream( &memFile );
    while ( true )
    {
        entry = readStream.readLine();
        if ( entry.isNull() ) break;
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

void DocumentPrivate::loadDocumentInfo()
// note: load data and stores it internally (document or pages). observers
// are still uninitialized at this point so don't access them
{
    //kDebug(OkularDebug) << "Using '" << d->m_xmlFileName << "' as document info file." << endl;
    if ( m_xmlFileName.isEmpty() )
        return;

    QFile infoFile( m_xmlFileName );
    if ( !infoFile.exists() || !infoFile.open( QIODevice::ReadOnly ) )
        return;

    // Load DOM from XML file
    QDomDocument doc( "documentInfo" );
    if ( !doc.setContent( &infoFile ) )
    {
        kDebug(OkularDebug) << "Can't load XML pair! Check for broken xml." << endl;
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
                    if ( ok && pageNumber >= 0 && pageNumber < (int)m_pagesVector.count() )
                        m_pagesVector[ pageNumber ]->d->restoreLocalContents( pageElement );
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

                // restore viewports history
                if ( infoElement.tagName() == "history" )
                {
                    // clear history
                    m_viewportHistory.clear();
                    // append old viewports
                    QDomNode historyNode = infoNode.firstChild();
                    while ( historyNode.isElement() )
                    {
                        QDomElement historyElement = historyNode.toElement();
                        if ( historyElement.hasAttribute( "viewport" ) )
                        {
                            QString vpString = historyElement.attribute( "viewport" );
                            m_viewportIterator = m_viewportHistory.insert( m_viewportHistory.end(),
                                    DocumentViewport( vpString ) );
                        }
                        historyNode = historyNode.nextSibling();
                    }
                    // consistancy check
                    if ( m_viewportHistory.isEmpty() )
                        m_viewportIterator = m_viewportHistory.insert( m_viewportHistory.end(), DocumentViewport() );
                }
                infoNode = infoNode.nextSibling();
            }
        }

        topLevelNode = topLevelNode.nextSibling();
    } // </documentInfo>
}

QString DocumentPrivate::giveAbsolutePath( const QString & fileName ) const
{
    if ( !m_url.isValid() )
        return QString();

    if ( !QDir::isRelativePath( fileName ) )
        return fileName;

    return m_url.upUrl().url() + fileName;
}

bool DocumentPrivate::openRelativeFile( const QString & fileName )
{
    QString absFileName = giveAbsolutePath( fileName );
    if ( absFileName.isEmpty() )
        return false;

    kDebug(OkularDebug) << "openDocument: '" << absFileName << "'" << endl;

    emit m_parent->openUrl( absFileName );
    return true;
}

Generator * DocumentPrivate::loadGeneratorLibrary( const QString& name, const QString& libname )
{
    KLibrary *lib = KLibLoader::self()->library( QFile::encodeName( libname ), QLibrary::ExportExternalSymbolsHint );
    if ( !lib )
    {
        kWarning() << "Could not load '" << libname << "' library." << endl;
        kWarning() << KLibLoader::self()->lastErrorMessage() << endl;
        emit m_parent->error( i18n( "Could not load the necessary plugin to view the document" ), -1 );
        return 0;
    }

    Generator* (*create_plugin)() = ( Generator* (*)() ) lib->resolveFunction( "create_plugin" );
    if ( !create_plugin )
    {
        kWarning(OkularDebug) << "Broken generator " << libname << ": missing create_plugin()!" << endl;
        return 0;
    }
    Generator * generator = create_plugin();
    if ( !generator )
    {
        kWarning() << "Broken generator " << libname << "!" << endl;
        return 0;
    }
    GeneratorInfo info;
    info.generator = generator;
    info.library = lib;
    if ( generator->componentData() && generator->componentData()->aboutData() )
        info.catalogName = generator->componentData()->aboutData()->catalogName();
    m_loadedGenerators.insert( name, info );
    return generator;
}

void DocumentPrivate::loadAllGeneratorLibraries()
{
    if ( m_generatorsLoaded )
        return;

    m_generatorsLoaded = true;

    QString constraint("([X-KDE-Priority] > 0) and (exist Library)") ;
    KService::List offers = KServiceTypeTrader::self()->query( "okular/Generator", constraint );
    loadServiceList( offers );
}

void DocumentPrivate::loadServiceList( const KService::List& offers )
{
    int count = offers.count();
    if ( count <= 0 )
        return;

    for ( int i = 0; i < count; ++i )
    {
        QString propName = offers.at(i)->name();
        // don't load already loaded generators
        QHash< QString, GeneratorInfo >::const_iterator genIt = m_loadedGenerators.constFind( propName );
        if ( genIt != m_loadedGenerators.end() )
            continue;

        Generator * g = loadGeneratorLibrary( propName, offers.at(i)->library() );
        (void)g;
    }
}

void DocumentPrivate::unloadGenerator( const GeneratorInfo& info )
{
    delete info.generator;
    info.library->unload();
}

void DocumentPrivate::cacheExportFormats()
{
    if ( m_exportCached )
        return;

    const ExportFormat::List formats = m_generator->exportFormats();
    for ( int i = 0; i < formats.count(); ++i )
    {
        if ( formats.at( i ).mimeType()->name() == QLatin1String( "text/plain" ) )
            m_exportToText = formats.at( i );
        else
            m_exportFormats.append( formats.at( i ) );
    }

    m_exportCached = true;
}

void DocumentPrivate::saveDocumentInfo() const
{
    if ( m_docFileName.isEmpty() )
        return;

    QFile infoFile( m_xmlFileName );
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
        QVector< Page * >::const_iterator pIt = m_pagesVector.begin(), pEnd = m_pagesVector.end();
        for ( ; pIt != pEnd; ++pIt )
            (*pIt)->d->saveLocalContents( pageList, doc );

        // 2.2. Save document info (current viewport, history, ... ) to DOM
        QDomElement generalInfo = doc.createElement( "generalInfo" );
        root.appendChild( generalInfo );
        // <general info><history> ... </history> save history up to 10 viewports
        QLinkedList< DocumentViewport >::const_iterator backIterator = m_viewportIterator;
        if ( backIterator != m_viewportHistory.end() )
        {
            // go back up to 10 steps from the current viewportIterator
            int backSteps = 10;
            while ( backSteps-- && backIterator != m_viewportHistory.begin() )
                --backIterator;

            // create history root node
            QDomElement historyNode = doc.createElement( "history" );
            generalInfo.appendChild( historyNode );

            // add old[backIterator] and present[viewportIterator] items
            QLinkedList< DocumentViewport >::const_iterator endIt = m_viewportIterator;
            ++endIt;
            while ( backIterator != endIt )
            {
                QString name = (backIterator == m_viewportIterator) ? "current" : "oldPage";
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

void DocumentPrivate::slotTimedMemoryCheck()
{
    // [MEM] clean memory (for 'free mem dependant' profiles only)
    if ( Settings::memoryLevel() != Settings::EnumMemoryLevel::Low &&
         m_allocatedPixmapsTotalMemory > 1024*1024 )
        cleanupPixmapMemory();
}

void DocumentPrivate::sendGeneratorRequest()
{
    // find a request
    PixmapRequest * request = 0;
    m_pixmapRequestsMutex.lock();
    while ( !m_pixmapRequestsStack.isEmpty() && !request )
    {
        PixmapRequest * r = m_pixmapRequestsStack.last();
        if (!r)
            m_pixmapRequestsStack.pop_back();

        // request only if page isn't already present or request has invalid id
        else if ( r->page()->hasPixmap( r->id(), r->width(), r->height() ) || r->id() <= 0 || r->id() >= MAX_OBSERVER_ID)
        {
            m_pixmapRequestsStack.pop_back();
            delete r;
        }
        else if ( (long)r->width() * (long)r->height() > 20000000L )
        {
            m_pixmapRequestsStack.pop_back();
            if ( !m_warnedOutOfMemory )
            {
                kWarning() << "Running out of memory on page " << r->pageNumber()
                    << " (" << r->width() << "x" << r->height() << " px);" << endl;
                kWarning() << "this message will be reported only once." << endl;
                m_warnedOutOfMemory = true;
            }
            delete r;
        }
        else
            request = r;
    }

    // if no request found (or already generated), return
    if ( !request )
    {
        m_pixmapRequestsMutex.unlock();
        return;
    }

    // [MEM] preventive memory freeing
    qulonglong pixmapBytes = 4 * request->width() * request->height();
    if ( pixmapBytes > (1024 * 1024) )
        cleanupPixmapMemory( pixmapBytes );

    // submit the request to the generator
    if ( m_generator->canGeneratePixmap() )
    {
        kDebug(OkularDebug) << "sending request id=" << request->id() << " " <<request->width() << "x" << request->height() << "@" << request->pageNumber() << " async == " << request->asynchronous() << endl;
        m_pixmapRequestsStack.removeAll ( request );

        if ( (int)m_rotation % 2 )
            request->swap();

        // we always have to unlock _before_ the generatePixmap() because
        // a sync generation would end with requestDone() -> deadlock, and
        // we can not really know if the generator can do async requests
        m_pixmapRequestsMutex.unlock();
        m_generator->generatePixmap( request );
    }
    else
    {
        m_pixmapRequestsMutex.unlock();
        // pino (7/4/2006): set the polling interval from 10 to 30
        QTimer::singleShot( 30, m_parent, SLOT(sendGeneratorRequest()) );
    }
}

void DocumentPrivate::rotationFinished( int page )
{
    QMap< int, DocumentObserver * >::const_iterator it = m_observers.begin(), end = m_observers.end();
    for ( ; it != end ; ++ it ) {
        (*it)->notifyPageChanged( page, DocumentObserver::Pixmap | DocumentObserver::Annotations );
    }
}

void DocumentPrivate::fontReadingProgress( int page )
{
    emit m_parent->fontReadingProgress( page );

    if ( page == (int)m_parent->pages() )
    {
        emit m_parent->fontReadingEnded();
        m_fontThread = 0;
        m_fontsCached = true;
    }
}

void DocumentPrivate::fontReadingGotFont( const Okular::FontInfo& font )
{
    // TODO try to avoid duplicate fonts
    m_fontsCache.append( font );

    emit m_parent->gotFont( font );
}

void DocumentPrivate::slotGeneratorConfigChanged( const QString& )
{
    if ( !m_generator )
        return;

    // reparse generator config and if something changed clear Pages
    bool configchanged = false;
    QHash< QString, GeneratorInfo >::const_iterator it = m_loadedGenerators.constBegin(), itEnd = m_loadedGenerators.constEnd();
    for ( ; it != itEnd; ++it )
    {
        Okular::ConfigInterface * iface = qobject_cast< Okular::ConfigInterface * >( it.value().generator );
        if ( iface )
        {
            bool it_changed = iface->reparseConfig();
            if ( it_changed && ( m_generator == it.value().generator ) )
                configchanged = true;
        }
    }
    if ( configchanged )
    {
        // invalidate pixmaps
        QVector<Page*>::const_iterator it = m_pagesVector.begin(), end = m_pagesVector.end();
        for ( ; it != end; ++it ) {
            (*it)->deletePixmaps();
        }

        // [MEM] remove allocation descriptors
        QLinkedList< AllocatedPixmap * >::const_iterator aIt = m_allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::const_iterator aEnd = m_allocatedPixmapsFifo.end();
        for ( ; aIt != aEnd; ++aIt )
            delete *aIt;
        m_allocatedPixmapsFifo.clear();
        m_allocatedPixmapsTotalMemory = 0;

        // send reload signals to observers
        foreachObserverD( notifyContentsCleared( DocumentObserver::Pixmap ) );
    }

    // free memory if in 'low' profile
    if ( Settings::memoryLevel() == Settings::EnumMemoryLevel::Low &&
         !m_allocatedPixmapsFifo.isEmpty() && !m_pagesVector.isEmpty() )
        cleanupPixmapMemory();
}


Document::Document( QWidget *widget )
    : QObject( widget ), d( new DocumentPrivate( this ) )
{
    d->m_bookmarkManager = new BookmarkManager( this );

    connect( PageController::self(), SIGNAL( rotationFinished( int ) ),
             this, SLOT( rotationFinished( int ) ) );

    qRegisterMetaType<Okular::FontInfo>();
}

Document::~Document()
{
    // stop any audio playback
    AudioPlayer::instance()->stopPlaybacks();

    // delete generator, pages, and related stuff
    closeDocument();

    // delete the bookmark manager
    delete d->m_bookmarkManager;

    // delete the loaded generators
    QHash< QString, GeneratorInfo >::const_iterator it = d->m_loadedGenerators.constBegin(), itEnd = d->m_loadedGenerators.constEnd();
    for ( ; it != itEnd; ++it )
        d->unloadGenerator( it.value() );
    d->m_loadedGenerators.clear();

    // delete the private structure
    delete d;
}

static bool kserviceMoreThan( const KService::Ptr &s1, const KService::Ptr &s2 )
{
    return s1->property( "X-KDE-Priority" ).toInt() > s2->property( "X-KDE-Priority" ).toInt();
}

bool Document::openDocument( const QString & docFile, const KUrl& url, const KMimeType::Ptr &_mime )
{
    KMimeType::Ptr mime = _mime;
    QByteArray filedata;
    int document_size = -1;
    bool isstdin = url.fileName( KUrl::ObeyTrailingSlash ) == QLatin1String( "-" );
    if ( !isstdin )
    {
        if ( mime.count() <= 0 )
            return false;

        // docFile is always local so we can use QFile on it
        QFile fileReadTest( docFile );
        if ( !fileReadTest.open( QIODevice::ReadOnly ) )
        {
            d->m_docFileName.clear();
            return false;
        }
        // determine the related "xml document-info" filename
        d->m_url = url;
        d->m_docFileName = docFile;
        QString fn = docFile.contains('/') ? docFile.section('/', -1, -1) : docFile;
        document_size = fileReadTest.size();
        fn = QString::number( document_size ) + '.' + fn + ".xml";
        fileReadTest.close();
        QString newokular = "okular/docdata/" + fn;
        QString newokularfile = KStandardDirs::locateLocal( "data", newokular );
        QString oldkpdf = "kpdf/" + fn;
        QString oldkpdffile = KStandardDirs::locateLocal( "data", oldkpdf );
        if ( QFile::exists( oldkpdffile ) && !QFile::exists( newokularfile ) )
        {
            // ### copy or move?
            if ( !QFile::copy( oldkpdffile, newokularfile ) )
                return false;
        }
        d->m_xmlFileName = newokularfile;
    }
    else
    {
        QFile qstdin;
        qstdin.open( stdin, QIODevice::ReadOnly );
        filedata = qstdin.readAll();
        mime = KMimeType::findByContent( filedata );
        if ( !mime || mime->name() == QLatin1String( "application/octet-stream" ) )
            return false;
        document_size = filedata.size();
    }

    // 0. load Generator
    // request only valid non-disabled plugins suitable for the mimetype
    QString constraint("([X-KDE-Priority] > 0) and (exist Library)") ;
    KService::List offers = KMimeTypeTrader::self()->query(mime->name(),"okular/Generator",constraint);
    if (offers.isEmpty())
    {
        emit error( i18n( "Can not find a plugin which is able to handle the passed document." ), -1 );
        kWarning() << "No plugin for mimetype '" << mime->name() << "'." << endl;
        return false;
    }
    int hRank=0;
    // best ranked offer search
    int offercount = offers.count();
    if ( offercount > 1 )
    {
        // sort the offers: the offers with an higher priority come before
        qStableSort( offers.begin(), offers.end(), kserviceMoreThan );

        if ( Settings::chooseGenerators() )
        {
            QStringList list;
            for ( int i = 0; i < offercount; ++i )
            {
                list << offers.at(i)->name();
            }

            ChooseEngineDialog choose( list, mime, widget() );

            if ( choose.exec() == QDialog::Rejected )
                return false;

            hRank = choose.selectedGenerator();
        }
    }

    QString propName = offers.at(hRank)->name();
    QHash< QString, GeneratorInfo >::const_iterator genIt = d->m_loadedGenerators.constFind( propName );
    QString catalogName;
    if ( genIt != d->m_loadedGenerators.constEnd() )
    {
        d->m_generator = genIt.value().generator;
        catalogName = genIt.value().catalogName;
    }
    else
    {
        d->m_generator = d->loadGeneratorLibrary( propName, offers.at(hRank)->library() );
        if ( !d->m_generator )
            return false;
        genIt = d->m_loadedGenerators.constFind( propName );
        Q_ASSERT( genIt != d->m_loadedGenerators.constEnd() );
        catalogName = genIt.value().catalogName;
    }
    Q_ASSERT_X( d->m_generator, "Document::load()", "null generator?!" );

    if ( !catalogName.isEmpty() )
        KGlobal::locale()->insertCatalog( catalogName );

    d->m_generator->d->m_document = this;

    // connect error reporting signals
    connect( d->m_generator, SIGNAL( error( const QString&, int ) ), this, SIGNAL( error( const QString&, int ) ) );
    connect( d->m_generator, SIGNAL( warning( const QString&, int ) ), this, SIGNAL( warning( const QString&, int ) ) );
    connect( d->m_generator, SIGNAL( notice( const QString&, int ) ), this, SIGNAL( notice( const QString&, int ) ) );

    // 1. load Document (and set busy cursor while loading)
    QApplication::setOverrideCursor( Qt::WaitCursor );
    bool openOk = false;
    if ( !isstdin )
    {
        openOk = d->m_generator->loadDocument( docFile, d->m_pagesVector );
    }
    else if ( !filedata.isEmpty() )
    {
        if ( d->m_generator->hasFeature( Generator::ReadRawData ) )
        {
            openOk = d->m_generator->loadDocumentFromData( filedata, d->m_pagesVector );
        }
        else
        {
            d->m_tempFile = new KTemporaryFile();
            if ( !d->m_tempFile->open() )
            {
                delete d->m_tempFile;
                d->m_tempFile = 0;
            }
            else
            {
                d->m_tempFile->write( filedata );
                QString tmpFileName = d->m_tempFile->fileName();
                d->m_tempFile->close();
                openOk = d->m_generator->loadDocument( tmpFileName, d->m_pagesVector );
            }
        }
    }

    QApplication::restoreOverrideCursor();
    if ( !openOk || d->m_pagesVector.size() <= 0 )
    {
        if ( !catalogName.isEmpty() )
            KGlobal::locale()->removeCatalog( catalogName );

        d->m_generator = 0;
        return openOk;
    }

    d->m_generatorName = propName;

    // 2. load Additional Data (our bookmarks and metadata) about the document
    d->loadDocumentInfo();
    d->m_bookmarkManager->setUrl( d->m_url );

    // 3. setup observers inernal lists and data
    foreachObserver( notifySetup( d->m_pagesVector, true ) );

    // 4. set initial page (restoring the page saved in xml if loaded)
    DocumentViewport loadedViewport = (*d->m_viewportIterator);
    if ( loadedViewport.isValid() )
    {
        (*d->m_viewportIterator) = DocumentViewport();
        if ( loadedViewport.pageNumber >= (int)d->m_pagesVector.size() )
            loadedViewport.pageNumber = d->m_pagesVector.size() - 1;
    }
    else
        loadedViewport.pageNumber = 0;
    setViewport( loadedViewport );

    // start bookmark saver timer
    if ( !d->m_saveBookmarksTimer )
    {
        d->m_saveBookmarksTimer = new QTimer( this );
        connect( d->m_saveBookmarksTimer, SIGNAL( timeout() ), this, SLOT( saveDocumentInfo() ) );
    }
    d->m_saveBookmarksTimer->start( 5 * 60 * 1000 );

    // start memory check timer
    if ( !d->m_memCheckTimer )
    {
        d->m_memCheckTimer = new QTimer( this );
        connect( d->m_memCheckTimer, SIGNAL( timeout() ), this, SLOT( slotTimedMemoryCheck() ) );
    }
    d->m_memCheckTimer->start( 2000 );

    if (d->m_nextDocumentViewport.isValid())
    {
        setViewport(d->m_nextDocumentViewport);
        d->m_nextDocumentViewport = DocumentViewport();
    }

    AudioPlayer::instance()->d->m_currentDocument = isstdin ? KUrl() : d->m_url;
    d->m_docSize = document_size;

    return true;
}


QString Document::xmlFile() const
{
    if ( d->m_generator )
    {
        Okular::GuiInterface * iface = qobject_cast< Okular::GuiInterface * >( d->m_generator );
        return iface ? iface->xmlFile() : QString();
    }
    else
        return QString();
}

void Document::setupGui( KActionCollection *ac, QToolBox *tBox )
{
    if ( d->m_generator && ac && tBox )
    {
        Okular::GuiInterface * iface = qobject_cast< Okular::GuiInterface * >( d->m_generator );
        if ( iface )
            iface->setupGui( ac, tBox );
    }
}

void Document::closeDocument()
{
    if ( d->m_fontThread )
    {
        disconnect( d->m_fontThread, 0, this, 0 );
        d->m_fontThread->stopExtraction();
        d->m_fontThread->wait();
        d->m_fontThread = 0;
    }

    // close the current document and save document info if a document is still opened
    if ( d->m_generator && d->m_pagesVector.size() > 0 )
    {
        d->saveDocumentInfo();
        d->m_generator->closeDocument();
    }

    // stop timers
    if ( d->m_memCheckTimer )
        d->m_memCheckTimer->stop();
    if ( d->m_saveBookmarksTimer )
        d->m_saveBookmarksTimer->stop();

    if ( d->m_generator )
    {
        Okular::GuiInterface * iface = qobject_cast< Okular::GuiInterface * >( d->m_generator );
        if ( iface )
            iface->freeGui();
        // disconnect the generator from this document ...
        d->m_generator->d->m_document = 0;
        // .. and this document from the generator signals
        disconnect( d->m_generator, 0, this, 0 );

        QHash< QString, GeneratorInfo >::const_iterator genIt = d->m_loadedGenerators.constFind( d->m_generatorName );
        Q_ASSERT( genIt != d->m_loadedGenerators.constEnd() );
        if ( !genIt.value().catalogName.isEmpty() )
            KGlobal::locale()->removeCatalog( genIt.value().catalogName );
    }
    d->m_generator = 0;
    d->m_generatorName = QString();
    d->m_url = KUrl();
    d->m_docFileName = QString();
    d->m_xmlFileName = QString();
    delete d->m_tempFile;
    d->m_tempFile = 0;
    d->m_docSize = -1;
    d->m_exportCached = false;
    d->m_exportFormats.clear();
    d->m_exportToText = ExportFormat();
    d->m_fontsCached = false;
    d->m_fontsCache.clear();
    d->m_rotation = Rotation0;
    // remove requests left in queue
    d->m_pixmapRequestsMutex.lock();
    QLinkedList< PixmapRequest * >::const_iterator sIt = d->m_pixmapRequestsStack.begin();
    QLinkedList< PixmapRequest * >::const_iterator sEnd = d->m_pixmapRequestsStack.end();
    for ( ; sIt != sEnd; ++sIt )
        delete *sIt;
    d->m_pixmapRequestsStack.clear();
    d->m_pixmapRequestsMutex.unlock();

    // send an empty list to observers (to free their data)
    foreachObserver( notifySetup( QVector< Page * >(), true ) );

    // delete pages and clear 'd->m_pagesVector' container
    QVector< Page * >::const_iterator pIt = d->m_pagesVector.begin();
    QVector< Page * >::const_iterator pEnd = d->m_pagesVector.end();
    for ( ; pIt != pEnd; ++pIt )
        delete *pIt;
    d->m_pagesVector.clear();

    // clear 'memory allocation' descriptors
    QLinkedList< AllocatedPixmap * >::const_iterator aIt = d->m_allocatedPixmapsFifo.begin();
    QLinkedList< AllocatedPixmap * >::const_iterator aEnd = d->m_allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    d->m_allocatedPixmapsFifo.clear();

    // clear 'running searches' descriptors
    QMap< int, RunningSearch * >::const_iterator rIt = d->m_searches.begin();
    QMap< int, RunningSearch * >::const_iterator rEnd = d->m_searches.end();
    for ( ; rIt != rEnd; ++rIt )
        delete *rIt;
    d->m_searches.clear();

    // clear the visible areas and notify the observers
    QVector< VisiblePageRect * >::const_iterator vIt = d->m_pageRects.begin();
    QVector< VisiblePageRect * >::const_iterator vEnd = d->m_pageRects.end();
    for ( ; vIt != vEnd; ++vIt )
        delete *vIt;
    d->m_pageRects.clear();
    foreachObserver( notifyVisibleRectsChanged() );

    // reset internal variables

    d->m_viewportHistory.clear();
    d->m_viewportHistory.append( DocumentViewport() );
    d->m_viewportIterator = d->m_viewportHistory.begin();
    d->m_allocatedPixmapsTotalMemory = 0;
    d->m_pageSize = PageSize();
    d->m_pageSizes.clear();
    AudioPlayer::instance()->d->m_currentDocument = KUrl();
}

void Document::addObserver( DocumentObserver * pObserver )
{
    // keep the pointer to the observer in a map
    d->m_observers.insert( pObserver->observerId(), pObserver );

    // if the observer is added while a document is already opened, tell it
    if ( !d->m_pagesVector.isEmpty() )
    {
        pObserver->notifySetup( d->m_pagesVector, true );
        pObserver->notifyViewportChanged( false /*disables smoothMove*/ );
    }
}

void Document::removeObserver( DocumentObserver * pObserver )
{
    // remove observer from the map. it won't receive notifications anymore
    if ( d->m_observers.contains( pObserver->observerId() ) )
    {
        // free observer's pixmap data
        int observerId = pObserver->observerId();
        QVector<Page*>::const_iterator it = d->m_pagesVector.begin(), end = d->m_pagesVector.end();
        for ( ; it != end; ++it )
            (*it)->deletePixmap( observerId );

        // [MEM] free observer's allocation descriptors
        QLinkedList< AllocatedPixmap * >::iterator aIt = d->m_allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::iterator aEnd = d->m_allocatedPixmapsFifo.end();
        while ( aIt != aEnd )
        {
            AllocatedPixmap * p = *aIt;
            if ( p->id == observerId )
            {
                aIt = d->m_allocatedPixmapsFifo.erase( aIt );
                delete p;
            }
            else
                ++aIt;
        }

        // delete observer entry from the map
        d->m_observers.remove( observerId );
    }
}

void Document::reparseConfig()
{
    // reparse generator config and if something changed clear Pages
    bool configchanged = false;
    if ( d->m_generator )
    {
        Okular::ConfigInterface * iface = qobject_cast< Okular::ConfigInterface * >( d->m_generator );
        if ( iface )
            configchanged = iface->reparseConfig();
    }
    if ( configchanged )
    {
        // invalidate pixmaps
        QVector<Page*>::const_iterator it = d->m_pagesVector.begin(), end = d->m_pagesVector.end();
        for ( ; it != end; ++it ) {
            (*it)->deletePixmaps();
        }

        // [MEM] remove allocation descriptors
        QLinkedList< AllocatedPixmap * >::const_iterator aIt = d->m_allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::const_iterator aEnd = d->m_allocatedPixmapsFifo.end();
        for ( ; aIt != aEnd; ++aIt )
            delete *aIt;
        d->m_allocatedPixmapsFifo.clear();
        d->m_allocatedPixmapsTotalMemory = 0;

        // send reload signals to observers
        foreachObserver( notifyContentsCleared( DocumentObserver::Pixmap ) );
    }

    // free memory if in 'low' profile
    if ( Settings::memoryLevel() == Settings::EnumMemoryLevel::Low &&
         !d->m_allocatedPixmapsFifo.isEmpty() && !d->m_pagesVector.isEmpty() )
        d->cleanupPixmapMemory();
}


QWidget *Document::widget() const
{
    return parent() ? static_cast< QWidget * >( parent() ) : 0;
}

bool Document::isOpened() const
{
    return d->m_generator;
}

bool Document::canConfigurePrinter( ) const
{
    if ( d->m_generator )
    {
        Okular::PrintInterface * iface = qobject_cast< Okular::PrintInterface * >( d->m_generator );
        return iface ? true : false;
    }
    else
        return 0;
}

const DocumentInfo * Document::documentInfo() const
{
    if ( d->m_generator )
    {
        const DocumentInfo *infoConst = d->m_generator->generateDocumentInfo();
        if ( !infoConst )
            return 0;

        DocumentInfo *info = const_cast< DocumentInfo * >( infoConst );
        QString pagesSize = d->pagesSizeString();
        if ( d->m_docSize != -1 )
        {
            QString sizeString = KGlobal::locale()->formatByteSize( d->m_docSize );
            info->set( "documentSize", sizeString, i18n( "File Size" ) );
        }
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
    return d->m_generator ? d->m_generator->generateDocumentSynopsis() : NULL;
}

void Document::startFontReading()
{
    if ( !d->m_generator || !d->m_generator->hasFeature( Generator::FontInfo ) || d->m_fontThread )
        return;

    if ( d->m_fontsCached )
    {
        // in case we have cached fonts, simulate a reading
        // this way the API is the same, and users no need to care about the
        // internal caching
        for ( int i = 0; i < d->m_fontsCache.count(); ++i )
        {
            emit gotFont( d->m_fontsCache.at( i ) );
            emit fontReadingProgress( i / pages() );
        }
        emit fontReadingEnded();
        return;
    }

    d->m_fontThread = new FontExtractionThread( d->m_generator, pages() );
    connect( d->m_fontThread, SIGNAL( gotFont( const Okular::FontInfo& ) ), this, SLOT( fontReadingGotFont( const Okular::FontInfo& ) ) );
    connect( d->m_fontThread, SIGNAL( progress( int ) ), this, SLOT( fontReadingProgress( int ) ) );

    d->m_fontThread->startExtraction( /*d->m_generator->hasFeature( Generator::Threaded )*/true );
}

void Document::stopFontReading()
{
    if ( !d->m_fontThread )
        return;

    disconnect( d->m_fontThread, 0, this, 0 );
    d->m_fontThread->stopExtraction();
    d->m_fontThread = 0;
    d->m_fontsCache.clear();
}

bool Document::canProvideFontInformation() const
{
    return d->m_generator ? d->m_generator->hasFeature( Generator::FontInfo ) : false;
}

const QList<EmbeddedFile*> *Document::embeddedFiles() const
{
    return d->m_generator ? d->m_generator->embeddedFiles() : NULL;
}

const Page * Document::page( int n ) const
{
    return ( n < d->m_pagesVector.count() ) ? d->m_pagesVector.at(n) : 0;
}

const DocumentViewport & Document::viewport() const
{
    return (*d->m_viewportIterator);
}

const QVector< VisiblePageRect * > & Document::visiblePageRects() const
{
    return d->m_pageRects;
}

void Document::setVisiblePageRects( const QVector< VisiblePageRect * > & visiblePageRects, int excludeId )
{
    QVector< VisiblePageRect * >::const_iterator vIt = d->m_pageRects.begin();
    QVector< VisiblePageRect * >::const_iterator vEnd = d->m_pageRects.end();
    for ( ; vIt != vEnd; ++vIt )
        delete *vIt;
    d->m_pageRects = visiblePageRects;
    // notify change to all other (different from id) observers
    QMap< int, DocumentObserver * >::const_iterator it = d->m_observers.begin(), end = d->m_observers.end();
    for ( ; it != end ; ++ it )
        if ( it.key() != excludeId )
            (*it)->notifyVisibleRectsChanged();
}

uint Document::currentPage() const
{
    return (*d->m_viewportIterator).pageNumber;
}

uint Document::pages() const
{
    return d->m_pagesVector.size();
}

KUrl Document::currentDocument() const
{
    return d->m_url;
}

bool Document::isAllowed( Permission action ) const
{
#if !OKULAR_FORCE_DRM
    if ( KAuthorized::authorize( "skip_drm" ) && !Okular::Settings::obeyDRM() )
        return true;
#endif

    return d->m_generator ? d->m_generator->isAllowed( action ) : false;
}

bool Document::supportsSearching() const
{
    return d->m_generator ? d->m_generator->hasFeature( Generator::TextExtraction ) : false;
}

bool Document::supportsPageSizes() const
{
    return d->m_generator ? d->m_generator->hasFeature( Generator::PageSizes ) : false;
}

PageSize::List Document::pageSizes() const
{
    if ( d->m_generator )
    {
        if ( d->m_pageSizes.isEmpty() )
            d->m_pageSizes = d->m_generator->pageSizes();
        return d->m_pageSizes;
    }
    return PageSize::List();
}

bool Document::canExportToText() const
{
    if ( !d->m_generator )
        return false;

    d->cacheExportFormats();
    return !d->m_exportToText.isNull();
}

bool Document::exportToText( const QString& fileName ) const
{
    if ( !d->m_generator )
        return false;

    d->cacheExportFormats();
    if ( d->m_exportToText.isNull() )
        return false;

    return d->m_generator->exportTo( fileName, d->m_exportToText );
}

ExportFormat::List Document::exportFormats() const
{
    if ( !d->m_generator )
        return ExportFormat::List();

    d->cacheExportFormats();
    return d->m_exportFormats;
}

bool Document::exportTo( const QString& fileName, const ExportFormat& format ) const
{
    return d->m_generator ? d->m_generator->exportTo( fileName, format ) : false;
}

bool Document::historyAtBegin() const
{
    return d->m_viewportIterator == d->m_viewportHistory.begin();
}

bool Document::historyAtEnd() const
{
    return d->m_viewportIterator == --(d->m_viewportHistory.end());
}

QVariant Document::metaData( const QString & key, const QVariant & option ) const
{
    return d->m_generator ? d->m_generator->metaData( key, option ) : QVariant();
}

Rotation Document::rotation() const
{
    return d->m_rotation;
}

QSizeF Document::allPagesSize() const
{
    bool allPagesSameSize = true;
    QSizeF size;
    for (int i = 0; allPagesSameSize && i < d->m_pagesVector.count(); ++i)
    {
        const Page *p = d->m_pagesVector.at(i);
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
    if (d->m_generator)
    {
        if (d->m_generator->pagesSizeMetric() != Generator::None)
        {
            const Page *p = d->m_pagesVector.at( page );
            return d->localizedSize(QSizeF(p->width(), p->height()));
        }
    }
    return QString();
}

void Document::requestPixmaps( const QLinkedList< PixmapRequest * > & requests )
{
    if ( requests.isEmpty() )
        return;

    if ( !d->m_generator )
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
    d->m_pixmapRequestsMutex.lock();
    QLinkedList< PixmapRequest * >::iterator sIt = d->m_pixmapRequestsStack.begin(), sEnd = d->m_pixmapRequestsStack.end();
    while ( sIt != sEnd )
    {
        if ( (*sIt)->id() == requesterID )
        {
            // delete request and remove it from stack
            delete *sIt;
            sIt = d->m_pixmapRequestsStack.erase( sIt );
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
        kDebug(OkularDebug) << "request id=" << request->id() << " " <<request->width() << "x" << request->height() << "@" << request->pageNumber() << endl;
        if ( d->m_pagesVector.value( request->pageNumber() ) == 0 )
        {
            // skip requests referencing an invalid page (must not happen)
            delete request;
            continue;
        }

        request->setPage( d->m_pagesVector.value( request->pageNumber() ) );

        if ( !request->asynchronous() )
            request->setPriority( 0 );

        if ( request->asynchronous() && threadingDisabled )
            request->setAsynchronous( false );

        // add request to the 'stack' at the right place
        if ( !request->priority() )
            // add priority zero requests to the top of the stack
            d->m_pixmapRequestsStack.append( request );
        else
        {
            // insert in stack sorted by priority
            sIt = d->m_pixmapRequestsStack.begin();
            sEnd = d->m_pixmapRequestsStack.end();
            while ( sIt != sEnd && (*sIt)->priority() > request->priority() )
                ++sIt;
            d->m_pixmapRequestsStack.insert( sIt, request );
        }
    }
    d->m_pixmapRequestsMutex.unlock();

    // 3. [START FIRST GENERATION] if <NO>generator is ready, start a new generation,
    // or else (if gen is running) it will be started when the new contents will
    //come from generator (in requestDone())</NO>
    // all handling of requests put into sendGeneratorRequest
    //    if ( generator->canRequestPixmap() )
        d->sendGeneratorRequest();
}

void Document::requestTextPage( uint page, enum GenerationType type )
{
    Page * kp = d->m_pagesVector[ page ];
    if ( !d->m_generator || !kp )
        return;

    // Memory management for TextPages

    d->m_generator->generateTextPage( kp, type );
}

void Document::addPageAnnotation( int page, Annotation * annotation )
{
    // find out the page to attach annotation
    Page * kp = d->m_pagesVector[ page ];
    if ( !d->m_generator || !kp )
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
    Page * kp = d->m_pagesVector[ page ];
    if ( !d->m_generator || !kp )
        return;

    kp->d->modifyAnnotation( newannotation );
    // notify observers about the change
    foreachObserver( notifyPageChanged( page, DocumentObserver::Annotations ) );
}


void Document::removePageAnnotation( int page, Annotation * annotation )
{
    // find out the page
    Page * kp = d->m_pagesVector[ page ];
    if ( !d->m_generator || !kp )
        return;

    // try to remove the annotation
    if ( kp->removeAnnotation( annotation ) )
    {
        // in case of success, notify observers about the change
        foreachObserver( notifyPageChanged( page, DocumentObserver::Annotations ) );
    }
}

void Document::removePageAnnotations( int page, const QList< Annotation * > &annotations )
{
    // find out the page
    Page * kp = d->m_pagesVector[ page ];
    if ( !d->m_generator || !kp )
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
    Page * kp = d->m_pagesVector[ page ];
    if ( !d->m_generator || !kp )
        return;

    // add or remove the selection basing whether rect is null or not
    if ( rect )
        kp->d->setTextSelections( rect, color );
    else
        kp->deleteTextSelections();

    // notify observers about the change
    foreachObserver( notifyPageChanged( page, DocumentObserver::TextSelection ) );
}

/* REFERENCE IMPLEMENTATION: better calling setViewport from other code
void Document::setNextPage()
{
    // advance page and set viewport on observers
    if ( (*d->m_viewportIterator).pageNumber < (int)d->m_pagesVector.count() - 1 )
        setViewport( DocumentViewport( (*d->m_viewportIterator).pageNumber + 1 ) );
}

void Document::setPrevPage()
{
    // go to previous page and set viewport on observers
    if ( (*d->m_viewportIterator).pageNumber > 0 )
        setViewport( DocumentViewport( (*d->m_viewportIterator).pageNumber - 1 ) );
}
*/
void Document::setViewportPage( int page, int excludeId, bool smoothMove )
{
    // clamp page in range [0 ... numPages-1]
    if ( page < 0 )
        page = 0;
    else if ( page > (int)d->m_pagesVector.count() )
        page = d->m_pagesVector.count() - 1;

    // make a viewport from the page and broadcast it
    setViewport( DocumentViewport( page ), excludeId, smoothMove );
}

void Document::setViewport( const DocumentViewport & viewport, int excludeId, bool smoothMove )
{
    // if already broadcasted, don't redo it
    DocumentViewport & oldViewport = *d->m_viewportIterator;
    // disabled by enrico on 2005-03-18 (less debug output)
    //if ( viewport == oldViewport )
    //    kDebug(OkularDebug) << "setViewport with the same viewport." << endl;

    // set internal viewport taking care of history
    if ( oldViewport.pageNumber == viewport.pageNumber || !oldViewport.isValid() )
    {
        // if page is unchanged save the viewport at current position in queue
        oldViewport = viewport;
    }
    else
    {
        // remove elements after viewportIterator in queue
        d->m_viewportHistory.erase( ++d->m_viewportIterator, d->m_viewportHistory.end() );

        // keep the list to a reasonable size by removing head when needed
        if ( d->m_viewportHistory.count() >= 100 )
            d->m_viewportHistory.pop_front();

        // add the item at the end of the queue
        d->m_viewportIterator = d->m_viewportHistory.insert( d->m_viewportHistory.end(), viewport );
    }

    // notify change to all other (different from id) observers
    QMap< int, DocumentObserver * >::const_iterator it = d->m_observers.begin(), end = d->m_observers.end();
    for ( ; it != end ; ++ it )
        if ( it.key() != excludeId )
            (*it)->notifyViewportChanged( smoothMove );

    // [MEM] raise position of currently viewed page in allocation queue
    if ( d->m_allocatedPixmapsFifo.count() > 1 )
    {
        const int page = viewport.pageNumber;
        QLinkedList< AllocatedPixmap * > viewportPixmaps;
        QLinkedList< AllocatedPixmap * >::iterator aIt = d->m_allocatedPixmapsFifo.begin();
        QLinkedList< AllocatedPixmap * >::iterator aEnd = d->m_allocatedPixmapsFifo.end();
        while ( aIt != aEnd )
        {
            if ( (*aIt)->page == page )
            {
                viewportPixmaps.append( *aIt );
                aIt = d->m_allocatedPixmapsFifo.erase( aIt );
                continue;
            }
            ++aIt;
        }
        if ( !viewportPixmaps.isEmpty() )
            d->m_allocatedPixmapsFifo += viewportPixmaps;
    }
}

void Document::setZoom(int factor, int excludeId)
{
    // notify change to all other (different from id) observers
    QMap< int, DocumentObserver * >::const_iterator it = d->m_observers.begin(), end = d->m_observers.end();
    for ( ; it != end ; ++ it )
        if ( it.key() != excludeId )
            (*it)->notifyZoom( factor );
}

void Document::setPrevViewport()
// restore viewport from the history
{
    if ( d->m_viewportIterator != d->m_viewportHistory.begin() )
    {
        // restore previous viewport and notify it to observers
        --d->m_viewportIterator;
        foreachObserver( notifyViewportChanged( true ) );
    }
}

void Document::setNextViewport()
// restore next viewport from the history
{
    QLinkedList< DocumentViewport >::const_iterator nextIterator = d->m_viewportIterator;
    ++nextIterator;
    if ( nextIterator != d->m_viewportHistory.end() )
    {
        // restore next viewport and notify it to observers
        ++d->m_viewportIterator;
        foreachObserver( notifyViewportChanged( true ) );
    }
}

void Document::setNextDocumentViewport( const DocumentViewport & viewport )
{
    d->m_nextDocumentViewport = viewport;
}

bool Document::searchText( int searchID, const QString & text, bool fromStart, Qt::CaseSensitivity caseSensitivity,
                               SearchType type, bool moveViewport, const QColor & color, bool noDialogs )
{
    // safety checks: don't perform searches on empty or unsearchable docs
    if ( !d->m_generator || !d->m_generator->hasFeature( Generator::TextExtraction ) || d->m_pagesVector.isEmpty() )
        return false;

    // if searchID search not recorded, create new descriptor and init params
    QMap< int, RunningSearch * >::iterator searchIt = d->m_searches.find( searchID );
    if ( searchIt == d->m_searches.end() )
    {
        RunningSearch * search = new RunningSearch();
        search->continueOnPage = -1;
        searchIt = d->m_searches.insert( searchID, search );
    }
    if (d->m_lastSearchID != searchID)
    {
        resetSearch(d->m_lastSearchID);
    }
    d->m_lastSearchID = searchID;
    RunningSearch * s = *searchIt;

    // update search stucture
    bool newText = text != s->cachedString;
    s->cachedString = text;
    s->cachedType = type;
    s->cachedCaseSensitivity = caseSensitivity;
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
        d->m_pagesVector[ *it ]->d->deleteHighlights( searchID );
    s->highlightedPages.clear();

    // set hourglass cursor
    QApplication::setOverrideCursor( Qt::WaitCursor );

    // 1. ALLDOC - proces all document marking pages
    if ( type == AllDocument )
    {
        // search and highlight 'text' (as a solid phrase) on all pages
        QVector< Page * >::const_iterator it = d->m_pagesVector.begin(), end = d->m_pagesVector.end();
        for ( ; it != end; ++it )
        {
            // get page (from the first to the last)
            Page * page = *it;
            int pageNumber = page->number();

            // request search page if needed
            if ( !page->hasTextPage() )
                requestTextPage( pageNumber );

            // loop on a page adding highlights for all found items
            bool addedHighlights = false;
            RegularAreaRect * lastMatch = 0;
            while ( 1 )
            {
                if ( lastMatch )
                {
                    RegularAreaRect * lastMatchOld = lastMatch;
                    lastMatch = page->findText( searchID, text, NextResult, caseSensitivity, lastMatch );
                    delete lastMatchOld;
                }
                else
                    lastMatch = page->findText( searchID, text, FromTop, caseSensitivity );

                if ( !lastMatch )
                    break;

                // add highligh rect to the page
                page->d->setHighlight( searchID, lastMatch, color );
                addedHighlights = true;
            }
            delete lastMatch;

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
        foreachObserver( notifySetup( d->m_pagesVector, false ) );
    }
    // 2. NEXTMATCH - find next matching item (or start from top)
    else if ( type == NextMatch )
    {
        // find out from where to start/resume search from
        int viewportPage = (*d->m_viewportIterator).pageNumber;
        int currentPage = fromStart ? 0 : ((s->continueOnPage != -1) ? s->continueOnPage : viewportPage);
        Page * lastPage = fromStart ? 0 : d->m_pagesVector[ currentPage ];

        // continue checking last TextPage first (if it is the current page)
        RegularAreaRect * match = 0;
        if ( lastPage && lastPage->number() == s->continueOnPage )
        {
            if ( newText )
                match = lastPage->findText( searchID, text, FromTop, caseSensitivity );
            else
                match = lastPage->findText( searchID, text, NextResult, caseSensitivity, &s->continueOnMatch );
            if ( !match )
                currentPage++;
        }

        // if no match found, loop through the whole doc, starting from currentPage
        if ( !match )
        {
            const int pageCount = d->m_pagesVector.count();
            for ( int i = 0; i < pageCount; i++ )
            {
                if ( currentPage >= pageCount )
                {
                    if ( noDialogs || KMessageBox::questionYesNo(widget(), i18n("End of document reached.\nContinue from the beginning?"), QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel()) == KMessageBox::Yes )
                        currentPage = 0;
                    else
                        break;
                }
                // get page
                Page * page = d->m_pagesVector[ currentPage ];
                // request search page if needed
                if ( !page->hasTextPage() )
                    requestTextPage( page->number() );
                // if found a match on the current page, end the loop
                if ( ( match = page->findText( searchID, text, FromTop, caseSensitivity ) ) )
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
            d->m_pagesVector[ currentPage ]->d->setHighlight( searchID, match, color );

            // ..queue page for notifying changes..
            if ( !pagesToNotify.contains( currentPage ) )
                pagesToNotify.append( currentPage );

            // ..move the viewport to show the first of the searched word sequence centered
            if ( moveViewport )
            {
                DocumentViewport searchViewport( currentPage );
                searchViewport.rePos.enabled = true;
                searchViewport.rePos.normalizedX = (match->first().left + match->first().right) / 2.0;
                searchViewport.rePos.normalizedY = (match->first().top + match->first().bottom) / 2.0;
                setViewport( searchViewport, -1, true );
            }
            delete match;
        }
        else if ( !noDialogs )
            KMessageBox::information( widget(), i18n( "No matches found for '%1'.", text ) );
    }
    // 3. PREVMATCH //TODO
    else if ( type == PreviousMatch )
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
        QVector< Page * >::const_iterator it = d->m_pagesVector.begin(), end = d->m_pagesVector.end();
        for ( ; it != end; ++it )
        {
            // get page (from the first to the last)
            Page * page = *it;
            int pageNumber = page->number();

            // request search page if needed
            if ( !page->hasTextPage() )
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
                    {
                        RegularAreaRect * lastMatchOld = lastMatch;
                        lastMatch = page->findText( searchID, word, NextResult, caseSensitivity, lastMatch );
                        delete lastMatchOld;
                    }
                    else
                        lastMatch = page->findText( searchID, word, FromTop, caseSensitivity);

                    if ( !lastMatch )
                        break;

                    // add highligh rect to the page
                    page->d->setHighlight( searchID, lastMatch, wordColor );
                    wordMatched = true;
                }
                allMatched = allMatched && wordMatched;
                anyMatched = anyMatched || wordMatched;
            }

            // if not all words are present in page, remove partial highlights
            if ( !allMatched && matchAll )
                page->d->deleteHighlights( searchID );

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
        foreachObserver( notifySetup( d->m_pagesVector, false ) );
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
    QMap< int, RunningSearch * >::const_iterator it = d->m_searches.constFind( searchID );
    if ( it == d->m_searches.constEnd() )
        return false;

    // start search with cached parameters from last search by searchID
    RunningSearch * p = *it;
    return searchText( searchID, p->cachedString, false, p->cachedCaseSensitivity,
                       p->cachedType, p->cachedViewportMove, p->cachedColor,
                       p->cachedNoDialogs );
}

void Document::resetSearch( int searchID )
{
    // check if searchID is present in runningSearches
    QMap< int, RunningSearch * >::iterator searchIt = d->m_searches.find( searchID );
    if ( searchIt == d->m_searches.end() )
        return;

    // get previous parameters for search
    RunningSearch * s = *searchIt;

    // unhighlight pages and inform observers about that
    QLinkedList< int >::const_iterator it = s->highlightedPages.begin(), end = s->highlightedPages.end();
    for ( ; it != end; ++it )
    {
        int pageNumber = *it;
        d->m_pagesVector[ pageNumber ]->d->deleteHighlights( searchID );
        foreachObserver( notifyPageChanged( pageNumber, DocumentObserver::Highlights ) );
    }

    // send the setup signal too (to update views that filter on matches)
    foreachObserver( notifySetup( d->m_pagesVector, false ) );

    // remove serch from the runningSearches list and delete it
    d->m_searches.erase( searchIt );
    delete s;
}

bool Document::continueLastSearch()
{
    return continueSearch( d->m_lastSearchID );
}


void Document::addBookmark( int n )
{
    if ( n >= 0 && n < (int)d->m_pagesVector.count() )
    {
        if ( d->m_bookmarkManager->setPageBookmark( n ) )
            foreachObserver( notifyPageChanged( n, DocumentObserver::Bookmark ) );
    }
}

void Document::addBookmark( const KUrl& referurl, const Okular::DocumentViewport& vp, const QString& title )
{
    if ( !vp.isValid() )
        return;

    if ( d->m_bookmarkManager->addBookmark( referurl, vp, title ) )
        foreachObserver( notifyPageChanged( vp.pageNumber, DocumentObserver::Bookmark ) );
}

bool Document::isBookmarked( int page ) const
{
    return d->m_bookmarkManager->isPageBookmarked( page );
}

void Document::removeBookmark( int n )
{
    if ( n >= 0 && n < (int)d->m_pagesVector.count() )
    {
        if ( d->m_bookmarkManager->removePageBookmark( n ) )
            foreachObserver( notifyPageChanged( n, DocumentObserver::Bookmark ) );
    }
}

void Document::removeBookmark( const KUrl& referurl, const KBookmark& bm )
{
    int changedpage = d->m_bookmarkManager->removeBookmark( referurl, bm );
    if ( changedpage != -1 )
        foreachObserver( notifyPageChanged( changedpage, DocumentObserver::Bookmark ) );
}

const BookmarkManager * Document::bookmarkManager() const
{
    return d->m_bookmarkManager;
}

void Document::processAction( const Action * action )
{
    if ( !action )
        return;

    switch( action->actionType() )
    {
        case Action::Goto: {
            const GotoAction * go = static_cast< const GotoAction * >( action );
            d->m_nextDocumentViewport = go->destViewport();

            // Explanation of why d->m_nextDocumentViewport is needed:
            // all openRelativeFile does is launch a signal telling we
            // want to open another URL, the problem is that when the file is
            // non local, the loading is done assynchronously so you can't
            // do a setViewport after the if as it was because you are doing the setViewport
            // on the old file and when the new arrives there is no setViewport for it and
            // it does not show anything

            // first open filename if link is pointing outside this document
            if ( go->isExternal() && !d->openRelativeFile( go->fileName() ) )
            {
                kWarning() << "Action: Error opening '" << go->fileName() << "'." << endl;
                return;
            }
            else
            {
                // skip local links that point to nowhere (broken ones)
                if (!d->m_nextDocumentViewport.isValid())
                    return;

                setViewport( d->m_nextDocumentViewport, -1, true );
                d->m_nextDocumentViewport = DocumentViewport();
            }

            } break;

        case Action::Execute: {
            const ExecuteAction * exe  = static_cast< const ExecuteAction * >( action );
            QString fileName = exe->fileName();
            if ( fileName.endsWith( ".pdf" ) || fileName.endsWith( ".PDF" ) )
            {
                d->openRelativeFile( fileName );
                return;
            }

            // Albert: the only pdf i have that has that kind of link don't define
            // an application and use the fileName as the file to open
            fileName = d->giveAbsolutePath( fileName );
            KMimeType::Ptr mime = KMimeType::findByPath( fileName );
            // Check executables
            if ( KRun::isExecutableFile( fileName, mime->name() ) )
            {
                // Don't have any pdf that uses this code path, just a guess on how it should work
                if ( !exe->parameters().isEmpty() )
                {
                    fileName = d->giveAbsolutePath( exe->parameters() );
                    mime = KMimeType::findByPath( fileName );
                    if ( KRun::isExecutableFile( fileName, mime->name() ) )
                    {
                        // this case is a link pointing to an executable with a parameter
                        // that also is an executable, possibly a hand-crafted pdf
                        KMessageBox::information( widget(), i18n("The document is trying to execute an external application and for your safety okular does not allow that.") );
                        return;
                    }
                }
                else
                {
                    // this case is a link pointing to an executable with no parameters
                    // core developers find unacceptable executing it even after asking the user
                    KMessageBox::information( widget(), i18n("The document is trying to execute an external application and for your safety okular does not allow that.") );
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
                KMessageBox::information( widget(), i18n( "No application found for opening file of mimetype %1.", mime->name() ) );
            } break;

        case Action::DocAction: {
            const DocumentAction * docaction = static_cast< const DocumentAction * >( action );
            switch( docaction->documentActionType() )
            {
                case DocumentAction::PageFirst:
                    setViewportPage( 0 );
                    break;
                case DocumentAction::PagePrev:
                    if ( (*d->m_viewportIterator).pageNumber > 0 )
                        setViewportPage( (*d->m_viewportIterator).pageNumber - 1 );
                    break;
                case DocumentAction::PageNext:
                    if ( (*d->m_viewportIterator).pageNumber < (int)d->m_pagesVector.count() - 1 )
                        setViewportPage( (*d->m_viewportIterator).pageNumber + 1 );
                    break;
                case DocumentAction::PageLast:
                    setViewportPage( d->m_pagesVector.count() - 1 );
                    break;
                case DocumentAction::HistoryBack:
                    setPrevViewport();
                    break;
                case DocumentAction::HistoryForward:
                    setNextViewport();
                    break;
                case DocumentAction::Quit:
                    emit quit();
                    break;
                case DocumentAction::Presentation:
                    emit linkPresentation();
                    break;
                case DocumentAction::EndPresentation:
                    emit linkEndPresentation();
                    break;
                case DocumentAction::Find:
                    emit linkFind();
                    break;
                case DocumentAction::GoToPage:
                    emit linkGoToPage();
                    break;
                case DocumentAction::Close:
                    emit close();
                    break;
            }
            } break;

        case Action::Browse: {
            const BrowseAction * browse = static_cast< const BrowseAction * >( action );
            // if the url is a mailto one, invoke mailer
            if ( browse->url().startsWith( "mailto:", Qt::CaseInsensitive ) )
                KToolInvocation::invokeMailer( browse->url() );
            else
            {
                QString url = browse->url();

                // fix for #100366, documents with relative links that are the form of http:foo.pdf
                if (url.indexOf("http:") == 0 && url.indexOf("http://") == -1 && url.right(4) == ".pdf")
                {
                    d->openRelativeFile(url.mid(5));
                    return;
                }

                // Albert: this is not a leak!
                new KRun( KUrl( url ), widget() );
            }
            } break;

        case Action::Sound: {
            const SoundAction * linksound = static_cast< const SoundAction * >( action );
            AudioPlayer::instance()->playSound( linksound->sound(), linksound );
            } break;

        case Action::Movie:
            //const MovieAction * movie = static_cast< const MovieAction * >( action );
            // TODO this (Movie action)
            break;
    }
}

void Document::processSourceReference( const SourceReference * ref )
{
    if ( !ref )
        return;

    if ( !QFile::exists( ref->fileName() ) )
    {
        kDebug(OkularDebug) << "No such file: '" << ref->fileName() << "'" << endl;
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

    QHash< int, QString >::const_iterator it = editors.constFind( Settings::externalEditor() );
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
    return d->m_generator ? d->m_generator->print( printer ) : false;
}

KPrintDialogPage* Document::printConfigurationWidget() const
{
    if ( d->m_generator )
    {
        PrintInterface * iface = qobject_cast< Okular::PrintInterface * >( d->m_generator );
        return iface ? iface->printConfigurationWidget() : 0;
    }
    else
        return 0;
}

void Document::fillConfigDialog( KConfigDialog * dialog )
{
    if ( !dialog )
        return;

    // ensure that we have all the generators with settings loaded
    QString constraint( "([X-KDE-Priority] > 0) and (exist Library) and ([X-KDE-okularHasInternalSettings])" );
    KService::List offers = KServiceTypeTrader::self()->query( "okular/Generator", constraint );
    d->loadServiceList( offers );

    bool pagesAdded = false;
    QHash< QString, GeneratorInfo >::iterator it = d->m_loadedGenerators.begin();
    QHash< QString, GeneratorInfo >::iterator itEnd = d->m_loadedGenerators.end();
    for ( ; it != itEnd; ++it )
    {
        Okular::ConfigInterface * iface = qobject_cast< Okular::ConfigInterface * >( it.value().generator );
        if ( iface )
        {
            iface->addPages( dialog );
            it.value().hasConfig = true;
            pagesAdded = true;
        }
    }
    if ( pagesAdded )
    {
        connect( dialog, SIGNAL( settingsChanged( const QString& ) ),
                 this, SLOT( slotGeneratorConfigChanged( const QString& ) ) );
    }
}

int Document::configurableGenerators() const
{
    QString constraint( "([X-KDE-Priority] > 0) and (exist Library) and ([X-KDE-okularHasInternalSettings])" );
    KService::List offers = KServiceTypeTrader::self()->query( "okular/Generator", constraint );
    return !offers.isEmpty();
}

QStringList Document::supportedMimeTypes() const
{
    if ( !d->m_supportedMimeTypes.isEmpty() )
        return d->m_supportedMimeTypes;

    QString constraint( "([X-KDE-Priority] > 0) and (exist Library)" );
    KService::List offers = KServiceTypeTrader::self()->query( "okular/Generator", constraint );
    KService::List::ConstIterator it = offers.begin(), itEnd = offers.end();
    for ( ; it != itEnd; ++it )
    {
        KService::Ptr service = *it;
        QStringList mimeTypes = service->serviceTypes();
        foreach ( const QString& mimeType, mimeTypes )
            if ( !mimeType.contains( "okular" ) )
                d->m_supportedMimeTypes.append( mimeType );
    }

    return d->m_supportedMimeTypes;
}

const KComponentData* Document::componentData() const
{
    return d->m_generator ? d->m_generator->componentData() : 0;
}

void Document::requestDone( PixmapRequest * req )
{
    if ( !d->m_generator || !req )
        return;

#ifndef NDEBUG
    if ( !d->m_generator->canGeneratePixmap() )
        kDebug(OkularDebug) << "requestDone with generator not in READY state." << endl;
#endif

    // [MEM] 1.1 find and remove a previous entry for the same page and id
    QLinkedList< AllocatedPixmap * >::iterator aIt = d->m_allocatedPixmapsFifo.begin();
    QLinkedList< AllocatedPixmap * >::iterator aEnd = d->m_allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        if ( (*aIt)->page == req->pageNumber() && (*aIt)->id == req->id() )
        {
            AllocatedPixmap * p = *aIt;
            d->m_allocatedPixmapsFifo.erase( aIt );
            d->m_allocatedPixmapsTotalMemory -= p->memory;
            delete p;
            break;
        }

    QMap< int, DocumentObserver * >::const_iterator itObserver = d->m_observers.constFind( req->id() );
    if ( itObserver != d->m_observers.constEnd() )
    {
        // [MEM] 1.2 append memory allocation descriptor to the FIFO
        qulonglong memoryBytes = 4 * req->width() * req->height();
        AllocatedPixmap * memoryPage = new AllocatedPixmap( req->id(), req->pageNumber(), memoryBytes );
        d->m_allocatedPixmapsFifo.append( memoryPage );
        d->m_allocatedPixmapsTotalMemory += memoryBytes;

        // 2. notify an observer that its pixmap changed
        itObserver.value()->notifyPageChanged( req->pageNumber(), DocumentObserver::Pixmap );
    }
#ifndef NDEBUG
    else
        kWarning() << "Receiving a done request for the defunct observer " << req->id() << endl;
#endif

    // 3. delete request
    delete req;

    // 4. start a new generation if some is pending
    d->m_pixmapRequestsMutex.lock();
    bool hasPixmaps = !d->m_pixmapRequestsStack.isEmpty();
    d->m_pixmapRequestsMutex.unlock();
    if ( hasPixmaps )
        d->sendGeneratorRequest();
}

void Document::setRotation( int r )
{
    if(!d->m_generator) 
	return;
    Rotation rotation = (Rotation)r;
    // tell the pages to rotate
    QVector< Okular::Page * >::const_iterator pIt = d->m_pagesVector.begin();
    QVector< Okular::Page * >::const_iterator pEnd = d->m_pagesVector.end();
    for ( ; pIt != pEnd; ++pIt )
        (*pIt)->d->rotateAt( rotation );
    // notify the generator that the current rotation has changed
    d->m_generator->rotationChanged( rotation, d->m_rotation );
    // set the new rotation
    d->m_rotation = rotation;

    foreachObserver( notifySetup( d->m_pagesVector, true ) );
    foreachObserver( notifyContentsCleared (DocumentObserver::Pixmap | DocumentObserver::Highlights | DocumentObserver::Annotations));
    kDebug(OkularDebug) << "Rotated: " << r << endl;
}

void Document::setPageSize( const PageSize &size )
{
    if ( !d->m_generator || !d->m_generator->hasFeature( Generator::PageSizes ) )
        return;

    if ( d->m_pageSizes.isEmpty() )
        d->m_pageSizes = d->m_generator->pageSizes();
    int sizeid = d->m_pageSizes.indexOf( size );
    if ( sizeid == -1 )
        return;

    // tell the pages to change size
    QVector< Okular::Page * >::const_iterator pIt = d->m_pagesVector.begin();
    QVector< Okular::Page * >::const_iterator pEnd = d->m_pagesVector.end();
    for ( ; pIt != pEnd; ++pIt )
        (*pIt)->d->changeSize( size );
    // clear 'memory allocation' descriptors
    QLinkedList< AllocatedPixmap * >::const_iterator aIt = d->m_allocatedPixmapsFifo.begin();
    QLinkedList< AllocatedPixmap * >::const_iterator aEnd = d->m_allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    d->m_allocatedPixmapsFifo.clear();
    d->m_allocatedPixmapsTotalMemory = 0;
    // notify the generator that the current page size has changed
    d->m_generator->pageSizeChanged( size, d->m_pageSize );
    // set the new page size
    d->m_pageSize = size;

    foreachObserver( notifySetup( d->m_pagesVector, true ) );
    foreachObserver( notifyContentsCleared( DocumentObserver::Pixmap | DocumentObserver::Highlights ) );
    kDebug(OkularDebug) << "New PageSize id: " << sizeid << endl;
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

bool DocumentViewport::isValid() const
{
    return pageNumber >= 0;
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

void DocumentInfo::set( enum Key key, const QString &value )
{
    switch ( key ) {
        case Title:
            set( "title", value, i18n( "Title" ) );
            break;
        case Subject:
            set( "subject", value, i18n( "Subject" ) );
            break;
        case Description:
            set( "description", value, i18n( "Description" ) );
            break;
        case Author:
            set( "author", value, i18n( "Author" ) );
            break;
        case Creator:
            set( "creator", value, i18n( "Creator" ) );
            break;
        case Producer:
            set( "producer", value, i18n( "Producer" ) );
            break;
        case Copyright:
            set( "copyright", value, i18n( "Copyright" ) );
            break;
        case Pages:
            set( "pages", value, i18n( "Pages" ) );
            break;
        case CreationDate:
            set( "creationDate", value, i18n( "Created" ) );
            break;
        case ModificationDate:
            set( "modificationDate", value, i18n( "Modified" ) );
            break;
        case MimeType:
            set( "mimeType", value, i18n( "Mime Type" ) );
            break;
        case Category:
            set( "category", value, i18n( "Category" ) );
            break;
        case Keywords:
            set( "keywords", value, i18n( "Keywords" ) );
            break;
        default:
            kWarning() << "DocumentInfo::set(): Invalid key passed" << endl;
            break;
    }
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

/** EmbeddedFile **/

EmbeddedFile::EmbeddedFile()
{
}

EmbeddedFile::~EmbeddedFile()
{
}

VisiblePageRect::VisiblePageRect( int page, const NormalizedRect &rectangle )
    : pageNumber( page ), rect( rectangle )
{
}

#include "document.moc"
