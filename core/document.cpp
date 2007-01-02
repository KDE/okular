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
#include "bookmarkmanager.h"
#include "chooseenginedialog.h"
#include "configinterface.h"
#include "document.h"
#include "generator.h"
#include "guiinterface.h"
#include "link.h"
#include "observer.h"
#include "page.h"
#include "printinterface.h"
#include "settings.h"
#include "sourcereference.h"

using namespace Okular;

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
    Qt::CaseSensitivity cachedCaseSensitivity;
    bool cachedViewportMove;
    bool cachedNoDialogs;
    QColor cachedColor;
};

#define foreachObserver( cmd ) {\
    QMap< int, DocumentObserver * >::const_iterator it=d->m_observers.begin(), end=d->m_observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

/***** Document ******/

class Document::Private
{
    public:
        Private( Document *parent, QHash<QString, Generator*> * generators )
          : m_parent( parent ),
            m_lastSearchID( -1 ),
            m_allocatedPixmapsTotalMemory( 0 ),
            m_warnedOutOfMemory( false ),
            m_rotation( 0 ),
            m_bookmarkManager( 0 ),
            m_memCheckTimer( 0 ),
            m_saveBookmarksTimer( 0 ),
            m_loadedGenerators ( generators ),
            m_generator( 0 ),
            m_usingCachedGenerator( false )
        {
        }

        // private methods
        QString pagesSizeString() const;
        QString localizedSize(const QSizeF &size) const;
        void cleanupPixmapMemory( int bytesOffset = 0 );
        int getTotalMemory();
        int getFreeMemory();
        void loadDocumentInfo();
        QString giveAbsolutePath( const QString & fileName );
        bool openRelativeFile( const QString & fileName );

        // private slots
        void saveDocumentInfo() const;
        void slotTimedMemoryCheck();
        void sendGeneratorRequest();
        void rotationFinished( int page );

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

        // viewport stuff
        QLinkedList< DocumentViewport > m_viewportHistory;
        QLinkedList< DocumentViewport >::iterator m_viewportIterator;
        DocumentViewport m_nextDocumentViewport; // see Link::Goto for an explanation

        // observers / requests / allocator stuff
        QMap< int, DocumentObserver * > m_observers;
        QLinkedList< PixmapRequest * > m_pixmapRequestsStack;
        QLinkedList< AllocatedPixmap * > m_allocatedPixmapsFifo;
        int m_allocatedPixmapsTotalMemory;
        bool m_warnedOutOfMemory;

        // the rotation applied to the document, 0,1,2,3 * 90 degrees
        int m_rotation;

        // our bookmark manager
        BookmarkManager *m_bookmarkManager;

        // timers (memory checking / info saver)
        QTimer *m_memCheckTimer;
        QTimer *m_saveBookmarksTimer;

        QHash<QString, Generator*>* m_loadedGenerators ;
        Generator * m_generator;
        bool m_usingCachedGenerator;
        QVector< Page * > m_pagesVector;
        QVector< VisiblePageRect * > m_pageRects;
};

QString Document::Private::pagesSizeString() const
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

QString Document::Private::localizedSize(const QSizeF &size) const
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

void Document::Private::cleanupPixmapMemory( int /*sure? bytesOffset*/ )
{
    // [MEM] choose memory parameters based on configuration profile
    int clipValue = -1;
    int memoryToFree = -1;
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

int Document::Private::getTotalMemory()
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

int Document::Private::getFreeMemory()
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

void Document::Private::loadDocumentInfo()
// note: load data and stores it internally (document or pages). observers
// are still uninitialized at this point so don't access them
{
    //kDebug() << "Using '" << d->m_xmlFileName << "' as document info file." << endl;
    QFile infoFile( m_xmlFileName );
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
                    if ( ok && pageNumber >= 0 && pageNumber < (int)m_pagesVector.count() )
                        m_pagesVector[ pageNumber ]->restoreLocalContents( pageElement );
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
                        *m_viewportIterator = DocumentViewport( infoElement.attribute( "viewport" ) );
                }

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

QString Document::Private::giveAbsolutePath( const QString & fileName )
{
    if ( !m_url.isValid() )
        return QString();

    return m_url.upUrl().url() + fileName;
}

bool Document::Private::openRelativeFile( const QString & fileName )
{
    QString absFileName = giveAbsolutePath( fileName );
    if ( absFileName.isEmpty() )
        return false;

    kDebug() << "openDocument: '" << absFileName << "'" << endl;

    emit m_parent->openUrl( absFileName );
    return true;
}

void Document::Private::saveDocumentInfo() const
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
            (*pIt)->saveLocalContents( pageList, doc );

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

void Document::Private::slotTimedMemoryCheck()
{
    // [MEM] clean memory (for 'free mem dependant' profiles only)
    if ( Settings::memoryLevel() != Settings::EnumMemoryLevel::Low &&
         m_allocatedPixmapsTotalMemory > 1024*1024 )
        cleanupPixmapMemory();
}

void Document::Private::sendGeneratorRequest()
{
    // find a request
    PixmapRequest * request = 0;
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
        return;

    // [MEM] preventive memory freeing
    int pixmapBytes = 4 * request->width() * request->height();
    if ( pixmapBytes > (1024 * 1024) )
        cleanupPixmapMemory( pixmapBytes );

    // submit the request to the generator
    if ( m_generator->canGeneratePixmap( request->asynchronous() ) )
    {
        kWarning() << "sending request id=" << request->id() << " " <<request->width() << "x" << request->height() << "@" << request->pageNumber() << " async == " << request->asynchronous() << endl;
        m_pixmapRequestsStack.removeAll ( request );

        if ( m_rotation % 2 )
            request->swap();

        m_generator->generatePixmap ( request );
    }
    else
        // pino (7/4/2006): set the polling interval from 10 to 30
        QTimer::singleShot( 30, m_parent, SLOT(sendGeneratorRequest()) );
}

void Document::Private::rotationFinished( int page )
{
    QMap< int, DocumentObserver * >::const_iterator it = m_observers.begin(), end = m_observers.end();
    for ( ; it != end ; ++ it ) {
        (*it)->notifyPageChanged( page, DocumentObserver::Pixmap | DocumentObserver::Annotations );
    }
}


Document::Document( QHash<QString, Generator*> * generators )
    : d( new Private( this, generators ) )
{
    d->m_bookmarkManager = new BookmarkManager( this );
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
        d->m_docFileName.clear();
        return false;
    }
    // determine the related "xml document-info" filename
    d->m_url = url;
    d->m_docFileName = docFile;
    QString fn = docFile.contains('/') ? docFile.section('/', -1, -1) : docFile;
    fn = "kpdf/" + QString::number(fileReadTest.size()) + '.' + fn + ".xml";
    fileReadTest.close();
    d->m_xmlFileName = KStandardDirs::locateLocal( "data", fn );

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
    d->m_usingCachedGenerator=false;
    d->m_generator = d->m_loadedGenerators->take(propName);
    if (!d->m_generator)
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
        d->m_generator = create_plugin();

        if ( !d->m_generator )
        {
            kWarning() << "Sth broke." << endl;
            return false;
        }
        if ( offers.at(hRank)->property( "X-KDE-okularHasInternalSettings" ).toBool() )
        {
            d->m_loadedGenerators->insert(propName, d->m_generator);
            d->m_usingCachedGenerator=true;
        }
        // end 
    }
    else
    {
        d->m_usingCachedGenerator=true;
    }
    d->m_generator->setDocument( this );

    // connect error reporting signals
    connect( d->m_generator, SIGNAL( error( const QString&, int ) ), this, SIGNAL( error( const QString&, int ) ) );
    connect( d->m_generator, SIGNAL( warning( const QString&, int ) ), this, SIGNAL( warning( const QString&, int ) ) );
    connect( d->m_generator, SIGNAL( notice( const QString&, int ) ), this, SIGNAL( notice( const QString&, int ) ) );

    // 1. load Document (and set busy cursor while loading)
    QApplication::setOverrideCursor( Qt::WaitCursor );
    bool openOk = d->m_generator->loadDocument( docFile, d->m_pagesVector );

    for ( int i = 0; i < d->m_pagesVector.count(); ++i )
        connect( d->m_pagesVector[ i ], SIGNAL( rotationFinished( int ) ),
                 this, SLOT( rotationFinished( int ) ) );

    QApplication::restoreOverrideCursor();
    if ( !openOk || d->m_pagesVector.size() <= 0 )
    {
        if (!d->m_usingCachedGenerator)
        {
            delete d->m_generator;
        }
        d->m_generator = 0;
        return openOk;
    }

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

    return true;
}


QString Document::xmlFile()
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
    // close the current document and save document info if a document is still opened
    if ( d->m_generator && d->m_pagesVector.size() > 0 )
    {
        d->m_generator->closeDocument();
        d->saveDocumentInfo();
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
    }
    if (!d->m_usingCachedGenerator)
    {
        // delete contents generator
        delete d->m_generator;
    }
    d->m_generator = 0;
    d->m_url = KUrl();
    // remove requests left in queue
    QLinkedList< PixmapRequest * >::const_iterator sIt = d->m_pixmapRequestsStack.begin();
    QLinkedList< PixmapRequest * >::const_iterator sEnd = d->m_pixmapRequestsStack.end();
    for ( ; sIt != sEnd; ++sIt )
        delete *sIt;
    d->m_pixmapRequestsStack.clear();

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
}

void Document::addObserver( DocumentObserver * pObserver )
{
    // keep the pointer to the observer in a map
    d->m_observers[ pObserver->observerId() ] = pObserver;

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
            (*it)->deleteRects();
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
        DocumentInfo *info = const_cast<DocumentInfo *>(d->m_generator->generateDocumentInfo());
        QString pagesSize = d->pagesSizeString();
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

const DocumentFonts * Document::documentFonts() const
{
    return d->m_generator ? d->m_generator->generateDocumentFonts() : NULL;
}

const QList<EmbeddedFile*> *Document::embeddedFiles() const
{
    return d->m_generator ? d->m_generator->embeddedFiles() : NULL;
}

const Page * Document::page( int n ) const
{
    return ( n < d->m_pagesVector.count() ) ? d->m_pagesVector[n] : 0;
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

bool Document::isAllowed( int flags ) const
{
    return d->m_generator ? d->m_generator->isAllowed( flags ) : false;
}

bool Document::supportsSearching() const
{
    return d->m_generator ? d->m_generator->supportsSearching() : false;
}

bool Document::supportsRotation() const
{
    return d->m_generator ? d->m_generator->supportsRotation() : false;
}

bool Document::supportsPaperSizes() const
{
    return d->m_generator ? d->m_generator->supportsPaperSizes() : false;
}

QStringList Document::paperSizes() const
{
    return d->m_generator ? d->m_generator->paperSizes() : QStringList();
}

bool Document::canExportToText() const
{
    if ( !d->m_generator )
        return false;

    const ExportFormat::List formats = d->m_generator->exportFormats();
    for ( int i = 0; i < formats.count(); ++i ) {
        if ( formats[ i ].mimeType()->name() == QLatin1String( "text/plain" ) )
            return true;
    }

    return false;
}

bool Document::exportToText( const QString& fileName ) const
{
    if ( !d->m_generator )
        return false;

    const ExportFormat::List formats = d->m_generator->exportFormats();
    for ( int i = 0; i < formats.count(); ++i ) {
        if ( formats[ i ].mimeType()->name() == QLatin1String( "text/plain" ) )
            return d->m_generator->exportTo( fileName, formats[ i ] );
    }

    return false;
}

ExportFormat::List Document::exportFormats() const
{
    return d->m_generator ? d->m_generator->exportFormats() : ExportFormat::List();
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

QVariant Document::getMetaData( const QString & key, const QVariant & option ) const
{
    return d->m_generator ? d->m_generator->metaData( key, option ) : QVariant();
}

int Document::rotation() const
{
    return d->m_rotation;
}

QSizeF Document::allPagesSize() const
{
    bool allPagesSameSize = true;
    QSizeF size;
    for (int i = 0; allPagesSameSize && i < d->m_pagesVector.count(); ++i)
    {
        Page *p = d->m_pagesVector[i];
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
            Page *p = d->m_pagesVector[page];
            return d->localizedSize(QSizeF(p->width(), p->height()));
        }
    }
    return QString();
}

void Document::requestPixmaps( const QLinkedList< PixmapRequest * > & requests )
{
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
        kWarning() << "request id=" << request->id() << " " <<request->width() << "x" << request->height() << "@" << request->pageNumber() << endl;
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
            while ( sIt != sEnd && (*sIt)->priority() >= request->priority() )
                ++sIt;
            d->m_pixmapRequestsStack.insert( sIt, request );
        }
    }

    // 3. [START FIRST GENERATION] if <NO>generator is ready, start a new generation,
    // or else (if gen is running) it will be started when the new contents will
    //come from generator (in requestDone())</NO>
    // all handling of requests put into sendGeneratorRequest
    //    if ( generator->canGeneratePixmap() )
        d->sendGeneratorRequest();
}

void Document::requestTextPage( uint page )
{
    Page * kp = d->m_pagesVector[ page ];
    if ( !d->m_generator || !kp )
        return;

    // Memory management for TextPages

    d->m_generator->generateSyncTextPage( kp );
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
    
    kp->modifyAnnotation( newannotation );
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

void Document::removePageAnnotations( int page, QList< Annotation * > annotations )
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
    //    kDebug() << "setViewport with the same viewport." << endl;

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
    if ( !d->m_generator || !d->m_generator->supportsSearching() || d->m_pagesVector.isEmpty() )
        return false;

    // if searchID search not recorded, create new descriptor and init params
    if ( !d->m_searches.contains( searchID ) )
    {
        RunningSearch * search = new RunningSearch();
        search->continueOnPage = -1;
        d->m_searches[ searchID ] = search;
    }
    if (d->m_lastSearchID != searchID)
    {
        resetSearch(d->m_lastSearchID);
    }
    d->m_lastSearchID = searchID;
    RunningSearch * s = d->m_searches[ searchID ];

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
        d->m_pagesVector[ *it ]->deleteHighlights( searchID );
    s->highlightedPages.clear();

    // set hourglass cursor
    QApplication::setOverrideCursor( Qt::WaitCursor );

    // 1. ALLDOC - proces all document marking pages
    if ( type == AllDoc )
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
                    lastMatch = page->findText( searchID, text, NextResult, caseSensitivity, lastMatch );
                else
                    lastMatch = page->findText( searchID, text, FromTop, caseSensitivity );

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
                    if ( noDialogs || KMessageBox::questionYesNo(0, i18n("End of document reached.\nContinue from the beginning?"), QString::null, KStandardGuiItem::cont(), KStandardGuiItem::cancel()) == KMessageBox::Yes )
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
            d->m_pagesVector[ currentPage ]->setHighlight( searchID, match, color );

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
                        lastMatch = page->findText( searchID, word, NextResult, caseSensitivity, lastMatch );
                    else
                        lastMatch = page->findText( searchID, word, FromTop, caseSensitivity);

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
    if ( !d->m_searches.contains( searchID ) )
        return false;

    // start search with cached parameters from last search by searchID
    RunningSearch * p = d->m_searches[ searchID ];
    return searchText( searchID, p->cachedString, false, p->cachedCaseSensitivity,
                       p->cachedType, p->cachedViewportMove, p->cachedColor,
                       p->cachedNoDialogs );
}

void Document::resetSearch( int searchID )
{
    // check if searchID is present in runningSearches
    if ( !d->m_searches.contains( searchID ) )
        return;

    // get previous parameters for search
    RunningSearch * s = d->m_searches[ searchID ];

    // unhighlight pages and inform observers about that
    QLinkedList< int >::const_iterator it = s->highlightedPages.begin(), end = s->highlightedPages.end();
    for ( ; it != end; ++it )
    {
        int pageNumber = *it;
        d->m_pagesVector[ pageNumber ]->deleteHighlights( searchID );
        foreachObserver( notifyPageChanged( pageNumber, DocumentObserver::Highlights ) );
    }

    // send the setup signal too (to update views that filter on matches)
    foreachObserver( notifySetup( d->m_pagesVector, false ) );

    // remove serch from the runningSearches list and delete it
    d->m_searches.remove( searchID );
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

void Document::processLink( const Link * link )
{
    if ( !link )
        return;

    switch( link->linkType() )
    {
        case Link::Goto: {
            const LinkGoto * go = static_cast< const LinkGoto * >( link );
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
                kWarning() << "Link: Error opening '" << go->fileName() << "'." << endl;
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

        case Link::Execute: {
            const LinkExecute * exe  = static_cast< const LinkExecute * >( link );
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
                    if ( (*d->m_viewportIterator).pageNumber > 0 )
                        setViewportPage( (*d->m_viewportIterator).pageNumber - 1 );
                    break;
                case LinkAction::PageNext:
                    if ( (*d->m_viewportIterator).pageNumber < (int)d->m_pagesVector.count() - 1 )
                        setViewportPage( (*d->m_viewportIterator).pageNumber + 1 );
                    break;
                case LinkAction::PageLast:
                    setViewportPage( d->m_pagesVector.count() - 1 );
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
                    d->openRelativeFile(url.mid(5));
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
        case Link::Sound:
            // TODO this (Sound link)
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
    return d->m_generator ? d->m_generator->print( printer ) : false;
}

KPrintDialogPage* Document::configurationWidget() const
{
    if ( d->m_generator )
    {
        PrintInterface * iface = qobject_cast< Okular::PrintInterface * >( d->m_generator );
        return iface ? iface->configurationWidget() : 0;
    }
    else
        return 0;
}

void Document::requestDone( PixmapRequest * req )
{
#ifndef NDEBUG
    if ( !d->m_generator->canGeneratePixmap( req->asynchronous() ) )
        kDebug() << "requestDone with generator not in READY state." << endl;
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

    // [MEM] 1.2 append memory allocation descriptor to the FIFO
    int memoryBytes = 4 * req->width() * req->height();
    AllocatedPixmap * memoryPage = new AllocatedPixmap( req->id(), req->pageNumber(), memoryBytes );
    d->m_allocatedPixmapsFifo.append( memoryPage );
    d->m_allocatedPixmapsTotalMemory += memoryBytes;

    // 2. notify an observer that its pixmap changed
    if ( d->m_observers.contains( req->id() ) )
        d->m_observers[ req->id() ]->notifyPageChanged( req->pageNumber(), DocumentObserver::Pixmap );

    // 3. delete request
    delete req;

    // 4. start a new generation if some is pending
    if ( !d->m_pixmapRequestsStack.isEmpty() )
        d->sendGeneratorRequest();
}

void Document::slotRotation( int rotation )
{
    // tell the pages to rotate
    QVector< Okular::Page * >::const_iterator pIt = d->m_pagesVector.begin();
    QVector< Okular::Page * >::const_iterator pEnd = d->m_pagesVector.end();
    for ( ; pIt != pEnd; ++pIt )
        (*pIt)->rotateAt( rotation );
    // clear 'memory allocation' descriptors
    QLinkedList< AllocatedPixmap * >::const_iterator aIt = d->m_allocatedPixmapsFifo.begin();
    QLinkedList< AllocatedPixmap * >::const_iterator aEnd = d->m_allocatedPixmapsFifo.end();
    for ( ; aIt != aEnd; ++aIt )
        delete *aIt;
    d->m_allocatedPixmapsFifo.clear();
    d->m_allocatedPixmapsTotalMemory = 0;
    // notify the generator that the current rotation has changed
    d->m_generator->rotationChanged( rotation, d->m_rotation );
    // set the new rotation
    d->m_rotation = rotation;

    foreachObserver( notifySetup( d->m_pagesVector, true ) );
    foreachObserver( notifyContentsCleared (DocumentObserver::Pixmap | DocumentObserver::Highlights | DocumentObserver::Annotations));
    kDebug() << "Rotated: " << rotation << endl;
}

void Document::slotPaperSizes( int newsize )
{
    if (d->m_generator->supportsPaperSizes())
    {
        d->m_generator->setPaperSize(d->m_pagesVector,newsize);
        foreachObserver( notifySetup( d->m_pagesVector, true ) );
        kDebug() << "PaperSize no: " << newsize << endl;
    }
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
    return pageNumber != -1;
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

VisiblePageRect::VisiblePageRect( int page, const NormalizedRect &rectangle )
    : pageNumber( page ), rect( rectangle )
{
}

#include "document.moc"
