/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *   Copyright (C) 2004-2008 by Albert Astals Cid <aacid@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "document.h"
#include "document_p.h"
#include "documentcommands_p.h"

#include <limits.h>
#ifdef Q_OS_WIN
#define _WIN32_WINNT 0x0500
#include <windows.h>
#elif defined(Q_OS_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#endif

// qt/kde/system includes
#include <QtCore/QtAlgorithms>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QUndoCommand>

#include <kaboutdata.h>
#include <kauthorized.h>
#include <kcomponentdata.h>
#include <kconfigdialog.h>
#include <kdebug.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmacroexpander.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kprocess.h>
#include <krun.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>
#include <ktoolinvocation.h>
#include <kzip.h>

// local includes
#include "action.h"
#include "annotations.h"
#include "annotations_p.h"
#include "audioplayer.h"
#include "audioplayer_p.h"
#include "bookmarkmanager.h"
#include "chooseenginedialog_p.h"
#include "debug_p.h"
#include "generator_p.h"
#include "interfaces/configinterface.h"
#include "interfaces/guiinterface.h"
#include "interfaces/printinterface.h"
#include "interfaces/saveinterface.h"
#include "observer.h"
#include "misc.h"
#include "page.h"
#include "page_p.h"
#include "pagecontroller_p.h"
#include "scripter.h"
#include "settings_core.h"
#include "sourcereference.h"
#include "sourcereference_p.h"
#include "texteditors_p.h"
#include "tile.h"
#include "tilesmanager_p.h"
#include "utils_p.h"
#include "view.h"
#include "view_p.h"
#include "form.h"
#include "utils.h"

#include <memory>

#include <config-okular.h>

using namespace Okular;

struct AllocatedPixmap
{
    // owner of the page
    DocumentObserver *observer;
    int page;
    qulonglong memory;
    // public constructor: initialize data
    AllocatedPixmap( DocumentObserver *o, int p, qulonglong m ) : observer( o ), page( p ), memory( m ) {}
};

struct ArchiveData
{
    ArchiveData()
    {
    }

    KTemporaryFile document;
    QString metadataFileName;
};

struct RunningSearch
{
    // store search properties
    int continueOnPage;
    RegularAreaRect continueOnMatch;
    QSet< int > highlightedPages;

    // fields related to previous searches (used for 'continueSearch')
    QString cachedString;
    Document::SearchType cachedType;
    Qt::CaseSensitivity cachedCaseSensitivity;
    bool cachedViewportMove : 1;
    bool cachedNoDialogs : 1;
    bool isCurrentlySearching : 1;
    QColor cachedColor;
};

#define foreachObserver( cmd ) {\
    QSet< DocumentObserver * >::const_iterator it=d->m_observers.constBegin(), end=d->m_observers.constEnd();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

#define foreachObserverD( cmd ) {\
    QSet< DocumentObserver * >::const_iterator it = m_observers.constBegin(), end = m_observers.constEnd();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

#define OKULAR_HISTORY_MAXSTEPS 100
#define OKULAR_HISTORY_SAVEDSTEPS 10

/***** Document ******/

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

QString DocumentPrivate::namePaperSize(double inchesWidth, double inchesHeight) const
{
    // Account for small deviations in paper sizes
    static const double marginFactor = 0.03;
    static const double lowerBoundFactor = 1.0 - marginFactor;
    static const double upperBoundFactor = 1.0 + marginFactor;

    const QPrinter::Orientation orientation = inchesWidth > inchesHeight ? QPrinter::Landscape : QPrinter::Portrait;
    // enforce portrait mode for further tests
    if (inchesWidth > inchesHeight)
        qSwap(inchesWidth, inchesHeight);

    // Use QPrinter to find which of the predefined paper sizes
    // matches best the given paper width and height
    QPrinter dummyPrinter;
    QPrinter::PaperSize paperSize = QPrinter::Custom;
    for (int i = 0; i < (int)QPrinter::NPaperSize; ++i)
    {
        const QPrinter::PaperSize ps = (QPrinter::PaperSize)i;
        dummyPrinter.setPaperSize(ps);
        const QSizeF definedPaperSize = dummyPrinter.paperSize(QPrinter::Inch);

        if (inchesWidth > definedPaperSize.width() * lowerBoundFactor && inchesWidth < definedPaperSize.width() * upperBoundFactor
            && inchesHeight > definedPaperSize.height() * lowerBoundFactor && inchesHeight < definedPaperSize.height() * upperBoundFactor)
        {
            paperSize = ps;
            break;
        }
    }

    // Handle all paper sizes defined in QPrinter,
    // return string depending if paper's orientation is landscape or portrait
    switch (paperSize) {
        case QPrinter::A0:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A0") : i18nc("paper size", "portrait DIN/ISO A0");
        case QPrinter::A1:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A1") : i18nc("paper size", "portrait DIN/ISO A1");
        case QPrinter::A2:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A2") : i18nc("paper size", "portrait DIN/ISO A2");
        case QPrinter::A3:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A3") : i18nc("paper size", "portrait DIN/ISO A3");
        case QPrinter::A4:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A4") : i18nc("paper size", "portrait DIN/ISO A4");
        case QPrinter::A5:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A5") : i18nc("paper size", "portrait DIN/ISO A5");
        case QPrinter::A6:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A6") : i18nc("paper size", "portrait DIN/ISO A6");
        case QPrinter::A7:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A7") : i18nc("paper size", "portrait DIN/ISO A7");
        case QPrinter::A8:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A8") : i18nc("paper size", "portrait DIN/ISO A8");
        case QPrinter::A9:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO A9") : i18nc("paper size", "portrait DIN/ISO A9");
        case QPrinter::B0:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B0") : i18nc("paper size", "portrait DIN/ISO B0");
        case QPrinter::B1:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B1") : i18nc("paper size", "portrait DIN/ISO B1");
        case QPrinter::B2:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B2") : i18nc("paper size", "portrait DIN/ISO B2");
        case QPrinter::B3:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B3") : i18nc("paper size", "portrait DIN/ISO B3");
        case QPrinter::B4:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B4") : i18nc("paper size", "portrait DIN/ISO B4");
        case QPrinter::B5:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B5") : i18nc("paper size", "portrait DIN/ISO B5");
        case QPrinter::B6:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B6") : i18nc("paper size", "portrait DIN/ISO B6");
        case QPrinter::B7:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B7") : i18nc("paper size", "portrait DIN/ISO B7");
        case QPrinter::B8:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B8") : i18nc("paper size", "portrait DIN/ISO B8");
        case QPrinter::B9:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B9") : i18nc("paper size", "portrait DIN/ISO B9");
        case QPrinter::B10:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DIN/ISO B10") : i18nc("paper size", "portrait DIN/ISO B10");
        case QPrinter::Letter:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape letter") : i18nc("paper size", "portrait letter");
        case QPrinter::Legal:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape legal") : i18nc("paper size", "portrait legal");
        case QPrinter::Executive:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape executive") : i18nc("paper size", "portrait executive");
        case QPrinter::C5E:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape C5E") : i18nc("paper size", "portrait C5E");
        case QPrinter::Comm10E:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape Comm10E") : i18nc("paper size", "portrait Comm10E");
        case QPrinter::DLE:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape DLE") : i18nc("paper size", "portrait DLE");
        case QPrinter::Folio:
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "landscape folio") : i18nc("paper size", "portrait folio");
        case QPrinter::Tabloid:
        case QPrinter::Ledger:
            /// Ledger and Tabloid are the same, just rotated by 90 degrees
            return  orientation == QPrinter::Landscape ? i18nc("paper size", "ledger") : i18nc("paper size", "tabloid");
        case QPrinter::Custom:
            return orientation == QPrinter::Landscape ? i18nc("paper size", "unknown landscape paper size") : i18nc("paper size", "unknown portrait paper size");
    }

    kWarning() << "PaperSize" << paperSize << "has not been covered";
    return QString();
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

        case Generator::Pixels:
        {
            const QSizeF dpi = m_generator->dpi();
            inchesWidth = size.width() / dpi.width();
            inchesHeight = size.height() / dpi.height();
        }
        break;

        case Generator::None:
        break;
    }
    if (KGlobal::locale()->measureSystem() == KLocale::Imperial)
    {
        return i18nc("%1 is width, %2 is height, %3 is paper size name", "%1 x %2 in (%3)", inchesWidth, inchesHeight, namePaperSize(inchesWidth, inchesHeight));
    }
    else
    {
        return i18nc("%1 is width, %2 is height, %3 is paper size name", "%1 x %2 mm (%3)", QString::number(inchesWidth * 25.4, 'd', 0), QString::number(inchesHeight * 25.4, 'd', 0), namePaperSize(inchesWidth, inchesHeight));
    }
}

qulonglong DocumentPrivate::calculateMemoryToFree()
{
    // [MEM] choose memory parameters based on configuration profile
    qulonglong clipValue = 0;
    qulonglong memoryToFree = 0;

    switch ( SettingsCore::memoryLevel() )
    {
        case SettingsCore::EnumMemoryLevel::Low:
            memoryToFree = m_allocatedPixmapsTotalMemory;
            break;

        case SettingsCore::EnumMemoryLevel::Normal:
        {
            qulonglong thirdTotalMemory = getTotalMemory() / 3;
            qulonglong freeMemory = getFreeMemory();
            if (m_allocatedPixmapsTotalMemory > thirdTotalMemory) memoryToFree = m_allocatedPixmapsTotalMemory - thirdTotalMemory;
            if (m_allocatedPixmapsTotalMemory > freeMemory) clipValue = (m_allocatedPixmapsTotalMemory - freeMemory) / 2;
        }
        break;

        case SettingsCore::EnumMemoryLevel::Aggressive:
        {
            qulonglong freeMemory = getFreeMemory();
            if (m_allocatedPixmapsTotalMemory > freeMemory) clipValue = (m_allocatedPixmapsTotalMemory - freeMemory) / 2;
        }
        break;
        case SettingsCore::EnumMemoryLevel::Greedy:
        {
            qulonglong freeSwap;
            qulonglong freeMemory = getFreeMemory( &freeSwap );
            const qulonglong memoryLimit = qMin( qMax( freeMemory, getTotalMemory()/2 ), freeMemory+freeSwap );
            if (m_allocatedPixmapsTotalMemory > memoryLimit) clipValue = (m_allocatedPixmapsTotalMemory - memoryLimit) / 2;
        }
        break;
    }

    if ( clipValue > memoryToFree )
        memoryToFree = clipValue;

    return memoryToFree;
}

void DocumentPrivate::cleanupPixmapMemory()
{
    cleanupPixmapMemory( calculateMemoryToFree() );
}

void DocumentPrivate::cleanupPixmapMemory( qulonglong memoryToFree )
{
    if ( memoryToFree < 1 )
        return;

    const int currentViewportPage = (*m_viewportIterator).pageNumber;

    // Create a QMap of visible rects, indexed by page number
    QMap< int, VisiblePageRect * > visibleRects;
    QVector< Okular::VisiblePageRect * >::const_iterator vIt = m_pageRects.constBegin(), vEnd = m_pageRects.constEnd();
    for ( ; vIt != vEnd; ++vIt )
        visibleRects.insert( (*vIt)->pageNumber, (*vIt) );

    // Free memory starting from pages that are farthest from the current one
    int pagesFreed = 0;
    while ( memoryToFree > 0 )
    {
        AllocatedPixmap * p = searchLowestPriorityPixmap( true, true );
        if ( !p ) // No pixmap to remove
            break;

        kDebug().nospace() << "Evicting cache pixmap observer=" << p->observer << " page=" << p->page;

        // m_allocatedPixmapsTotalMemory can't underflow because we always add or remove
        // the memory used by the AllocatedPixmap so at most it can reach zero
        m_allocatedPixmapsTotalMemory -= p->memory;
        // Make sure memoryToFree does not underflow
        if ( p->memory > memoryToFree )
            memoryToFree = 0;
        else
            memoryToFree -= p->memory;
        pagesFreed++;
        // delete pixmap
        m_pagesVector.at( p->page )->deletePixmap( p->observer );
        // delete allocation descriptor
        delete p;
    }

    // If we're still on low memory, try to free individual tiles

    // Store pages that weren't completely removed

    QLinkedList< AllocatedPixmap * > pixmapsToKeep;
    while (memoryToFree > 0)
    {
        int clean_hits = 0;
        foreach (DocumentObserver *observer, m_observers)
        {
            AllocatedPixmap * p = searchLowestPriorityPixmap( false, true, observer );
            if ( !p ) // No pixmap to remove
                continue;

            clean_hits++;

            TilesManager *tilesManager = m_pagesVector.at( p->page )->d->tilesManager( observer );
            if ( tilesManager && tilesManager->totalMemory() > 0 )
            {
                qulonglong memoryDiff = p->memory;
                NormalizedRect visibleRect;
                if ( visibleRects.contains( p->page ) )
                    visibleRect = visibleRects[ p->page ]->rect;

                // Free non visible tiles
                tilesManager->cleanupPixmapMemory( memoryToFree, visibleRect, currentViewportPage );

                p->memory = tilesManager->totalMemory();
                memoryDiff -= p->memory;
                memoryToFree = (memoryDiff < memoryToFree) ? (memoryToFree - memoryDiff) : 0;
                m_allocatedPixmapsTotalMemory -= memoryDiff;

                if ( p->memory > 0 )
                    pixmapsToKeep.append( p );
                else
                    delete p;
            }
            else
                pixmapsToKeep.append( p );
        }

        if (clean_hits == 0) break;
    }

    m_allocatedPixmaps += pixmapsToKeep;
    //p--rintf("freeMemory A:[%d -%d = %d] \n", m_allocatedPixmaps.count() + pagesFreed, pagesFreed, m_allocatedPixmaps.count() );
}

/* Returns the next pixmap to evict from cache, or NULL if no suitable pixmap
 * if found. If unloadableOnly is set, only unloadable pixmaps are returned. If
 * thenRemoveIt is set, the pixmap is removed from m_allocatedPixmaps before
 * returning it
 */
AllocatedPixmap * DocumentPrivate::searchLowestPriorityPixmap( bool unloadableOnly, bool thenRemoveIt, DocumentObserver *observer )
{
    QLinkedList< AllocatedPixmap * >::iterator pIt = m_allocatedPixmaps.begin();
    QLinkedList< AllocatedPixmap * >::iterator pEnd = m_allocatedPixmaps.end();
    QLinkedList< AllocatedPixmap * >::iterator farthestPixmap = pEnd;
    const int currentViewportPage = (*m_viewportIterator).pageNumber;

    /* Find the pixmap that is farthest from the current viewport */
    int maxDistance = -1;
    while ( pIt != pEnd )
    {
        const AllocatedPixmap * p = *pIt;
        // Filter by observer
        if ( observer == 0 || p->observer == observer )
        {
            const int distance = qAbs( p->page - currentViewportPage );
            if ( maxDistance < distance && ( !unloadableOnly || p->observer->canUnloadPixmap( p->page ) ) )
            {
                maxDistance = distance;
                farthestPixmap = pIt;
            }
        }
        ++pIt;
    }

    /* No pixmap to remove */
    if ( farthestPixmap == pEnd )
        return 0;

    AllocatedPixmap * selectedPixmap = *farthestPixmap;
    if ( thenRemoveIt )
        m_allocatedPixmaps.erase( farthestPixmap );
    return selectedPixmap;
}

qulonglong DocumentPrivate::getTotalMemory()
{
    static qulonglong cachedValue = 0;
    if ( cachedValue )
        return cachedValue;

#if defined(Q_OS_LINUX)
    // if /proc/meminfo doesn't exist, return 128MB
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( QIODevice::ReadOnly ) )
        return (cachedValue = 134217728);

    QTextStream readStream( &memFile );
    while ( true )
    {
        QString entry = readStream.readLine();
        if ( entry.isNull() ) break;
        if ( entry.startsWith( "MemTotal:" ) )
            return (cachedValue = (Q_UINT64_C(1024) * entry.section( ' ', -2, -2 ).toULongLong()));
    }
#elif defined(Q_OS_FREEBSD)
    qulonglong physmem;
    int mib[] = {CTL_HW, HW_PHYSMEM};
    size_t len = sizeof( physmem );
    if ( sysctl( mib, 2, &physmem, &len, NULL, 0 ) == 0 )
        return (cachedValue = physmem);
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
    GlobalMemoryStatusEx (&stat);

    return ( cachedValue = stat.ullTotalPhys );
#endif
    return (cachedValue = 134217728);
}

qulonglong DocumentPrivate::getFreeMemory( qulonglong *freeSwap )
{
    static QTime lastUpdate = QTime::currentTime().addSecs(-3);
    static qulonglong cachedValue = 0;
    static qulonglong cachedFreeSwap = 0;

    if ( qAbs( lastUpdate.secsTo( QTime::currentTime() ) ) <= 2 )
    {
        if (freeSwap)
            *freeSwap = cachedFreeSwap;
        return cachedValue;
    }

    /* Initialize the returned free swap value to 0. It is overwritten if the
     * actual value is available */
    if (freeSwap)
        *freeSwap = 0;

#if defined(Q_OS_LINUX)
    // if /proc/meminfo doesn't exist, return MEMORY FULL
    QFile memFile( "/proc/meminfo" );
    if ( !memFile.open( QIODevice::ReadOnly ) )
        return 0;

    // read /proc/meminfo and sum up the contents of 'MemFree', 'Buffers'
    // and 'Cached' fields. consider swapped memory as used memory.
    qulonglong memoryFree = 0;
    QString entry;
    QTextStream readStream( &memFile );
    static const int nElems = 5;
    QString names[nElems] = { "MemFree:", "Buffers:", "Cached:", "SwapFree:", "SwapTotal:" };
    qulonglong values[nElems] = { 0, 0, 0, 0, 0 };
    bool foundValues[nElems] = { false, false, false, false, false };
    while ( true )
    {
        entry = readStream.readLine();
        if ( entry.isNull() ) break;
        for ( int i = 0; i < nElems; ++i )
        {
            if ( entry.startsWith( names[i] ) )
            {
                values[i] = entry.section( ' ', -2, -2 ).toULongLong( &foundValues[i] );
            }
        }
    }
    memFile.close();
    bool found = true;
    for ( int i = 0; found && i < nElems; ++i )
        found = found && foundValues[i];
    if ( found )
    {
        /* MemFree + Buffers + Cached - SwapUsed =
         * = MemFree + Buffers + Cached - (SwapTotal - SwapFree) =
         * = MemFree + Buffers + Cached + SwapFree - SwapTotal */
        memoryFree = values[0] + values[1] + values[2] + values[3];
        if ( values[4] > memoryFree )
            memoryFree = 0;
        else
            memoryFree -= values[4];
    }
    else
    {
        return 0;
    }

    lastUpdate = QTime::currentTime();

    if (freeSwap)
        *freeSwap = ( cachedFreeSwap = (Q_UINT64_C(1024) * values[3]) );
    return ( cachedValue = (Q_UINT64_C(1024) * memoryFree) );
#elif defined(Q_OS_FREEBSD)
    qulonglong cache, inact, free, psize;
    size_t cachelen, inactlen, freelen, psizelen;
    cachelen = sizeof( cache );
    inactlen = sizeof( inact );
    freelen = sizeof( free );
    psizelen = sizeof( psize );
    // sum up inactive, cached and free memory
    if ( sysctlbyname( "vm.stats.vm.v_cache_count", &cache, &cachelen, NULL, 0 ) == 0 &&
            sysctlbyname( "vm.stats.vm.v_inactive_count", &inact, &inactlen, NULL, 0 ) == 0 &&
            sysctlbyname( "vm.stats.vm.v_free_count", &free, &freelen, NULL, 0 ) == 0 &&
            sysctlbyname( "vm.stats.vm.v_page_size", &psize, &psizelen, NULL, 0 ) == 0 )
    {
        lastUpdate = QTime::currentTime();
        return (cachedValue = (cache + inact + free) * psize);
    }
    else
    {
        return 0;
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
    GlobalMemoryStatusEx (&stat);

    lastUpdate = QTime::currentTime();

    if (freeSwap)
        *freeSwap = ( cachedFreeSwap = stat.ullAvailPageFile );
    return ( cachedValue = stat.ullAvailPhys );
#else
    // tell the memory is full.. will act as in LOW profile
    return 0;
#endif
}

void DocumentPrivate::loadDocumentInfo()
// note: load data and stores it internally (document or pages). observers
// are still uninitialized at this point so don't access them
{
    //kDebug(OkularDebug).nospace() << "Using '" << d->m_xmlFileName << "' as document info file.";
    if ( m_xmlFileName.isEmpty() )
        return;

    loadDocumentInfo( m_xmlFileName );
}

void DocumentPrivate::loadDocumentInfo( const QString &fileName )
{
    QFile infoFile( fileName );
    if ( !infoFile.exists() || !infoFile.open( QIODevice::ReadOnly ) )
        return;

    // Load DOM from XML file
    QDomDocument doc( "documentInfo" );
    if ( !doc.setContent( &infoFile ) )
    {
        kDebug(OkularDebug) << "Can't load XML pair! Check for broken xml.";
        infoFile.close();
        return;
    }
    infoFile.close();

    QDomElement root = doc.documentElement();
    if ( root.tagName() != "documentInfo" )
        return;

    KUrl documentUrl( root.attribute( "url" ) );

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
                else if ( infoElement.tagName() == "rotation" )
                {
                    QString str = infoElement.text();
                    bool ok = true;
                    int newrotation = !str.isEmpty() ? ( str.toInt( &ok ) % 4 ) : 0;
                    if ( ok && newrotation != 0 )
                    {
                        setRotationInternal( newrotation, false );
                    }
                }
                else if ( infoElement.tagName() == "views" )
                {
                    QDomNode viewNode = infoNode.firstChild();
                    while ( viewNode.isElement() )
                    {
                        QDomElement viewElement = viewNode.toElement();
                        if ( viewElement.tagName() == "view" )
                        {
                            const QString viewName = viewElement.attribute( "name" );
                            Q_FOREACH ( View * view, m_views )
                            {
                                if ( view->name() == viewName )
                                {
                                    loadViewsInfo( view, viewElement );
                                    break;
                                }
                            }
                        }
                        viewNode = viewNode.nextSibling();
                    }
                }
                infoNode = infoNode.nextSibling();
            }
        }

        topLevelNode = topLevelNode.nextSibling();
    } // </documentInfo>
}

void DocumentPrivate::loadViewsInfo( View *view, const QDomElement &e )
{
    QDomNode viewNode = e.firstChild();
    while ( viewNode.isElement() )
    {
        QDomElement viewElement = viewNode.toElement();

        if ( viewElement.tagName() == "zoom" )
        {
            const QString valueString = viewElement.attribute( "value" );
            bool newzoom_ok = true;
            const double newzoom = !valueString.isEmpty() ? valueString.toDouble( &newzoom_ok ) : 1.0;
            if ( newzoom_ok && newzoom != 0
                 && view->supportsCapability( View::Zoom )
                 && ( view->capabilityFlags( View::Zoom ) & ( View::CapabilityRead | View::CapabilitySerializable ) ) )
            {
                view->setCapability( View::Zoom, newzoom );
            }
            const QString modeString = viewElement.attribute( "mode" );
            bool newmode_ok = true;
            const int newmode = !modeString.isEmpty() ? modeString.toInt( &newmode_ok ) : 2;
            if ( newmode_ok
                 && view->supportsCapability( View::ZoomModality )
                 && ( view->capabilityFlags( View::ZoomModality ) & ( View::CapabilityRead | View::CapabilitySerializable ) ) )
            {
                view->setCapability( View::ZoomModality, newmode );
            }
        }

        viewNode = viewNode.nextSibling();
    }
}

void DocumentPrivate::saveViewsInfo( View *view, QDomElement &e ) const
{
    if ( view->supportsCapability( View::Zoom )
         && ( view->capabilityFlags( View::Zoom ) & ( View::CapabilityRead | View::CapabilitySerializable ) )
         && view->supportsCapability( View::ZoomModality )
         && ( view->capabilityFlags( View::ZoomModality ) & ( View::CapabilityRead | View::CapabilitySerializable ) ) )
    {
        QDomElement zoomEl = e.ownerDocument().createElement( "zoom" );
        e.appendChild( zoomEl );
        bool ok = true;
        const double zoom = view->capability( View::Zoom ).toDouble( &ok );
        if ( ok && zoom != 0 )
        {
            zoomEl.setAttribute( "value", QString::number(zoom) );
        }
        const int mode = view->capability( View::ZoomModality ).toInt( &ok );
        if ( ok )
        {
            zoomEl.setAttribute( "mode", mode );
        }
    }
}

QString DocumentPrivate::giveAbsolutePath( const QString & fileName ) const
{
    if ( !QDir::isRelativePath( fileName ) )
        return fileName;

    if ( !m_url.isValid() )
        return QString();

    return m_url.upUrl().url() + fileName;
}

bool DocumentPrivate::openRelativeFile( const QString & fileName )
{
    QString absFileName = giveAbsolutePath( fileName );
    if ( absFileName.isEmpty() )
        return false;

    kDebug(OkularDebug).nospace() << "openDocument: '" << absFileName << "'";

    emit m_parent->openUrl( absFileName );
    return true;
}

Generator * DocumentPrivate::loadGeneratorLibrary( const KService::Ptr &service )
{
    KPluginFactory *factory = KPluginLoader( service->library() ).factory();
    if ( !factory )
    {
        kWarning(OkularDebug).nospace() << "Invalid plugin factory for " << service->library() << "!";
        return 0;
    }
    Generator * generator = factory->create< Okular::Generator >( service->pluginKeyword(), 0 );
    GeneratorInfo info( factory->componentData() );
    info.generator = generator;
    if ( info.data.isValid() && info.data.aboutData() )
        info.catalogName = info.data.aboutData()->catalogName();
    m_loadedGenerators.insert( service->name(), info );
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
        if ( !m_loadedGenerators.isEmpty() && genIt != m_loadedGenerators.constEnd() )
            continue;

        Generator * g = loadGeneratorLibrary( offers.at(i) );
        (void)g;
    }
}

void DocumentPrivate::unloadGenerator( const GeneratorInfo& info )
{
    delete info.generator;
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

ConfigInterface* DocumentPrivate::generatorConfig( GeneratorInfo& info )
{
    if ( info.configChecked )
        return info.config;

    info.config = qobject_cast< Okular::ConfigInterface * >( info.generator );
    info.configChecked = true;
    return info.config;
}

SaveInterface* DocumentPrivate::generatorSave( GeneratorInfo& info )
{
    if ( info.saveChecked )
        return info.save;

    info.save = qobject_cast< Okular::SaveInterface * >( info.generator );
    info.saveChecked = true;
    return info.save;
}

bool DocumentPrivate::openDocumentInternal( const KService::Ptr& offer, bool isstdin, const QString& docFile, const QByteArray& filedata )
{
    QString propName = offer->name();
    QHash< QString, GeneratorInfo >::const_iterator genIt = m_loadedGenerators.constFind( propName );
    QString catalogName;
    if ( genIt != m_loadedGenerators.constEnd() )
    {
        m_generator = genIt.value().generator;
        catalogName = genIt.value().catalogName;
    }
    else
    {
        m_generator = loadGeneratorLibrary( offer );
        if ( !m_generator )
            return false;
        genIt = m_loadedGenerators.constFind( propName );
        Q_ASSERT( genIt != m_loadedGenerators.constEnd() );
        catalogName = genIt.value().catalogName;
    }
    Q_ASSERT_X( m_generator, "Document::load()", "null generator?!" );

    if ( !catalogName.isEmpty() )
        KGlobal::locale()->insertCatalog( catalogName );

    m_generator->d_func()->m_document = this;

    // connect error reporting signals
    QObject::connect( m_generator, SIGNAL(error(QString,int)), m_parent, SIGNAL(error(QString,int)) );
    QObject::connect( m_generator, SIGNAL(warning(QString,int)), m_parent, SIGNAL(warning(QString,int)) );
    QObject::connect( m_generator, SIGNAL(notice(QString,int)), m_parent, SIGNAL(notice(QString,int)) );

    QApplication::setOverrideCursor( Qt::WaitCursor );

    m_generator->setDPI(Utils::realDpi(m_widget));

    bool openOk = false;
    if ( !isstdin )
    {
        openOk = m_generator->loadDocument( docFile, m_pagesVector );
    }
    else if ( !filedata.isEmpty() )
    {
        if ( m_generator->hasFeature( Generator::ReadRawData ) )
        {
            openOk = m_generator->loadDocumentFromData( filedata, m_pagesVector );
        }
        else
        {
            m_tempFile = new KTemporaryFile();
            if ( !m_tempFile->open() )
            {
                delete m_tempFile;
                m_tempFile = 0;
            }
            else
            {
                m_tempFile->write( filedata );
                QString tmpFileName = m_tempFile->fileName();
                m_tempFile->close();
                openOk = m_generator->loadDocument( tmpFileName, m_pagesVector );
            }
        }
    }

    QApplication::restoreOverrideCursor();
    if ( !openOk || m_pagesVector.size() <= 0 )
    {
        if ( !catalogName.isEmpty() )
            KGlobal::locale()->removeCatalog( catalogName );

        m_generator->d_func()->m_document = 0;
        QObject::disconnect( m_generator, 0, m_parent, 0 );
        m_generator = 0;

        qDeleteAll( m_pagesVector );
        m_pagesVector.clear();
        delete m_tempFile;
        m_tempFile = 0;

        // TODO: emit a message telling the document is empty

        openOk = false;
    }

    return openOk;
}

bool DocumentPrivate::savePageDocumentInfo( KTemporaryFile *infoFile, int what ) const
{
    if ( infoFile->open() )
    {
        // 1. Create DOM
        QDomDocument doc( "documentInfo" );
        QDomProcessingInstruction xmlPi = doc.createProcessingInstruction(
                QString::fromLatin1( "xml" ), QString::fromLatin1( "version=\"1.0\" encoding=\"utf-8\"" ) );
        doc.appendChild( xmlPi );
        QDomElement root = doc.createElement( "documentInfo" );
        doc.appendChild( root );

        // 2.1. Save page attributes (bookmark state, annotations, ... ) to DOM
        QDomElement pageList = doc.createElement( "pageList" );
        root.appendChild( pageList );
        // <page list><page number='x'>.... </page> save pages that hold data
        QVector< Page * >::const_iterator pIt = m_pagesVector.constBegin(), pEnd = m_pagesVector.constEnd();
        for ( ; pIt != pEnd; ++pIt )
            (*pIt)->d->saveLocalContents( pageList, doc, PageItems( what ) );

        // 3. Save DOM to XML file
        QString xml = doc.toString();
        QTextStream os( infoFile );
        os.setCodec( "UTF-8" );
        os << xml;
        return true;
    }
    return false;
}

DocumentViewport DocumentPrivate::nextDocumentViewport() const
{
    DocumentViewport ret = m_nextDocumentViewport;
    if ( !m_nextDocumentDestination.isEmpty() && m_generator )
    {
        DocumentViewport vp( m_generator->metaData( "NamedViewport", m_nextDocumentDestination ).toString() );
        if ( vp.isValid() )
        {
            ret = vp;
        }
    }
    return ret;
}

void DocumentPrivate::warnLimitedAnnotSupport()
{
    if ( !m_showWarningLimitedAnnotSupport )
        return;
    m_showWarningLimitedAnnotSupport = false; // Show the warning once

    if ( m_annotationsNeedSaveAs )
    {
        // Shown if the user is editing annotations in a file whose metadata is
        // not stored locally (.okular archives belong to this category)
        KMessageBox::information( m_parent->widget(), i18n("Your annotation changes will not be saved automatically. Use File -> Save As...\nor your changes will be lost once the document is closed"), QString(), "annotNeedSaveAs" );
    }
    else if ( !canAddAnnotationsNatively() )
    {
        // If the generator doesn't support native annotations
        KMessageBox::information( m_parent->widget(), i18n("Your annotations are saved internally by Okular.\nYou can export the annotated document using File -> Export As -> Document Archive"), QString(), "annotExportAsArchive" );
    }
}

void DocumentPrivate::performAddPageAnnotation( int page, Annotation * annotation )
{
    Okular::SaveInterface * iface = qobject_cast< Okular::SaveInterface * >( m_generator );
    AnnotationProxy *proxy = iface ? iface->annotationProxy() : 0;

    // find out the page to attach annotation
    Page * kp = m_pagesVector[ page ];
    if ( !m_generator || !kp )
        return;

    // the annotation belongs already to a page
    if ( annotation->d_ptr->m_page )
        return;

    // add annotation to the page
    kp->addAnnotation( annotation );

    // tell the annotation proxy
    if ( proxy && proxy->supports(AnnotationProxy::Addition) )
        proxy->notifyAddition( annotation, page );

    // notify observers about the change
    notifyAnnotationChanges( page );

    if ( annotation->flags() & Annotation::ExternallyDrawn )
    {
        // Redraw everything, including ExternallyDrawn annotations
        refreshPixmaps( page );
    }

    warnLimitedAnnotSupport();
}

void DocumentPrivate::performRemovePageAnnotation( int page, Annotation * annotation )
{
    Okular::SaveInterface * iface = qobject_cast< Okular::SaveInterface * >( m_generator );
    AnnotationProxy *proxy = iface ? iface->annotationProxy() : 0;
    bool isExternallyDrawn;

    // find out the page
    Page * kp = m_pagesVector[ page ];
    if ( !m_generator || !kp )
        return;

    if ( annotation->flags() & Annotation::ExternallyDrawn )
        isExternallyDrawn = true;
    else
        isExternallyDrawn = false;

    // try to remove the annotation
    if ( m_parent->canRemovePageAnnotation( annotation ) )
    {
        // tell the annotation proxy
        if ( proxy && proxy->supports(AnnotationProxy::Removal) )
            proxy->notifyRemoval( annotation, page );

        kp->removeAnnotation( annotation ); // Also destroys the object

        // in case of success, notify observers about the change
        notifyAnnotationChanges( page );

        if ( isExternallyDrawn )
        {
            // Redraw everything, including ExternallyDrawn annotations
            refreshPixmaps( page );
        }
    }

    warnLimitedAnnotSupport();
}

void DocumentPrivate::performModifyPageAnnotation( int page, Annotation * annotation, bool appearanceChanged )
{
    Okular::SaveInterface * iface = qobject_cast< Okular::SaveInterface * >( m_generator );
    AnnotationProxy *proxy = iface ? iface->annotationProxy() : 0;

    // find out the page
    Page * kp = m_pagesVector[ page ];
    if ( !m_generator || !kp )
        return;

    // tell the annotation proxy
    if ( proxy && proxy->supports(AnnotationProxy::Modification) )
    {
        proxy->notifyModification( annotation, page, appearanceChanged );
    }

    // notify observers about the change
    notifyAnnotationChanges( page );
    if ( appearanceChanged && (annotation->flags() & Annotation::ExternallyDrawn) )
    {
        /* When an annotation is being moved, the generator will not render it.
         * Therefore there's no need to refresh pixmaps after the first time */
        if ( annotation->flags() & Annotation::BeingMoved )
        {
            if ( m_annotationBeingMoved )
                return;
            else // First time: take note
                m_annotationBeingMoved = true;
        }
        else
        {
            m_annotationBeingMoved = false;
        }

        // Redraw everything, including ExternallyDrawn annotations
        kDebug(OkularDebug) << "Refreshing Pixmaps";
        refreshPixmaps( page );
    }

    // If the user is moving the annotation, don't steal the focus
    if ( (annotation->flags() & Annotation::BeingMoved) == 0 )
        warnLimitedAnnotSupport();
}

void DocumentPrivate::performSetAnnotationContents( const QString & newContents, Annotation *annot, int pageNumber )
{
    bool appearanceChanged = false;

    // Check if appearanceChanged should be true
    switch ( annot->subType() )
    {
        // If it's an in-place TextAnnotation, set the inplace text
        case Okular::Annotation::AText:
        {
            Okular::TextAnnotation * txtann = static_cast< Okular::TextAnnotation * >( annot );
            if ( txtann->textType() == Okular::TextAnnotation::InPlace )
            {
                appearanceChanged = true;
            }
            break;
        }
        // If it's a LineAnnotation, check if caption text is visible
        case Okular::Annotation::ALine:
        {
            Okular::LineAnnotation * lineann = static_cast< Okular::LineAnnotation * >( annot );
            if ( lineann->showCaption() )
                appearanceChanged = true;
            break;
        }
        default:
            break;
    }

    // Set contents
    annot->setContents( newContents );

    // Tell the document the annotation has been modified
    performModifyPageAnnotation( pageNumber,  annot, appearanceChanged );
}

void DocumentPrivate::saveDocumentInfo() const
{
    if ( m_xmlFileName.isEmpty() )
        return;

    QFile infoFile( m_xmlFileName );
    if (infoFile.open( QIODevice::WriteOnly | QIODevice::Truncate) )
    {
        // 1. Create DOM
        QDomDocument doc( "documentInfo" );
        QDomProcessingInstruction xmlPi = doc.createProcessingInstruction(
                QString::fromLatin1( "xml" ), QString::fromLatin1( "version=\"1.0\" encoding=\"utf-8\"" ) );
        doc.appendChild( xmlPi );
        QDomElement root = doc.createElement( "documentInfo" );
        root.setAttribute( "url", m_url.pathOrUrl() );
        doc.appendChild( root );

        // 2.1. Save page attributes (bookmark state, annotations, ... ) to DOM
        QDomElement pageList = doc.createElement( "pageList" );
        root.appendChild( pageList );
        PageItems saveWhat = AllPageItems;
        if ( m_annotationsNeedSaveAs )
        {
            /* In this case, if the user makes a modification, he's requested to
             * save to a new document. Therefore, if there are existing local
             * annotations, we save them back unmodified in the original
             * document's metadata, so that it appears that it was not changed */
            saveWhat |= OriginalAnnotationPageItems;
        }
        // <page list><page number='x'>.... </page> save pages that hold data
        QVector< Page * >::const_iterator pIt = m_pagesVector.constBegin(), pEnd = m_pagesVector.constEnd();
        for ( ; pIt != pEnd; ++pIt )
            (*pIt)->d->saveLocalContents( pageList, doc, saveWhat );

        // 2.2. Save document info (current viewport, history, ... ) to DOM
        QDomElement generalInfo = doc.createElement( "generalInfo" );
        root.appendChild( generalInfo );
        // create rotation node
        if ( m_rotation != Rotation0 )
        {
            QDomElement rotationNode = doc.createElement( "rotation" );
            generalInfo.appendChild( rotationNode );
            rotationNode.appendChild( doc.createTextNode( QString::number( (int)m_rotation ) ) );
        }
        // <general info><history> ... </history> save history up to OKULAR_HISTORY_SAVEDSTEPS viewports
        QLinkedList< DocumentViewport >::const_iterator backIterator = m_viewportIterator;
        if ( backIterator != m_viewportHistory.constEnd() )
        {
            // go back up to OKULAR_HISTORY_SAVEDSTEPS steps from the current viewportIterator
            int backSteps = OKULAR_HISTORY_SAVEDSTEPS;
            while ( backSteps-- && backIterator != m_viewportHistory.constBegin() )
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
        // create views root node
        QDomElement viewsNode = doc.createElement( "views" );
        generalInfo.appendChild( viewsNode );
        Q_FOREACH ( View * view, m_views )
        {
            QDomElement viewEntry = doc.createElement( "view" );
            viewEntry.setAttribute( "name", view->name() );
            viewsNode.appendChild( viewEntry );
            saveViewsInfo( view, viewEntry );
        }

        // 3. Save DOM to XML file
        QString xml = doc.toString();
        QTextStream os( &infoFile );
        os.setCodec( "UTF-8" );
        os << xml;
    }
    infoFile.close();
}

void DocumentPrivate::slotTimedMemoryCheck()
{
    // [MEM] clean memory (for 'free mem dependant' profiles only)
    if ( SettingsCore::memoryLevel() != SettingsCore::EnumMemoryLevel::Low &&
         m_allocatedPixmapsTotalMemory > 1024*1024 )
        cleanupPixmapMemory();
}

void DocumentPrivate::sendGeneratorPixmapRequest()
{
    /* If the pixmap cache will have to be cleaned in order to make room for the
     * next request, get the distance from the current viewport of the page
     * whose pixmap will be removed. We will ignore preload requests for pages
     * that are at the same distance or farther */
    const qulonglong memoryToFree = calculateMemoryToFree();
    const int currentViewportPage = (*m_viewportIterator).pageNumber;
    int maxDistance = INT_MAX; // Default: No maximum
    if ( memoryToFree )
    {
        AllocatedPixmap *pixmapToReplace = searchLowestPriorityPixmap( true );
        if ( pixmapToReplace )
            maxDistance = qAbs( pixmapToReplace->page - currentViewportPage );
    }

    // find a request
    PixmapRequest * request = 0;
    m_pixmapRequestsMutex.lock();
    while ( !m_pixmapRequestsStack.isEmpty() && !request )
    {
        PixmapRequest * r = m_pixmapRequestsStack.last();
        if (!r)
        {
            m_pixmapRequestsStack.pop_back();
            continue;
        }

        QRect requestRect = r->isTile() ? r->normalizedRect().geometry( r->width(), r->height() ) : QRect( 0, 0, r->width(), r->height() );
        TilesManager *tilesManager = r->d->tilesManager();

        // If it's a preload but the generator is not threaded no point in trying to preload
        if ( r->preload() && !m_generator->hasFeature( Generator::Threaded ) )
        {
            m_pixmapRequestsStack.pop_back();
            delete r;
        }
        // request only if page isn't already present and request has valid id
        // request only if page isn't already present and request has valid id
        else if ( ( !r->d->mForce && r->page()->hasPixmap( r->observer(), r->width(), r->height(), r->normalizedRect() ) ) || !m_observers.contains(r->observer()) )
        {
            m_pixmapRequestsStack.pop_back();
            delete r;
        }
        else if ( !r->d->mForce && r->preload() && qAbs( r->pageNumber() - currentViewportPage ) >= maxDistance )
        {
            m_pixmapRequestsStack.pop_back();
            //kDebug() << "Ignoring request that doesn't fit in cache";
            delete r;
        }
        // Ignore requests for pixmaps that are already being generated
        else if ( tilesManager && tilesManager->isRequesting( r->normalizedRect(), r->width(), r->height() ) )
        {
            m_pixmapRequestsStack.pop_back();
            delete r;
        }
        // If the requested area is above 8000000 pixels, switch on the tile manager
        else if ( !tilesManager && m_generator->hasFeature( Generator::TiledRendering ) && (long)r->width() * (long)r->height() > 8000000L )
        {
            // if the image is too big. start using tiles
            kDebug(OkularDebug).nospace() << "Start using tiles on page " << r->pageNumber()
                << " (" << r->width() << "x" << r->height() << " px);";

            // fill the tiles manager with the last rendered pixmap
            const QPixmap *pixmap = r->page()->_o_nearestPixmap( r->observer(), r->width(), r->height() );
            if ( pixmap )
            {
                tilesManager = new TilesManager( r->pageNumber(), pixmap->width(), pixmap->height(), r->page()->rotation() );
                tilesManager->setPixmap( pixmap, NormalizedRect( 0, 0, 1, 1 ) );
                tilesManager->setSize( r->width(), r->height() );
            }
            else
            {
                // create new tiles manager
                tilesManager = new TilesManager( r->pageNumber(), r->width(), r->height(), r->page()->rotation() );
            }
            tilesManager->setRequest( r->normalizedRect(), r->width(), r->height() );
            r->page()->deletePixmap( r->observer() );
            r->page()->d->setTilesManager( r->observer(), tilesManager );
            r->setTile( true );

            // Change normalizedRect to the smallest rect that contains all
            // visible tiles.
            if ( !r->normalizedRect().isNull() )
            {
                NormalizedRect tilesRect;
                const QList<Tile> tiles = tilesManager->tilesAt( r->normalizedRect(), TilesManager::TerminalTile );
                QList<Tile>::const_iterator tIt = tiles.constBegin(), tEnd = tiles.constEnd();
                while ( tIt != tEnd )
                {
                    Tile tile = *tIt;
                    if ( tilesRect.isNull() )
                        tilesRect = tile.rect();
                    else
                        tilesRect |= tile.rect();

                    ++tIt;
                }

                r->setNormalizedRect( tilesRect );
                request = r;
            }
            else
            {
                // Discard request if normalizedRect is null. This happens in
                // preload requests issued by PageView if the requested page is
                // not visible and the user has just switched from a non-tiled
                // zoom level to a tiled one
                m_pixmapRequestsStack.pop_back();
                delete r;
            }
        }
        // If the requested area is below 6000000 pixels, switch off the tile manager
        else if ( tilesManager && (long)r->width() * (long)r->height() < 6000000L )
        {
            kDebug(OkularDebug).nospace() << "Stop using tiles on page " << r->pageNumber()
                << " (" << r->width() << "x" << r->height() << " px);";

            // page is too small. stop using tiles.
            r->page()->deletePixmap( r->observer() );
            r->setTile( false );

            request = r;
        }
        else if ( (long)requestRect.width() * (long)requestRect.height() > 20000000L )
        {
            m_pixmapRequestsStack.pop_back();
            if ( !m_warnedOutOfMemory )
            {
                kWarning(OkularDebug).nospace() << "Running out of memory on page " << r->pageNumber()
                    << " (" << r->width() << "x" << r->height() << " px);";
                kWarning(OkularDebug) << "this message will be reported only once.";
                m_warnedOutOfMemory = true;
            }
            delete r;
        }
        else
        {
            request = r;
        }
    }

    // if no request found (or already generated), return
    if ( !request )
    {
        m_pixmapRequestsMutex.unlock();
        return;
    }

    // [MEM] preventive memory freeing
    qulonglong pixmapBytes = 0;
    TilesManager * tm = request->d->tilesManager();
    if ( tm )
        pixmapBytes = tm->totalMemory();
    else
        pixmapBytes = 4 * request->width() * request->height();

    if ( pixmapBytes > (1024 * 1024) )
        cleanupPixmapMemory( memoryToFree /* previously calculated value */ );

    // submit the request to the generator
    if ( m_generator->canGeneratePixmap() )
    {
        QRect requestRect = !request->isTile() ? QRect(0, 0, request->width(), request->height() ) : request->normalizedRect().geometry( request->width(), request->height() );
        kDebug(OkularDebug).nospace() << "sending request observer=" << request->observer() << " " <<requestRect.width() << "x" << requestRect.height() << "@" << request->pageNumber() << " async == " << request->asynchronous() << " isTile == " << request->isTile();
        m_pixmapRequestsStack.removeAll ( request );

        if ( tm )
            tm->setRequest( request->normalizedRect(), request->width(), request->height() );

        if ( (int)m_rotation % 2 )
            request->d->swap();

        if ( m_rotation != Rotation0 && !request->normalizedRect().isNull() )
            request->setNormalizedRect( TilesManager::fromRotatedRect(
                        request->normalizedRect(), m_rotation ) );

        // we always have to unlock _before_ the generatePixmap() because
        // a sync generation would end with requestDone() -> deadlock, and
        // we can not really know if the generator can do async requests
        m_executingPixmapRequests.push_back( request );
        m_pixmapRequestsMutex.unlock();
        m_generator->generatePixmap( request );
    }
    else
    {
        m_pixmapRequestsMutex.unlock();
        // pino (7/4/2006): set the polling interval from 10 to 30
        QTimer::singleShot( 30, m_parent, SLOT(sendGeneratorPixmapRequest()) );
    }
}

void DocumentPrivate::rotationFinished( int page, Okular::Page *okularPage )
{
    Okular::Page *wantedPage = m_pagesVector.value( page, 0 );
    if ( !wantedPage || wantedPage != okularPage )
        return;

    foreach(DocumentObserver *o, m_observers)
        o->notifyPageChanged( page, DocumentObserver::Pixmap | DocumentObserver::Annotations );
}

void DocumentPrivate::fontReadingProgress( int page )
{
    emit m_parent->fontReadingProgress( page );

    if ( page >= (int)m_parent->pages() - 1 )
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
    QHash< QString, GeneratorInfo >::iterator it = m_loadedGenerators.begin(), itEnd = m_loadedGenerators.end();
    for ( ; it != itEnd; ++it )
    {
        Okular::ConfigInterface * iface = generatorConfig( it.value() );
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
        QVector<Page*>::const_iterator it = m_pagesVector.constBegin(), end = m_pagesVector.constEnd();
        for ( ; it != end; ++it ) {
            (*it)->deletePixmaps();
        }

        // [MEM] remove allocation descriptors
        qDeleteAll( m_allocatedPixmaps );
        m_allocatedPixmaps.clear();
        m_allocatedPixmapsTotalMemory = 0;

        // send reload signals to observers
        foreachObserverD( notifyContentsCleared( DocumentObserver::Pixmap ) );
    }

    // free memory if in 'low' profile
    if ( SettingsCore::memoryLevel() == SettingsCore::EnumMemoryLevel::Low &&
         !m_allocatedPixmaps.isEmpty() && !m_pagesVector.isEmpty() )
        cleanupPixmapMemory();
}

void DocumentPrivate::refreshPixmaps( int pageNumber )
{
    Page* page = m_pagesVector.value( pageNumber, 0 );
    if ( !page )
        return;

    QLinkedList< Okular::PixmapRequest * > requestedPixmaps;
    QMap< DocumentObserver*, PagePrivate::PixmapObject >::ConstIterator it = page->d->m_pixmaps.constBegin(), itEnd = page->d->m_pixmaps.constEnd();
    for ( ; it != itEnd; ++it )
    {
        QSize size = (*it).m_pixmap->size();
        PixmapRequest * p = new PixmapRequest( it.key(), pageNumber, size.width(), size.height(), 1, PixmapRequest::Asynchronous );
        p->d->mForce = true;
        requestedPixmaps.push_back( p );
    }

    foreach (DocumentObserver *observer, m_observers)
    {
        TilesManager *tilesManager = page->d->tilesManager( observer );
        if ( tilesManager )
        {
            tilesManager->markDirty();

            PixmapRequest * p = new PixmapRequest( observer, pageNumber, tilesManager->width(), tilesManager->height(), 1, PixmapRequest::Asynchronous );

            NormalizedRect tilesRect;

            // Get the visible page rect
            NormalizedRect visibleRect;
            QVector< Okular::VisiblePageRect * >::const_iterator vIt = m_pageRects.constBegin(), vEnd = m_pageRects.constEnd();
            for ( ; vIt != vEnd; ++vIt )
            {
                if ( (*vIt)->pageNumber == pageNumber )
                {
                    visibleRect = (*vIt)->rect;
                    break;
                }
            }

            if ( !visibleRect.isNull() )
            {
                p->setNormalizedRect( visibleRect );
                p->setTile( true );
                p->d->mForce = true;
                requestedPixmaps.push_back( p );
            }
            else
            {
                delete p;
            }
        }
    }

    if ( !requestedPixmaps.isEmpty() )
        m_parent->requestPixmaps( requestedPixmaps, Okular::Document::NoOption );
}

void DocumentPrivate::_o_configChanged()
{
    // free text pages if needed
    calculateMaxTextPages();
    while (m_allocatedTextPagesFifo.count() > m_maxAllocatedTextPages)
    {
        int pageToKick = m_allocatedTextPagesFifo.takeFirst();
        m_pagesVector.at(pageToKick)->setTextPage( 0 ); // deletes the textpage
    }
}

void DocumentPrivate::doContinueDirectionMatchSearch(void *doContinueDirectionMatchSearchStruct)
{
    DoContinueDirectionMatchSearchStruct *searchStruct = static_cast<DoContinueDirectionMatchSearchStruct *>(doContinueDirectionMatchSearchStruct);
    RunningSearch *search = m_searches.value(searchStruct->searchID);

    if ((m_searchCancelled && !searchStruct->match) || !search)
    {
        // if the user cancelled but he just got a match, give him the match!
        QApplication::restoreOverrideCursor();

        if (search) search->isCurrentlySearching = false;

        emit m_parent->searchFinished( searchStruct->searchID, Document::SearchCancelled );
        delete searchStruct->pagesToNotify;
        delete searchStruct;
        return;
    }

    bool doContinue = false;
    // if no match found, loop through the whole doc, starting from currentPage
    if ( !searchStruct->match )
    {
        const int pageCount = m_pagesVector.count();
        if (searchStruct->pagesDone < pageCount)
        {
            doContinue = true;
            if ( searchStruct->currentPage >= pageCount || searchStruct->currentPage < 0 )
            {
                const QString question = searchStruct->forward ? i18n("End of document reached.\nContinue from the beginning?") : i18n("Beginning of document reached.\nContinue from the bottom?");
                if ( searchStruct->noDialogs || KMessageBox::questionYesNo(m_parent->widget(), question, QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel()) == KMessageBox::Yes )
                    searchStruct->currentPage = searchStruct->forward ? 0 : pageCount - 1;
                else
                    doContinue = false;
            }
        }
    }

    if (doContinue)
    {
        // get page
        Page * page = m_pagesVector[ searchStruct->currentPage ];
        // request search page if needed
        if ( !page->hasTextPage() )
            m_parent->requestTextPage( page->number() );

        // if found a match on the current page, end the loop
        searchStruct->match = page->findText( searchStruct->searchID, searchStruct->text, searchStruct->forward ? FromTop : FromBottom, searchStruct->caseSensitivity );

        if ( !searchStruct->match )
        {
            if (searchStruct->forward) searchStruct->currentPage++;
            else searchStruct->currentPage--;
            searchStruct->pagesDone++;
        }
        else
        {
            searchStruct->pagesDone = 1;
        }

        // Both of the previous if branches need to call doContinueDirectionMatchSearch
        QMetaObject::invokeMethod(m_parent, "doContinueDirectionMatchSearch", Qt::QueuedConnection, Q_ARG(void *, searchStruct));
    }
    else
    {
        doProcessSearchMatch( searchStruct->match, search, searchStruct->pagesToNotify, searchStruct->currentPage, searchStruct->searchID, searchStruct->moveViewport, searchStruct->color );
        delete searchStruct;
    }
}

void DocumentPrivate::doProcessSearchMatch( RegularAreaRect *match, RunningSearch *search, QSet< int > *pagesToNotify, int currentPage, int searchID, bool moveViewport, const QColor & color )
{
    // reset cursor to previous shape
    QApplication::restoreOverrideCursor();

    bool foundAMatch = false;

    search->isCurrentlySearching = false;

    // if a match has been found..
    if ( match )
    {
        // update the RunningSearch structure adding this match..
        foundAMatch = true;
        search->continueOnPage = currentPage;
        search->continueOnMatch = *match;
        search->highlightedPages.insert( currentPage );
        // ..add highlight to the page..
        m_pagesVector[ currentPage ]->d->setHighlight( searchID, match, color );

        // ..queue page for notifying changes..
        pagesToNotify->insert( currentPage );

        // Create a normalized rectangle around the search match that includes a 5% buffer on all sides.
        const Okular::NormalizedRect matchRectWithBuffer = Okular::NormalizedRect( match->first().left - 0.05,
                                                                                   match->first().top - 0.05,
                                                                                   match->first().right + 0.05,
                                                                                   match->first().bottom + 0.05 );

        const bool matchRectFullyVisible = isNormalizedRectangleFullyVisible( matchRectWithBuffer, currentPage );

        // ..move the viewport to show the first of the searched word sequence centered
        if ( moveViewport && !matchRectFullyVisible )
        {
            DocumentViewport searchViewport( currentPage );
            searchViewport.rePos.enabled = true;
            searchViewport.rePos.normalizedX = (match->first().left + match->first().right) / 2.0;
            searchViewport.rePos.normalizedY = (match->first().top + match->first().bottom) / 2.0;
            m_parent->setViewport( searchViewport, 0, true );
        }
        delete match;
    }
#if 0
    else if ( !noDialogs )
        KMessageBox::information( m_parent->widget(), i18n( "No matches found for '%1'.", text ) );
#endif

    // notify observers about highlights changes
    foreach(int pageNumber, *pagesToNotify)
        foreach(DocumentObserver *observer, m_observers)
            observer->notifyPageChanged( pageNumber, DocumentObserver::Highlights );

    if (foundAMatch) emit m_parent->searchFinished( searchID, Document::MatchFound );
    else emit m_parent->searchFinished( searchID, Document::NoMatchFound );

    delete pagesToNotify;
}

void DocumentPrivate::doContinueAllDocumentSearch(void *pagesToNotifySet, void *pageMatchesMap, int currentPage, int searchID, const QString & text, int theCaseSensitivity, const QColor & color)
{
    QMap< Page *, QVector<RegularAreaRect *> > *pageMatches = static_cast< QMap< Page *, QVector<RegularAreaRect *> > * >(pageMatchesMap);
    Qt::CaseSensitivity caseSensitivity = static_cast<Qt::CaseSensitivity>(theCaseSensitivity);
    QSet< int > *pagesToNotify = static_cast< QSet< int > * >( pagesToNotifySet );
    RunningSearch *search = m_searches.value(searchID);

    if (m_searchCancelled || !search)
    {
        typedef QVector<RegularAreaRect *> MatchesVector;

        QApplication::restoreOverrideCursor();

        if (search) search->isCurrentlySearching = false;

        emit m_parent->searchFinished( searchID, Document::SearchCancelled );
        foreach(const MatchesVector &mv, *pageMatches) qDeleteAll(mv);
        delete pageMatches;
        delete pagesToNotify;
        return;
    }

    if (currentPage < m_pagesVector.count())
    {
        // get page (from the first to the last)
        Page *page = m_pagesVector.at(currentPage);
        int pageNumber = page->number(); // redundant? is it == currentPage ?

        // request search page if needed
        if ( !page->hasTextPage() )
            m_parent->requestTextPage( pageNumber );

        // loop on a page adding highlights for all found items
        RegularAreaRect * lastMatch = 0;
        while ( 1 )
        {
            if ( lastMatch )
                lastMatch = page->findText( searchID, text, NextResult, caseSensitivity, lastMatch );
            else
                lastMatch = page->findText( searchID, text, FromTop, caseSensitivity );

            if ( !lastMatch )
                break;

            // add highligh rect to the matches map
            (*pageMatches)[page].append(lastMatch);
        }
        delete lastMatch;

        QMetaObject::invokeMethod(m_parent, "doContinueAllDocumentSearch", Qt::QueuedConnection, Q_ARG(void *, pagesToNotifySet), Q_ARG(void *, pageMatches), Q_ARG(int, currentPage + 1), Q_ARG(int, searchID), Q_ARG(QString, text), Q_ARG(int, caseSensitivity), Q_ARG(QColor, color));
    }
    else
    {
        // reset cursor to previous shape
        QApplication::restoreOverrideCursor();

        search->isCurrentlySearching = false;
        bool foundAMatch = pageMatches->count() != 0;
        QMap< Page *, QVector<RegularAreaRect *> >::const_iterator it, itEnd;
        it = pageMatches->constBegin();
        itEnd = pageMatches->constEnd();
        for ( ; it != itEnd; ++it)
        {
            foreach(RegularAreaRect *match, it.value())
            {
                it.key()->d->setHighlight( searchID, match, color );
                delete match;
            }
            search->highlightedPages.insert( it.key()->number() );
            pagesToNotify->insert( it.key()->number() );
        }

        foreach(DocumentObserver *observer, m_observers)
            observer->notifySetup( m_pagesVector, 0 );

        // notify observers about highlights changes
        foreach(int pageNumber, *pagesToNotify)
            foreach(DocumentObserver *observer, m_observers)
                observer->notifyPageChanged( pageNumber, DocumentObserver::Highlights );

        if (foundAMatch) emit m_parent->searchFinished(searchID, Document::MatchFound );
        else emit m_parent->searchFinished( searchID, Document::NoMatchFound );

        delete pageMatches;
        delete pagesToNotify;
    }
}

void DocumentPrivate::doContinueGooglesDocumentSearch(void *pagesToNotifySet, void *pageMatchesMap, int currentPage, int searchID, const QStringList & words, int theCaseSensitivity, const QColor & color, bool matchAll)
{
    typedef QPair<RegularAreaRect *, QColor> MatchColor;
    QMap< Page *, QVector<MatchColor> > *pageMatches = static_cast< QMap< Page *, QVector<MatchColor> > * >(pageMatchesMap);
    Qt::CaseSensitivity caseSensitivity = static_cast<Qt::CaseSensitivity>(theCaseSensitivity);
    QSet< int > *pagesToNotify = static_cast< QSet< int > * >( pagesToNotifySet );
    RunningSearch *search = m_searches.value(searchID);

    if (m_searchCancelled || !search)
    {
        typedef QVector<MatchColor> MatchesVector;

        QApplication::restoreOverrideCursor();

        if (search) search->isCurrentlySearching = false;

        emit m_parent->searchFinished( searchID, Document::SearchCancelled );

        foreach(const MatchesVector &mv, *pageMatches)
        {
            foreach(const MatchColor &mc, mv) delete mc.first;
        }
        delete pageMatches;
        delete pagesToNotify;
        return;
    }

    const int wordCount = words.count();
    const int hueStep = (wordCount > 1) ? (60 / (wordCount - 1)) : 60;
    int baseHue, baseSat, baseVal;
    color.getHsv( &baseHue, &baseSat, &baseVal );

    if (currentPage < m_pagesVector.count())
    {
        // get page (from the first to the last)
        Page *page = m_pagesVector.at(currentPage);
        int pageNumber = page->number(); // redundant? is it == currentPage ?

        // request search page if needed
        if ( !page->hasTextPage() )
            m_parent->requestTextPage( pageNumber );

        // loop on a page adding highlights for all found items
        bool allMatched = wordCount > 0,
             anyMatched = false;
        for ( int w = 0; w < wordCount; w++ )
        {
            const QString &word = words[ w ];
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

                // add highligh rect to the matches map
                (*pageMatches)[page].append(MatchColor(lastMatch, wordColor));
                wordMatched = true;
            }
            allMatched = allMatched && wordMatched;
            anyMatched = anyMatched || wordMatched;
        }

        // if not all words are present in page, remove partial highlights
        if ( !allMatched && matchAll )
        {
            QVector<MatchColor> &matches = (*pageMatches)[page];
            foreach(const MatchColor &mc, matches) delete mc.first;
            pageMatches->remove(page);
        }

        QMetaObject::invokeMethod(m_parent, "doContinueGooglesDocumentSearch", Qt::QueuedConnection, Q_ARG(void *, pagesToNotifySet), Q_ARG(void *, pageMatches), Q_ARG(int, currentPage + 1), Q_ARG(int, searchID), Q_ARG(QStringList, words), Q_ARG(int, caseSensitivity), Q_ARG(QColor, color), Q_ARG(bool, matchAll));
    }
    else
    {
        // reset cursor to previous shape
        QApplication::restoreOverrideCursor();

        search->isCurrentlySearching = false;
        bool foundAMatch = pageMatches->count() != 0;
        QMap< Page *, QVector<MatchColor> >::const_iterator it, itEnd;
        it = pageMatches->constBegin();
        itEnd = pageMatches->constEnd();
        for ( ; it != itEnd; ++it)
        {
            foreach(const MatchColor &mc, it.value())
            {
                it.key()->d->setHighlight( searchID, mc.first, mc.second );
                delete mc.first;
            }
            search->highlightedPages.insert( it.key()->number() );
            pagesToNotify->insert( it.key()->number() );
        }

        // send page lists to update observers (since some filter on bookmarks)
        foreach(DocumentObserver *observer, m_observers)
            observer->notifySetup( m_pagesVector, 0 );

        // notify observers about highlights changes
        foreach(int pageNumber, *pagesToNotify)
            foreach(DocumentObserver *observer, m_observers)
                observer->notifyPageChanged( pageNumber, DocumentObserver::Highlights );

        if (foundAMatch) emit m_parent->searchFinished( searchID, Document::MatchFound );
        else emit m_parent->searchFinished( searchID, Document::NoMatchFound );

        delete pageMatches;
        delete pagesToNotify;
    }
}

QVariant DocumentPrivate::documentMetaData( const QString &key, const QVariant &option ) const
{
    if ( key == QLatin1String( "PaperColor" ) )
    {
        bool giveDefault = option.toBool();
        // load paper color from Settings, or use the default color (white)
        // if we were told to do so
        QColor color;
        if ( ( SettingsCore::renderMode() == SettingsCore::EnumRenderMode::Paper )
             && SettingsCore::changeColors() )
        {
            color = SettingsCore::paperColor();
        }
        else if ( giveDefault )
        {
            color = Qt::white;
        }
        return color;
    }
    else if ( key == QLatin1String( "TextAntialias" ) )
    {
        switch ( SettingsCore::textAntialias() )
        {
            case SettingsCore::EnumTextAntialias::Enabled:
                return true;
                break;
#if 0
            case Settings::EnumTextAntialias::UseKDESettings:
                // TODO: read the KDE configuration
                return true;
                break;
#endif
            case SettingsCore::EnumTextAntialias::Disabled:
                return false;
                break;
        }
    }
    else if ( key == QLatin1String( "GraphicsAntialias" ) )
    {
        switch ( SettingsCore::graphicsAntialias() )
        {
            case SettingsCore::EnumGraphicsAntialias::Enabled:
                return true;
                break;
            case SettingsCore::EnumGraphicsAntialias::Disabled:
                return false;
                break;
        }
    }
    else if ( key == QLatin1String( "TextHinting" ) )
    {
        switch ( SettingsCore::textHinting() )
        {
            case SettingsCore::EnumTextHinting::Enabled:
                return true;
                break;
            case SettingsCore::EnumTextHinting::Disabled:
                return false;
                break;
        }
    }
    return QVariant();
}

bool DocumentPrivate::isNormalizedRectangleFullyVisible( const Okular::NormalizedRect & rectOfInterest, int rectPage )
{
    bool rectFullyVisible = false;
    const QVector<Okular::VisiblePageRect *> & visibleRects = m_parent->visiblePageRects();
    QVector<Okular::VisiblePageRect *>::const_iterator vEnd = visibleRects.end();
    QVector<Okular::VisiblePageRect *>::const_iterator vIt = visibleRects.begin();

    for ( ; ( vIt != vEnd ) && !rectFullyVisible; ++vIt )
    {
        if ( (*vIt)->pageNumber == rectPage &&
            (*vIt)->rect.contains( rectOfInterest.left, rectOfInterest.top ) &&
            (*vIt)->rect.contains( rectOfInterest.right, rectOfInterest.bottom ) )
        {
            rectFullyVisible = true;
        }
    }
    return rectFullyVisible;
}

Document::Document( QWidget *widget )
    : QObject( 0 ), d( new DocumentPrivate( this ) )
{
    d->m_widget = widget;
    d->m_bookmarkManager = new BookmarkManager( d );
    d->m_viewportIterator = d->m_viewportHistory.insert( d->m_viewportHistory.end(), DocumentViewport() );
    d->m_undoStack = new QUndoStack(this);

    connect( SettingsCore::self(), SIGNAL(configChanged()), this, SLOT(_o_configChanged()) );
    connect( d->m_undoStack, SIGNAL( canUndoChanged(bool) ), this, SIGNAL( canUndoChanged(bool)));
    connect( d->m_undoStack, SIGNAL( canRedoChanged(bool) ), this, SIGNAL( canRedoChanged(bool) ) );

    qRegisterMetaType<Okular::FontInfo>();
}

Document::~Document()
{
    // delete generator, pages, and related stuff
    closeDocument();

    QSet< View * >::const_iterator viewIt = d->m_views.constBegin(), viewEnd = d->m_views.constEnd();
    for ( ; viewIt != viewEnd; ++viewIt )
    {
        View *v = *viewIt;
        v->d_func()->document = 0;
    }

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

class kMimeTypeMoreThan {
public:
    kMimeTypeMoreThan( const KMimeType::Ptr &mime ) : _mime( mime ) {}
    bool operator()( const KService::Ptr &s1, const KService::Ptr &s2 )
    {
        const QString mimeName = _mime->name();
        if (s1->mimeTypes().contains( mimeName ) && !s2->mimeTypes().contains( mimeName ))
            return true;
        else if (s2->mimeTypes().contains( mimeName ) && !s1->mimeTypes().contains( mimeName ))
            return false;
        return s1->property( "X-KDE-Priority" ).toInt() > s2->property( "X-KDE-Priority" ).toInt();
    }

private:
    const KMimeType::Ptr &_mime;
};

bool Document::openDocument( const QString & docFile, const KUrl& url, const KMimeType::Ptr &_mime )
{
    KMimeType::Ptr mime = _mime;
    QByteArray filedata;
    qint64 document_size = -1;
    bool isstdin = url.fileName( KUrl::ObeyTrailingSlash ) == QLatin1String( "-" );
    bool triedMimeFromFileContent = false;
    if ( !isstdin )
    {
        if ( mime.count() <= 0 )
            return false;

        // docFile is always local so we can use QFileInfo on it
        QFileInfo fileReadTest( docFile );
        if ( fileReadTest.isFile() && !fileReadTest.isReadable() )
        {
            d->m_docFileName.clear();
            return false;
        }
        // determine the related "xml document-info" filename
        d->m_url = url;
        d->m_docFileName = docFile;
        if ( url.isLocalFile() && !d->m_archiveData )
        {
            QString fn = url.fileName();
            document_size = fileReadTest.size();
            fn = QString::number( document_size ) + '.' + fn + ".xml";
            QString newokular = "okular/docdata/" + fn;
            QString newokularfile = KStandardDirs::locateLocal( "data", newokular );
            if ( !QFile::exists( newokularfile ) )
            {
                QString oldkpdf = "kpdf/" + fn;
                QString oldkpdffile = KStandardDirs::locateLocal( "data", oldkpdf );
                if ( QFile::exists( oldkpdffile ) )
                {
                    // ### copy or move?
                    if ( !QFile::copy( oldkpdffile, newokularfile ) )
                        return false;
                }
            }
            d->m_xmlFileName = newokularfile;
        }
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
        triedMimeFromFileContent = true;
    }

    // 0. load Generator
    // request only valid non-disabled plugins suitable for the mimetype
    QString constraint("([X-KDE-Priority] > 0) and (exist Library)") ;
    KService::List offers = KMimeTypeTrader::self()->query(mime->name(),"okular/Generator",constraint);
    if ( offers.isEmpty() && !triedMimeFromFileContent )
    {
        KMimeType::Ptr newmime = KMimeType::findByFileContent( docFile );
        triedMimeFromFileContent = true;
        if ( newmime->name() != mime->name() )
        {
            mime = newmime;
            offers = KMimeTypeTrader::self()->query( mime->name(), "okular/Generator", constraint );
        }
        if ( offers.isEmpty() )
        {
            // There's still no offers, do a final mime search based on the filename
            // We need this because sometimes (e.g. when downloading from a webserver) the mimetype we
            // use is the one fed by the server, that may be wrong
            newmime = KMimeType::findByUrl( docFile );
            if ( newmime->name() != mime->name() )
            {
                mime = newmime;
                offers = KMimeTypeTrader::self()->query( mime->name(), "okular/Generator", constraint );
            }
        }
    }
    if (offers.isEmpty())
    {
        emit error( i18n( "Can not find a plugin which is able to handle the document being passed." ), -1 );
        kWarning(OkularDebug).nospace() << "No plugin for mimetype '" << mime->name() << "'.";
        return false;
    }
    int hRank=0;
    // best ranked offer search
    int offercount = offers.count();
    if ( offercount > 1 )
    {
        // sort the offers: the offers with an higher priority come before
        qStableSort( offers.begin(), offers.end(), kMimeTypeMoreThan( mime ) );

        if ( SettingsCore::chooseGenerators() )
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

    KService::Ptr offer = offers.at( hRank );
    // 1. load Document
    bool openOk = d->openDocumentInternal( offer, isstdin, docFile, filedata );
    if ( !openOk && !triedMimeFromFileContent )
    {
        KMimeType::Ptr newmime = KMimeType::findByFileContent( docFile );
        triedMimeFromFileContent = true;
        if ( newmime->name() != mime->name() )
        {
            mime = newmime;
            offers = KMimeTypeTrader::self()->query( mime->name(), "okular/Generator", constraint );
            if ( !offers.isEmpty() )
            {
                offer = offers.first();
                openOk = d->openDocumentInternal( offer, isstdin, docFile, filedata );
            }
        }
    }
    if ( !openOk )
    {
        return false;
    }

    d->m_generatorName = offer->name();
    d->m_pageController = new PageController();
    connect( d->m_pageController, SIGNAL(rotationFinished(int,Okular::Page*)),
             this, SLOT(rotationFinished(int,Okular::Page*)) );

    bool containsExternalAnnotations = false;
    foreach ( Page * p, d->m_pagesVector )
    {
        p->d->m_doc = d;
        if ( !p->annotations().empty() )
            containsExternalAnnotations = true;
    }

    // Be quiet while restoring local annotations
    d->m_showWarningLimitedAnnotSupport = false;
    d->m_annotationsNeedSaveAs = false;

    // 2. load Additional Data (bookmarks, local annotations and metadata) about the document
    if ( d->m_archiveData )
    {
        d->loadDocumentInfo( d->m_archiveData->metadataFileName );
        d->m_annotationsNeedSaveAs = true;
    }
    else
    {
        d->loadDocumentInfo();
        d->m_annotationsNeedSaveAs = ( d->canAddAnnotationsNatively() && containsExternalAnnotations );
    }

    d->m_showWarningLimitedAnnotSupport = true;
    d->m_bookmarkManager->setUrl( d->m_url );

    // 3. setup observers inernal lists and data
    foreachObserver( notifySetup( d->m_pagesVector, DocumentObserver::DocumentChanged ) );

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
        connect( d->m_saveBookmarksTimer, SIGNAL(timeout()), this, SLOT(saveDocumentInfo()) );
    }
    d->m_saveBookmarksTimer->start( 5 * 60 * 1000 );

    // start memory check timer
    if ( !d->m_memCheckTimer )
    {
        d->m_memCheckTimer = new QTimer( this );
        connect( d->m_memCheckTimer, SIGNAL(timeout()), this, SLOT(slotTimedMemoryCheck()) );
    }
    d->m_memCheckTimer->start( 2000 );

    const DocumentViewport nextViewport = d->nextDocumentViewport();
    if ( nextViewport.isValid() )
    {
        setViewport( nextViewport );
        d->m_nextDocumentViewport = DocumentViewport();
        d->m_nextDocumentDestination = QString();
    }

    AudioPlayer::instance()->d->m_currentDocument = isstdin ? KUrl() : d->m_url;
    d->m_docSize = document_size;

    const QStringList docScripts = d->m_generator->metaData( "DocumentScripts", "JavaScript" ).toStringList();
    if ( !docScripts.isEmpty() )
    {
        d->m_scripter = new Scripter( d );
        Q_FOREACH ( const QString &docscript, docScripts )
        {
            d->m_scripter->execute( JavaScript, docscript );
        }
    }

    return true;
}


KXMLGUIClient* Document::guiClient()
{
    if ( d->m_generator )
    {
        Okular::GuiInterface * iface = qobject_cast< Okular::GuiInterface * >( d->m_generator );
        if ( iface )
            return iface->guiClient();
    }
    return 0;
}

void Document::closeDocument()
{
    // check if there's anything to close...
    if ( !d->m_generator )
        return;

    delete d->m_pageController;
    d->m_pageController = 0;

    delete d->m_scripter;
    d->m_scripter = 0;

     // remove requests left in queue
    d->m_pixmapRequestsMutex.lock();
    QLinkedList< PixmapRequest * >::const_iterator sIt = d->m_pixmapRequestsStack.constBegin();
    QLinkedList< PixmapRequest * >::const_iterator sEnd = d->m_pixmapRequestsStack.constEnd();
    for ( ; sIt != sEnd; ++sIt )
        delete *sIt;
    d->m_pixmapRequestsStack.clear();
    d->m_pixmapRequestsMutex.unlock();

    QEventLoop loop;
    bool startEventLoop = false;
    do
    {
        d->m_pixmapRequestsMutex.lock();
        startEventLoop = !d->m_executingPixmapRequests.isEmpty();
        d->m_pixmapRequestsMutex.unlock();
        if ( startEventLoop )
        {
            d->m_closingLoop = &loop;
            loop.exec();
            d->m_closingLoop = 0;
        }
    }
    while ( startEventLoop );

    if ( d->m_fontThread )
    {
        disconnect( d->m_fontThread, 0, this, 0 );
        d->m_fontThread->stopExtraction();
        d->m_fontThread->wait();
        d->m_fontThread = 0;
    }

    // stop any audio playback
    AudioPlayer::instance()->stopPlaybacks();

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
        // disconnect the generator from this document ...
        d->m_generator->d_func()->m_document = 0;
        // .. and this document from the generator signals
        disconnect( d->m_generator, 0, this, 0 );

        QHash< QString, GeneratorInfo >::const_iterator genIt = d->m_loadedGenerators.constFind( d->m_generatorName );
        Q_ASSERT( genIt != d->m_loadedGenerators.constEnd() );
        if ( !genIt.value().catalogName.isEmpty() && !genIt.value().config )
            KGlobal::locale()->removeCatalog( genIt.value().catalogName );
    }
    d->m_generator = 0;
    d->m_generatorName = QString();
    d->m_url = KUrl();
    d->m_docFileName = QString();
    d->m_xmlFileName = QString();
    delete d->m_tempFile;
    d->m_tempFile = 0;
    delete d->m_archiveData;
    d->m_archiveData = 0;
    d->m_docSize = -1;
    d->m_exportCached = false;
    d->m_exportFormats.clear();
    d->m_exportToText = ExportFormat();
    d->m_fontsCached = false;
    d->m_fontsCache.clear();
    d->m_rotation = Rotation0;

    // send an empty list to observers (to free their data)
    foreachObserver( notifySetup( QVector< Page * >(), DocumentObserver::DocumentChanged ) );

    // delete pages and clear 'd->m_pagesVector' container
    QVector< Page * >::const_iterator pIt = d->m_pagesVector.constBegin();
    QVector< Page * >::const_iterator pEnd = d->m_pagesVector.constEnd();
    for ( ; pIt != pEnd; ++pIt )
        delete *pIt;
    d->m_pagesVector.clear();

    // clear 'memory allocation' descriptors
    qDeleteAll( d->m_allocatedPixmaps );
    d->m_allocatedPixmaps.clear();

    // clear 'running searches' descriptors
    QMap< int, RunningSearch * >::const_iterator rIt = d->m_searches.constBegin();
    QMap< int, RunningSearch * >::const_iterator rEnd = d->m_searches.constEnd();
    for ( ; rIt != rEnd; ++rIt )
        delete *rIt;
    d->m_searches.clear();

    // clear the visible areas and notify the observers
    QVector< VisiblePageRect * >::const_iterator vIt = d->m_pageRects.constBegin();
    QVector< VisiblePageRect * >::const_iterator vEnd = d->m_pageRects.constEnd();
    for ( ; vIt != vEnd; ++vIt )
        delete *vIt;
    d->m_pageRects.clear();
    foreachObserver( notifyVisibleRectsChanged() );

    // reset internal variables

    d->m_viewportHistory.clear();
    d->m_viewportHistory.append( DocumentViewport() );
    d->m_viewportIterator = d->m_viewportHistory.begin();
    d->m_allocatedPixmapsTotalMemory = 0;
    d->m_allocatedTextPagesFifo.clear();
    d->m_pageSize = PageSize();
    d->m_pageSizes.clear();

    delete d->m_documentInfo;
    d->m_documentInfo = 0;

    AudioPlayer::instance()->d->m_currentDocument = KUrl();

    d->m_undoStack->clear();
}

void Document::addObserver( DocumentObserver * pObserver )
{
    Q_ASSERT( !d->m_observers.contains( pObserver ) );
    d->m_observers << pObserver;

    // if the observer is added while a document is already opened, tell it
    if ( !d->m_pagesVector.isEmpty() )
    {
        pObserver->notifySetup( d->m_pagesVector, DocumentObserver::DocumentChanged );
        pObserver->notifyViewportChanged( false /*disables smoothMove*/ );
    }
}

void Document::removeObserver( DocumentObserver * pObserver )
{
    // remove observer from the map. it won't receive notifications anymore
    if ( d->m_observers.contains( pObserver ) )
    {
        // free observer's pixmap data
        QVector<Page*>::const_iterator it = d->m_pagesVector.constBegin(), end = d->m_pagesVector.constEnd();
        for ( ; it != end; ++it )
            (*it)->deletePixmap( pObserver );

        // [MEM] free observer's allocation descriptors
        QLinkedList< AllocatedPixmap * >::iterator aIt = d->m_allocatedPixmaps.begin();
        QLinkedList< AllocatedPixmap * >::iterator aEnd = d->m_allocatedPixmaps.end();
        while ( aIt != aEnd )
        {
            AllocatedPixmap * p = *aIt;
            if ( p->observer == pObserver )
            {
                aIt = d->m_allocatedPixmaps.erase( aIt );
                delete p;
            }
            else
                ++aIt;
        }

        // delete observer entry from the map
        d->m_observers.remove( pObserver );
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
        QVector<Page*>::const_iterator it = d->m_pagesVector.constBegin(), end = d->m_pagesVector.constEnd();
        for ( ; it != end; ++it ) {
            (*it)->deletePixmaps();
        }

        // [MEM] remove allocation descriptors
        qDeleteAll( d->m_allocatedPixmaps );
        d->m_allocatedPixmaps.clear();
        d->m_allocatedPixmapsTotalMemory = 0;

        // send reload signals to observers
        foreachObserver( notifyContentsCleared( DocumentObserver::Pixmap ) );
    }

    // free memory if in 'low' profile
    if ( SettingsCore::memoryLevel() == SettingsCore::EnumMemoryLevel::Low &&
         !d->m_allocatedPixmaps.isEmpty() && !d->m_pagesVector.isEmpty() )
        d->cleanupPixmapMemory();
}


QWidget *Document::widget() const
{
    return d->m_widget;
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
    if ( d->m_documentInfo )
        return d->m_documentInfo;

    if ( d->m_generator )
    {
        DocumentInfo *info = new DocumentInfo();
        const DocumentInfo *tmp = d->m_generator->generateDocumentInfo();
        if ( tmp )
            *info = *tmp;

        info->set( DocumentInfo::FilePath, currentDocument().prettyUrl() );
        const QString pagesSize = d->pagesSizeString();
        if ( d->m_docSize != -1 )
        {
            const QString sizeString = KGlobal::locale()->formatByteSize( d->m_docSize );
            info->set( DocumentInfo::DocumentSize, sizeString );
        }
        if (!pagesSize.isEmpty())
        {
            info->set( DocumentInfo::PagesSize, pagesSize );
        }

        const DocumentInfo::Key keyPages = DocumentInfo::Pages;
        const QString keyString = DocumentInfo::getKeyString( keyPages );

        if ( info->get( keyString ).isEmpty() ) {
            info->set( keyString, QString::number( this->pages() ),
                       DocumentInfo::getKeyTitle( keyPages ) );
        }

        d->m_documentInfo = info;
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
    connect( d->m_fontThread, SIGNAL(gotFont(Okular::FontInfo)), this, SLOT(fontReadingGotFont(Okular::FontInfo)) );
    connect( d->m_fontThread, SIGNAL(progress(int)), this, SLOT(fontReadingProgress(int)) );

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

void Document::setVisiblePageRects( const QVector< VisiblePageRect * > & visiblePageRects, DocumentObserver *excludeObserver )
{
    QVector< VisiblePageRect * >::const_iterator vIt = d->m_pageRects.constBegin();
    QVector< VisiblePageRect * >::const_iterator vEnd = d->m_pageRects.constEnd();
    for ( ; vIt != vEnd; ++vIt )
        delete *vIt;
    d->m_pageRects = visiblePageRects;
    // notify change to all other (different from id) observers
    foreach(DocumentObserver *o, d->m_observers)
        if ( o != excludeObserver )
            o->notifyVisibleRectsChanged();
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
    if ( action == Okular::AllowNotes && !d->m_annotationEditingEnabled )
        return false;

#if !OKULAR_FORCE_DRM
    if ( KAuthorized::authorize( "skip_drm" ) && !Okular::SettingsCore::obeyDRM() )
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

bool Document::supportsTiles() const
{
    return d->m_generator ? d->m_generator->hasFeature( Generator::TiledRendering ) : false;
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
    requestPixmaps( requests, RemoveAllPrevious );
}

void Document::requestPixmaps( const QLinkedList< PixmapRequest * > & requests, PixmapRequestFlags reqOptions )
{
    if ( requests.isEmpty() )
        return;

    if ( !d->m_generator || d->m_closingLoop )
    {
        // delete requests..
        QLinkedList< PixmapRequest * >::const_iterator rIt = requests.constBegin(), rEnd = requests.constEnd();
        for ( ; rIt != rEnd; ++rIt )
            delete *rIt;
        // ..and return
        return;
    }

    // 1. [CLEAN STACK] remove previous requests of requesterID
    // FIXME This assumes all requests come from the same observer, that is true atm but not enforced anywhere
    DocumentObserver *requesterObserver = requests.first()->observer();
    QSet< int > requestedPages;
    {
        QLinkedList< PixmapRequest * >::const_iterator rIt = requests.constBegin(), rEnd = requests.constEnd();
        for ( ; rIt != rEnd; ++rIt )
            requestedPages.insert( (*rIt)->pageNumber() );
    }
    const bool removeAllPrevious = reqOptions & RemoveAllPrevious;
    d->m_pixmapRequestsMutex.lock();
    QLinkedList< PixmapRequest * >::iterator sIt = d->m_pixmapRequestsStack.begin(), sEnd = d->m_pixmapRequestsStack.end();
    while ( sIt != sEnd )
    {
        if ( (*sIt)->observer() == requesterObserver
             && ( removeAllPrevious || requestedPages.contains( (*sIt)->pageNumber() ) ) )
        {
            // delete request and remove it from stack
            delete *sIt;
            sIt = d->m_pixmapRequestsStack.erase( sIt );
        }
        else
            ++sIt;
    }

    // 2. [ADD TO STACK] add requests to stack
    QLinkedList< PixmapRequest * >::const_iterator rIt = requests.constBegin(), rEnd = requests.constEnd();
    for ( ; rIt != rEnd; ++rIt )
    {
        // set the 'page field' (see PixmapRequest) and check if it is valid
        PixmapRequest * request = *rIt;
        kDebug(OkularDebug).nospace() << "request observer=" << request->observer() << " " <<request->width() << "x" << request->height() << "@" << request->pageNumber();
        if ( d->m_pagesVector.value( request->pageNumber() ) == 0 )
        {
            // skip requests referencing an invalid page (must not happen)
            delete request;
            continue;
        }

        request->d->mPage = d->m_pagesVector.value( request->pageNumber() );

        if ( request->isTile() )
        {
            // Change the current request rect so that only invalid tiles are
            // requested. Also make sure the rect is tile-aligned.
            NormalizedRect tilesRect;
            const QList<Tile> tiles = request->d->tilesManager()->tilesAt( request->normalizedRect(), TilesManager::TerminalTile );
            QList<Tile>::const_iterator tIt = tiles.constBegin(), tEnd = tiles.constEnd();
            while ( tIt != tEnd )
            {
                const Tile &tile = *tIt;
                if ( !tile.isValid() )
                {
                    if ( tilesRect.isNull() )
                        tilesRect = tile.rect();
                    else
                        tilesRect |= tile.rect();
                }

                tIt++;
            }

            request->setNormalizedRect( tilesRect );
        }

        if ( !request->asynchronous() )
            request->d->mPriority = 0;

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
    // all handling of requests put into sendGeneratorPixmapRequest
    //    if ( generator->canRequestPixmap() )
        d->sendGeneratorPixmapRequest();
}

void Document::requestTextPage( uint page )
{
    Page * kp = d->m_pagesVector[ page ];
    if ( !d->m_generator || !kp )
        return;

    // Memory management for TextPages

    d->m_generator->generateTextPage( kp );
}

void DocumentPrivate::notifyAnnotationChanges( int page )
{
    int flags = DocumentObserver::Annotations;

    if ( m_annotationsNeedSaveAs )
        flags |= DocumentObserver::NeedSaveAs;

    foreachObserverD( notifyPageChanged( page, flags ) );
}

void Document::addPageAnnotation( int page, Annotation * annotation )
{
    // Transform annotation's base boundary rectangle into unrotated coordinates
    Page *p = d->m_pagesVector[page];
    QTransform t = p->d->rotationMatrix();
    annotation->d_ptr->baseTransform(t.inverted());
    QUndoCommand *uc = new AddAnnotationCommand(this->d, annotation, page);
    d->m_undoStack->push(uc);
}

bool Document::canModifyPageAnnotation( const Annotation * annotation ) const
{
    if ( !annotation || ( annotation->flags() & Annotation::DenyWrite ) )
        return false;

    if ( !isAllowed(Okular::AllowNotes) )
        return false;

    if ( ( annotation->flags() & Annotation::External ) && !d->canModifyExternalAnnotations() )
        return false;

    switch ( annotation->subType() )
    {
        case Annotation::AText:
        case Annotation::ALine:
        case Annotation::AGeom:
        case Annotation::AHighlight:
        case Annotation::AStamp:
        case Annotation::AInk:
            return true;
        default:
            return false;
    }
}

void Document::prepareToModifyAnnotationProperties( Annotation * annotation )
{
    Q_ASSERT(d->m_prevPropsOfAnnotBeingModified.isNull());
    if (!d->m_prevPropsOfAnnotBeingModified.isNull())
    {
        kError(OkularDebug) << "Error: Document::prepareToModifyAnnotationProperties has already been called since last call to Document::modifyPageAnnotationProperties";
        return;
    }
    d->m_prevPropsOfAnnotBeingModified = annotation->getAnnotationPropertiesDomNode();
}

void Document::modifyPageAnnotationProperties( int page, Annotation * annotation )
{
    Q_ASSERT(!d->m_prevPropsOfAnnotBeingModified.isNull());
    if (d->m_prevPropsOfAnnotBeingModified.isNull())
    {
        kError(OkularDebug) << "Error: Document::prepareToModifyAnnotationProperties must be called before Annotation is modified";
        return;
    }
    QDomNode prevProps = d->m_prevPropsOfAnnotBeingModified;
    QUndoCommand *uc = new Okular::ModifyAnnotationPropertiesCommand( d,
                                                                      annotation,
                                                                      page,
                                                                      prevProps,
                                                                      annotation->getAnnotationPropertiesDomNode() );
    d->m_undoStack->push( uc );
    d->m_prevPropsOfAnnotBeingModified.clear();
}

void Document::translatePageAnnotation(int page, Annotation* annotation, const NormalizedPoint & delta )
{
    int complete = (annotation->flags() & Okular::Annotation::BeingMoved) == 0;
    QUndoCommand *uc = new Okular::TranslateAnnotationCommand( d, annotation, page, delta, complete );
    d->m_undoStack->push(uc);
}

void Document::editPageAnnotationContents( int page, Annotation* annotation,
                                           const QString & newContents,
                                           int newCursorPos,
                                           int prevCursorPos,
                                           int prevAnchorPos
                                         )
{
    QString prevContents = annotation->contents();
    QUndoCommand *uc = new EditAnnotationContentsCommand( d, annotation, page, newContents, newCursorPos,
                                                            prevContents, prevCursorPos, prevAnchorPos );
    d->m_undoStack->push( uc );
}

bool Document::canRemovePageAnnotation( const Annotation * annotation ) const
{
    if ( !annotation || ( annotation->flags() & Annotation::DenyDelete ) )
        return false;

    if ( ( annotation->flags() & Annotation::External ) && !d->canRemoveExternalAnnotations() )
        return false;

    switch ( annotation->subType() )
    {
        case Annotation::AText:
        case Annotation::ALine:
        case Annotation::AGeom:
        case Annotation::AHighlight:
        case Annotation::AStamp:
        case Annotation::AInk:
            return true;
        default:
            return false;
    }
}

void Document::removePageAnnotation( int page, Annotation * annotation )
{
    QUndoCommand *uc = new RemoveAnnotationCommand(this->d, annotation, page);
    d->m_undoStack->push(uc);
}

void Document::removePageAnnotations( int page, const QList<Annotation*> &annotations )
{
    d->m_undoStack->beginMacro(i18nc("remove a collection of annotations from the page", "remove annotations"));
    foreach(Annotation* annotation, annotations)
    {
        QUndoCommand *uc = new RemoveAnnotationCommand(this->d, annotation, page);
        d->m_undoStack->push(uc);
    }
    d->m_undoStack->endMacro();
}

bool DocumentPrivate::canAddAnnotationsNatively() const
{
    Okular::SaveInterface * iface = qobject_cast< Okular::SaveInterface * >( m_generator );

    if ( iface && iface->supportsOption(Okular::SaveInterface::SaveChanges) &&
         iface->annotationProxy() && iface->annotationProxy()->supports(AnnotationProxy::Addition) )
        return true;

    return false;
}

bool DocumentPrivate::canModifyExternalAnnotations() const
{
    Okular::SaveInterface * iface = qobject_cast< Okular::SaveInterface * >( m_generator );

    if ( iface && iface->supportsOption(Okular::SaveInterface::SaveChanges) &&
         iface->annotationProxy() && iface->annotationProxy()->supports(AnnotationProxy::Modification) )
        return true;

    return false;
}

bool DocumentPrivate::canRemoveExternalAnnotations() const
{
    Okular::SaveInterface * iface = qobject_cast< Okular::SaveInterface * >( m_generator );

    if ( iface && iface->supportsOption(Okular::SaveInterface::SaveChanges) &&
         iface->annotationProxy() && iface->annotationProxy()->supports(AnnotationProxy::Removal) )
        return true;

    return false;
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
        kp->d->deleteTextSelections();

    // notify observers about the change
    foreachObserver( notifyPageChanged( page, DocumentObserver::TextSelection ) );
}

bool Document::canUndo() const
{
    return d->m_undoStack->canUndo();
}

bool Document::canRedo() const
{
    return d->m_undoStack->canRedo();
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
void Document::setViewportPage( int page, DocumentObserver *excludeObserver, bool smoothMove )
{
    // clamp page in range [0 ... numPages-1]
    if ( page < 0 )
        page = 0;
    else if ( page > (int)d->m_pagesVector.count() )
        page = d->m_pagesVector.count() - 1;

    // make a viewport from the page and broadcast it
    setViewport( DocumentViewport( page ), excludeObserver, smoothMove );
}

void Document::setViewport( const DocumentViewport & viewport, DocumentObserver *excludeObserver, bool smoothMove )
{
    if ( !viewport.isValid() )
    {
        kDebug(OkularDebug) << "invalid viewport:" << viewport.toString();
        return;
    }
    if ( viewport.pageNumber >= int(d->m_pagesVector.count()) )
    {
        //kDebug(OkularDebug) << "viewport out of document:" << viewport.toString();
        return;
    }

    // if already broadcasted, don't redo it
    DocumentViewport & oldViewport = *d->m_viewportIterator;
    // disabled by enrico on 2005-03-18 (less debug output)
    //if ( viewport == oldViewport )
    //    kDebug(OkularDebug) << "setViewport with the same viewport.";

    const int oldPageNumber = oldViewport.pageNumber;

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
        if ( d->m_viewportHistory.count() >= OKULAR_HISTORY_MAXSTEPS )
            d->m_viewportHistory.pop_front();

        // add the item at the end of the queue
        d->m_viewportIterator = d->m_viewportHistory.insert( d->m_viewportHistory.end(), viewport );
    }

    const int currentViewportPage = (*d->m_viewportIterator).pageNumber;

    const bool currentPageChanged = (oldPageNumber != currentViewportPage);

    // notify change to all other (different from id) observers
    foreach(DocumentObserver *o, d->m_observers)
    {
        if ( o != excludeObserver )
            o->notifyViewportChanged( smoothMove );

        if ( currentPageChanged )
            o->notifyCurrentPageChanged( oldPageNumber, currentViewportPage );
    }
}

void Document::setZoom(int factor, DocumentObserver *excludeObserver)
{
    // notify change to all other (different from id) observers
    foreach(DocumentObserver *o, d->m_observers)
        if (o != excludeObserver)
            o->notifyZoom( factor );
}

void Document::setPrevViewport()
// restore viewport from the history
{
    if ( d->m_viewportIterator != d->m_viewportHistory.begin() )
    {
        const int oldViewportPage = (*d->m_viewportIterator).pageNumber;

        // restore previous viewport and notify it to observers
        --d->m_viewportIterator;
        foreachObserver( notifyViewportChanged( true ) );

        const int currentViewportPage = (*d->m_viewportIterator).pageNumber;
        if (oldViewportPage != currentViewportPage)
            foreachObserver( notifyCurrentPageChanged( oldViewportPage, currentViewportPage ) );
    }
}

void Document::setNextViewport()
// restore next viewport from the history
{
    QLinkedList< DocumentViewport >::const_iterator nextIterator = d->m_viewportIterator;
    ++nextIterator;
    if ( nextIterator != d->m_viewportHistory.end() )
    {
        const int oldViewportPage = (*d->m_viewportIterator).pageNumber;

        // restore next viewport and notify it to observers
        ++d->m_viewportIterator;
        foreachObserver( notifyViewportChanged( true ) );

        const int currentViewportPage = (*d->m_viewportIterator).pageNumber;
        if (oldViewportPage != currentViewportPage)
            foreachObserver( notifyCurrentPageChanged( oldViewportPage, currentViewportPage ) );
    }
}

void Document::setNextDocumentViewport( const DocumentViewport & viewport )
{
    d->m_nextDocumentViewport = viewport;
}

void Document::setNextDocumentDestination( const QString &namedDestination )
{
    d->m_nextDocumentDestination = namedDestination;
}

void Document::searchText( int searchID, const QString & text, bool fromStart, Qt::CaseSensitivity caseSensitivity,
                               SearchType type, bool moveViewport, const QColor & color, bool noDialogs )
{
    d->m_searchCancelled = false;

    // safety checks: don't perform searches on empty or unsearchable docs
    if ( !d->m_generator || !d->m_generator->hasFeature( Generator::TextExtraction ) || d->m_pagesVector.isEmpty() )
    {
        emit searchFinished( searchID, NoMatchFound );
        return;
    }

    if ( !noDialogs )
    {
#if 0
        KDialog *searchDialog = new KDialog(widget());
        searchDialog->setCaption( i18n("Search in progress...") );
        searchDialog->setButtons( KDialog::Cancel );
        QLabel *searchLabel = new QLabel(i18n("Searching for %1", text), searchDialog);
        searchDialog->setMainWidget( searchLabel );

        QTimer::singleShot(500, searchDialog, SLOT(show()));
        connect(this, SIGNAL(searchFinished(int,Okular::Document::SearchStatus)), searchDialog, SLOT(deleteLater()));
        connect(searchDialog, SIGNAL(finished()), this, SLOT(cancelSearch()));
#endif
    }

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

    // update search structure
    bool newText = text != s->cachedString;
    s->cachedString = text;
    s->cachedType = type;
    s->cachedCaseSensitivity = caseSensitivity;
    s->cachedViewportMove = moveViewport;
    s->cachedNoDialogs = noDialogs;
    s->cachedColor = color;
    s->isCurrentlySearching = true;

    // global data for search
    QSet< int > *pagesToNotify = new QSet< int >;

    // remove highlights from pages and queue them for notifying changes
    *pagesToNotify += s->highlightedPages;
    foreach(int pageNumber, s->highlightedPages)
        d->m_pagesVector.at(pageNumber)->d->deleteHighlights( searchID );
    s->highlightedPages.clear();

    // set hourglass cursor
    QApplication::setOverrideCursor( Qt::WaitCursor );

    // 1. ALLDOC - proces all document marking pages
    if ( type == AllDocument )
    {
        QMap< Page *, QVector<RegularAreaRect *> > *pageMatches = new QMap< Page *, QVector<RegularAreaRect *> >;

        // search and highlight 'text' (as a solid phrase) on all pages
        QMetaObject::invokeMethod(this, "doContinueAllDocumentSearch", Qt::QueuedConnection, Q_ARG(void *, pagesToNotify), Q_ARG(void *, pageMatches), Q_ARG(int, 0), Q_ARG(int, searchID), Q_ARG(QString, text), Q_ARG(int, caseSensitivity), Q_ARG(QColor, color));
    }
    // 2. NEXTMATCH - find next matching item (or start from top)
    // 3. PREVMATCH - find previous matching item (or start from bottom)
    else if ( type == NextMatch || type == PreviousMatch )
    {
        // find out from where to start/resume search from
        const bool forward = type == NextMatch;
        const int viewportPage = (*d->m_viewportIterator).pageNumber;
        const int fromStartSearchPage = forward ? 0 : d->m_pagesVector.count() - 1;
        int currentPage = fromStart ? fromStartSearchPage : ((s->continueOnPage != -1) ? s->continueOnPage : viewportPage);
        Page * lastPage = fromStart ? 0 : d->m_pagesVector[ currentPage ];
        int pagesDone = 0;

        // continue checking last TextPage first (if it is the current page)
        RegularAreaRect * match = 0;
        if ( lastPage && lastPage->number() == s->continueOnPage )
        {
            if ( newText )
                match = lastPage->findText( searchID, text, forward ? FromTop : FromBottom, caseSensitivity );
            else
                match = lastPage->findText( searchID, text, forward ? NextResult : PreviousResult, caseSensitivity, &s->continueOnMatch );
            if ( !match )
            {
                if (forward) currentPage++;
                else currentPage--;
                pagesDone++;
            }
        }

        DoContinueDirectionMatchSearchStruct *searchStruct = new DoContinueDirectionMatchSearchStruct();
        searchStruct->forward = forward;
        searchStruct->pagesToNotify = pagesToNotify;
        searchStruct->match = match;
        searchStruct->currentPage = currentPage;
        searchStruct->searchID = searchID;
        searchStruct->text = text;
        searchStruct->caseSensitivity = caseSensitivity;
        searchStruct->moveViewport = moveViewport;
        searchStruct->color = color;
        searchStruct->noDialogs = noDialogs;
        searchStruct->pagesDone = pagesDone;

        QMetaObject::invokeMethod(this, "doContinueDirectionMatchSearch", Qt::QueuedConnection, Q_ARG(void *, searchStruct));
    }
    // 4. GOOGLE* - process all document marking pages
    else if ( type == GoogleAll || type == GoogleAny )
    {
        bool matchAll = type == GoogleAll;

        QMap< Page *, QVector< QPair<RegularAreaRect *, QColor> > > *pageMatches = new QMap< Page *, QVector<QPair<RegularAreaRect *, QColor> > >;
        const QStringList words = text.split( ' ', QString::SkipEmptyParts );

        // search and highlight every word in 'text' on all pages
        QMetaObject::invokeMethod(this, "doContinueGooglesDocumentSearch", Qt::QueuedConnection, Q_ARG(void *, pagesToNotify), Q_ARG(void *, pageMatches), Q_ARG(int, 0), Q_ARG(int, searchID), Q_ARG(QStringList, words), Q_ARG(int, caseSensitivity), Q_ARG(QColor, color), Q_ARG(bool, matchAll));
    }
}

void Document::continueSearch( int searchID )
{
    // check if searchID is present in runningSearches
    QMap< int, RunningSearch * >::const_iterator it = d->m_searches.constFind( searchID );
    if ( it == d->m_searches.constEnd() )
    {
        emit searchFinished( searchID, NoMatchFound );
        return;
    }

    // start search with cached parameters from last search by searchID
    RunningSearch * p = *it;
    if ( !p->isCurrentlySearching )
        searchText( searchID, p->cachedString, false, p->cachedCaseSensitivity,
                    p->cachedType, p->cachedViewportMove, p->cachedColor,
                    p->cachedNoDialogs );
}

void Document::continueSearch( int searchID, SearchType type )
{
    // check if searchID is present in runningSearches
    QMap< int, RunningSearch * >::const_iterator it = d->m_searches.constFind( searchID );
    if ( it == d->m_searches.constEnd() )
    {
        emit searchFinished( searchID, NoMatchFound );
        return;
    }

    // start search with cached parameters from last search by searchID
    RunningSearch * p = *it;
    if ( !p->isCurrentlySearching )
        searchText( searchID, p->cachedString, false, p->cachedCaseSensitivity,
                    type, p->cachedViewportMove, p->cachedColor,
                    p->cachedNoDialogs );
}

void Document::resetSearch( int searchID )
{
    // if we are closing down, don't bother doing anything
    if ( !d->m_generator )
        return;

    // check if searchID is present in runningSearches
    QMap< int, RunningSearch * >::iterator searchIt = d->m_searches.find( searchID );
    if ( searchIt == d->m_searches.end() )
        return;

    // get previous parameters for search
    RunningSearch * s = *searchIt;

    // unhighlight pages and inform observers about that
    foreach(int pageNumber, s->highlightedPages)
    {
        d->m_pagesVector.at(pageNumber)->d->deleteHighlights( searchID );
        foreachObserver( notifyPageChanged( pageNumber, DocumentObserver::Highlights ) );
    }

    // send the setup signal too (to update views that filter on matches)
    foreachObserver( notifySetup( d->m_pagesVector, 0 ) );

    // remove serch from the runningSearches list and delete it
    d->m_searches.erase( searchIt );
    delete s;
}

void Document::cancelSearch()
{
    d->m_searchCancelled = true;
}

void Document::undo()
{
    d->m_undoStack->undo();
}

void Document::redo()
{
    d->m_undoStack->redo();
}

void Document::editFormText( int pageNumber,
                             Okular::FormFieldText* form,
                             const QString & newContents,
                             int newCursorPos,
                             int prevCursorPos,
                             int prevAnchorPos )
{
    QUndoCommand *uc = new EditFormTextCommand( this->d, form, pageNumber, newContents, newCursorPos, form->text(), prevCursorPos, prevAnchorPos );
    d->m_undoStack->push( uc );
}

void Document::editFormList( int pageNumber,
                             FormFieldChoice* form,
                             const QList< int > & newChoices )
{
    const QList< int > prevChoices = form->currentChoices();
    QUndoCommand *uc = new EditFormListCommand( this->d, form, pageNumber, newChoices, prevChoices );
    d->m_undoStack->push( uc );
}

void Document::editFormCombo( int pageNumber,
                              FormFieldChoice* form,
                              const QString & newText,
                              int newCursorPos,
                              int prevCursorPos,
                              int prevAnchorPos )
{

    QString prevText;
    if ( form->currentChoices().isEmpty() )
    {
        prevText = form->editChoice();
    }
    else
    {
        prevText = form->choices()[form->currentChoices()[0]];
    }

    QUndoCommand *uc = new EditFormComboCommand( this->d, form, pageNumber, newText, newCursorPos, prevText, prevCursorPos, prevAnchorPos );
    d->m_undoStack->push( uc );
}

void Document::editFormButtons( int pageNumber, const QList< FormFieldButton* >& formButtons, const QList< bool >& newButtonStates )
{
    QUndoCommand *uc = new EditFormButtonsCommand( this->d, pageNumber, formButtons, newButtonStates );
    d->m_undoStack->push( uc );
}

BookmarkManager * Document::bookmarkManager() const
{
    return d->m_bookmarkManager;
}

QList<int> Document::bookmarkedPageList() const
{
    QList<int> list;
    uint docPages = pages();

    //pages are 0-indexed internally, but 1-indexed externally
    for ( uint i = 0; i < docPages; i++ )
    {
        if ( bookmarkManager()->isBookmarked( i ) )
        {
            list << i + 1;
        }
    }
    return list;
}

QString Document::bookmarkedPageRange() const
{
    // Code formerly in Part::slotPrint()
    // range detecting
    QString range;
    uint docPages = pages();
    int startId = -1;
    int endId = -1;

    for ( uint i = 0; i < docPages; ++i )
    {
        if ( bookmarkManager()->isBookmarked( i ) )
        {
            if ( startId < 0 )
                startId = i;
            if ( endId < 0 )
                endId = startId;
            else
                ++endId;
        }
        else if ( startId >= 0 && endId >= 0 )
        {
            if ( !range.isEmpty() )
                range += ',';

            if ( endId - startId > 0 )
                range += QString( "%1-%2" ).arg( startId + 1 ).arg( endId + 1 );
            else
                range += QString::number( startId + 1 );
            startId = -1;
            endId = -1;
        }
    }
    if ( startId >= 0 && endId >= 0 )
    {
        if ( !range.isEmpty() )
            range += ',';

        if ( endId - startId > 0 )
            range += QString( "%1-%2" ).arg( startId + 1 ).arg( endId + 1 );
        else
            range += QString::number( startId + 1 );
    }
    return range;
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
            d->m_nextDocumentDestination = go->destinationName();

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
                kWarning(OkularDebug).nospace() << "Action: Error opening '" << go->fileName() << "'.";
                return;
            }
            else
            {
                const DocumentViewport nextViewport = d->nextDocumentViewport();
                // skip local links that point to nowhere (broken ones)
                if ( !nextViewport.isValid() )
                    return;

                setViewport( nextViewport, 0, true );
                d->m_nextDocumentViewport = DocumentViewport();
                d->m_nextDocumentDestination = QString();
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
                        KMessageBox::information( widget(), i18n("The document is trying to execute an external application and, for your safety, Okular does not allow that.") );
                        return;
                    }
                }
                else
                {
                    // this case is a link pointing to an executable with no parameters
                    // core developers find unacceptable executing it even after asking the user
                    KMessageBox::information( widget(), i18n("The document is trying to execute an external application and, for your safety, Okular does not allow that.") );
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
            QString lilySource;
            int lilyRow = 0, lilyCol = 0;
            // if the url is a mailto one, invoke mailer
            if ( browse->url().startsWith( "mailto:", Qt::CaseInsensitive ) )
                KToolInvocation::invokeMailer( browse->url() );
            else if ( extractLilyPondSourceReference( browse->url(), &lilySource, &lilyRow, &lilyCol ) )
            {
                const SourceReference ref( lilySource, lilyRow, lilyCol );
                processSourceReference( &ref );
            }
            else
            {
                QString url = browse->url();

                // fix for #100366, documents with relative links that are the form of http:foo.pdf
                if (url.indexOf("http:") == 0 && url.indexOf("http://") == -1 && url.right(4) == ".pdf")
                {
                    d->openRelativeFile(url.mid(5));
                    return;
                }

                KUrl realUrl = KUrl( url );

                // handle documents with relative path
                if ( d->m_url.isValid() )
                {
                    realUrl = KUrl( d->m_url.upUrl(), url );
                }

                // Albert: this is not a leak!
                new KRun( realUrl, widget() );
            }
            } break;

        case Action::Sound: {
            const SoundAction * linksound = static_cast< const SoundAction * >( action );
            AudioPlayer::instance()->playSound( linksound->sound(), linksound );
            } break;

        case Action::Script: {
            const ScriptAction * linkscript = static_cast< const ScriptAction * >( action );
            if ( !d->m_scripter )
                d->m_scripter = new Scripter( d );
            d->m_scripter->execute( linkscript->scriptType(), linkscript->script() );
            } break;

        case Action::Movie:
            emit processMovieAction( static_cast< const MovieAction * >( action ) );
            break;
        case Action::Rendition: {
            const RenditionAction * linkrendition = static_cast< const RenditionAction * >( action );
            if ( !linkrendition->script().isEmpty() )
            {
                if ( !d->m_scripter )
                    d->m_scripter = new Scripter( d );
                d->m_scripter->execute( linkrendition->scriptType(), linkrendition->script() );
            }
            else
            {
                emit processRenditionAction( static_cast< const RenditionAction * >( action ) );
            }
            } break;
    }
}

void Document::processSourceReference( const SourceReference * ref )
{
    if ( !ref )
        return;

    const KUrl url( d->giveAbsolutePath( ref->fileName() ) );
    if ( !url.isLocalFile() )
    {
        kDebug(OkularDebug) << url.url() << "is not a local file.";
        return;
    }

    const QString absFileName = url.toLocalFile();
    if ( !QFile::exists( absFileName ) )
    {
        kDebug(OkularDebug) << "No such file:" << absFileName;
        return;
    }

    bool handled = false;
    emit sourceReferenceActivated(absFileName, ref->row(), ref->column(), &handled);
    if(handled) {
        return;
    }

    static QHash< int, QString > editors;
    // init the editors table if empty (on first run, usually)
    if ( editors.isEmpty() )
    {
        editors = buildEditorsMap();
    }

    QHash< int, QString >::const_iterator it = editors.constFind( SettingsCore::externalEditor() );
    QString p;
    if ( it != editors.constEnd() )
        p = *it;
    else
        p = SettingsCore::externalEditorCommand();
    // custom editor not yet configured
    if ( p.isEmpty() )
        return;

    // manually append the %f placeholder if not specified
    if ( p.indexOf( QLatin1String( "%f" ) ) == -1 )
        p.append( QLatin1String( " %f" ) );

    // replacing the placeholders
    QHash< QChar, QString > map;
    map.insert( 'f', absFileName );
    map.insert( 'c', QString::number( ref->column() ) );
    map.insert( 'l', QString::number( ref->row() ) );
    const QString cmd = KMacroExpander::expandMacrosShellQuote( p, map );
    if ( cmd.isEmpty() )
        return;
    const QStringList args = KShell::splitArgs( cmd );
    if ( args.isEmpty() )
        return;

    KProcess::startDetached( args );
}

const SourceReference * Document::dynamicSourceReference( int pageNr, double absX, double absY )
{
    const SourceReference * ref = 0;
    if ( d->m_generator )
    {
        QMetaObject::invokeMethod( d->m_generator, "dynamicSourceReference", Qt::DirectConnection, Q_RETURN_ARG(const Okular::SourceReference*, ref), Q_ARG(int, pageNr), Q_ARG(double, absX), Q_ARG(double, absY) );
    }
    return ref;
}

Document::PrintingType Document::printingSupport() const
{
    if ( d->m_generator )
    {

        if ( d->m_generator->hasFeature( Generator::PrintNative ) )
        {
            return NativePrinting;
        }

#ifndef Q_OS_WIN
        if ( d->m_generator->hasFeature( Generator::PrintPostscript ) )
        {
            return PostscriptPrinting;
        }
#endif

    }

    return NoPrinting;
}

bool Document::supportsPrintToFile() const
{
    return d->m_generator ? d->m_generator->hasFeature( Generator::PrintToFile ) : false;
}

bool Document::print( QPrinter &printer )
{
    return d->m_generator ? d->m_generator->print( printer ) : false;
}

QString Document::printError() const
{
    Okular::Generator::PrintError err = Generator::UnknownPrintError;
    if ( d->m_generator )
    {
        QMetaObject::invokeMethod( d->m_generator, "printError", Qt::DirectConnection, Q_RETURN_ARG(Okular::Generator::PrintError, err) );
    }
    Q_ASSERT( err != Generator::NoPrintError );
    switch ( err )
    {
        case Generator::TemporaryFileOpenPrintError:
            return i18n( "Could not open a temporary file" );
        case Generator::FileConversionPrintError:
            return i18n( "Print conversion failed" );
        case Generator::PrintingProcessCrashPrintError:
            return i18n( "Printing process crashed" );
        case Generator::PrintingProcessStartPrintError:
            return i18n( "Printing process could not start" );
        case Generator::PrintToFilePrintError:
            return i18n( "Printing to file failed" );
        case Generator::InvalidPrinterStatePrintError:
            return i18n( "Printer was in invalid state" );
        case Generator::UnableToFindFilePrintError:
            return i18n( "Unable to find file to print" );
        case Generator::NoFileToPrintError:
            return i18n( "There was no file to print" );
        case Generator::NoBinaryToPrintError:
            return i18n( "Could not find a suitable binary for printing. Make sure CUPS lpr binary is available" );
        case Generator::InvalidPageSizePrintError:
            return i18n( "The page print size is invalid" );
        case Generator::NoPrintError:
            return QString();
        case Generator::UnknownPrintError:
            return QString();
    }

    return QString();
}

QWidget* Document::printConfigurationWidget() const
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
        Okular::ConfigInterface * iface = d->generatorConfig( it.value() );
        if ( iface )
        {
            iface->addPages( dialog );
            pagesAdded = true;
            if ( !it.value().catalogName.isEmpty() )
                KGlobal::locale()->insertCatalog( it.value().catalogName );
        }
    }
    if ( pagesAdded )
    {
        connect( dialog, SIGNAL(settingsChanged(QString)),
                 this, SLOT(slotGeneratorConfigChanged(QString)) );
    }
}

int Document::configurableGenerators() const
{
    QString constraint( "([X-KDE-Priority] > 0) and (exist Library) and ([X-KDE-okularHasInternalSettings])" );
    KService::List offers = KServiceTypeTrader::self()->query( "okular/Generator", constraint );
    return offers.count();
}

QStringList Document::supportedMimeTypes() const
{
    if ( !d->m_supportedMimeTypes.isEmpty() )
        return d->m_supportedMimeTypes;

    QString constraint( "(Library == 'okularpart')" );
    QLatin1String basePartService( "KParts/ReadOnlyPart" );
    KService::List offers = KServiceTypeTrader::self()->query( basePartService, constraint );
    KService::List::ConstIterator it = offers.constBegin(), itEnd = offers.constEnd();
    for ( ; it != itEnd; ++it )
    {
        KService::Ptr service = *it;
        QStringList mimeTypes = service->serviceTypes();
        foreach ( const QString& mimeType, mimeTypes )
            if ( mimeType != basePartService )
                d->m_supportedMimeTypes.append( mimeType );
    }

    return d->m_supportedMimeTypes;
}

const KComponentData* Document::componentData() const
{
    if ( !d->m_generator )
        return 0;

    QHash< QString, GeneratorInfo >::const_iterator genIt = d->m_loadedGenerators.constFind( d->m_generatorName );
    Q_ASSERT( genIt != d->m_loadedGenerators.constEnd() );
    const KComponentData* kcd = &genIt.value().data;

    // empty about data
    if ( kcd->isValid() && kcd->aboutData() && kcd->aboutData()->programName().isEmpty() )
        return 0;

    return kcd;
}

bool Document::canSaveChanges() const
{
    if ( !d->m_generator )
        return false;
    Q_ASSERT( !d->m_generatorName.isEmpty() );

    QHash< QString, GeneratorInfo >::iterator genIt = d->m_loadedGenerators.find( d->m_generatorName );
    Q_ASSERT( genIt != d->m_loadedGenerators.end() );
    SaveInterface* saveIface = d->generatorSave( genIt.value() );
    if ( !saveIface )
        return false;

    return saveIface->supportsOption( SaveInterface::SaveChanges );
}

bool Document::canSaveChanges( SaveCapability cap ) const
{
    switch ( cap )
    {
        case SaveFormsCapability:
            /* Assume that if the generator supports saving, forms can be saved.
             * We have no means to actually query the generator at the moment
             * TODO: Add some method to query the generator in SaveInterface */
            return canSaveChanges();

        case SaveAnnotationsCapability:
            return d->canAddAnnotationsNatively();
    }

    return false;
}

bool Document::saveChanges( const QString &fileName )
{
    QString errorText;
    return saveChanges( fileName, &errorText );
}

bool Document::saveChanges( const QString &fileName, QString *errorText )
{
    if ( !d->m_generator || fileName.isEmpty() )
        return false;
    Q_ASSERT( !d->m_generatorName.isEmpty() );

    QHash< QString, GeneratorInfo >::iterator genIt = d->m_loadedGenerators.find( d->m_generatorName );
    Q_ASSERT( genIt != d->m_loadedGenerators.end() );
    SaveInterface* saveIface = d->generatorSave( genIt.value() );
    if ( !saveIface || !saveIface->supportsOption( SaveInterface::SaveChanges ) )
        return false;

    return saveIface->save( fileName, SaveInterface::SaveChanges, errorText );
}

void Document::registerView( View *view )
{
    if ( !view )
        return;

    Document *viewDoc = view->viewDocument();
    if ( viewDoc )
    {
        // check if already registered for this document
        if ( viewDoc == this )
            return;

        viewDoc->unregisterView( view );
    }

    d->m_views.insert( view );
    view->d_func()->document = d;
}

void Document::unregisterView( View *view )
{
    if ( !view )
        return;

    Document *viewDoc = view->viewDocument();
    if ( !viewDoc || viewDoc != this )
        return;

    view->d_func()->document = 0;
    d->m_views.remove( view );
}

QByteArray Document::fontData(const FontInfo &font) const
{
    QByteArray result;

    if (d->m_generator)
    {
        QMetaObject::invokeMethod(d->m_generator, "requestFontData", Qt::DirectConnection, Q_ARG(Okular::FontInfo, font), Q_ARG(QByteArray *, &result));
    }

    return result;
}

bool Document::openDocumentArchive( const QString & docFile, const KUrl & url )
{
    const KMimeType::Ptr mime = KMimeType::findByPath( docFile, 0, false /* content too */ );
    if ( !mime->is( "application/vnd.kde.okular-archive" ) )
        return false;

    KZip okularArchive( docFile );
    if ( !okularArchive.open( QIODevice::ReadOnly ) )
       return false;

    const KArchiveDirectory * mainDir = okularArchive.directory();
    const KArchiveEntry * mainEntry = mainDir->entry( "content.xml" );
    if ( !mainEntry || !mainEntry->isFile() )
        return false;

    std::auto_ptr< QIODevice > mainEntryDevice( static_cast< const KZipFileEntry * >( mainEntry )->createDevice() );
    QDomDocument doc;
    if ( !doc.setContent( mainEntryDevice.get() ) )
        return false;
    mainEntryDevice.reset();

    QDomElement root = doc.documentElement();
    if ( root.tagName() != "OkularArchive" )
        return false;

    QString documentFileName;
    QString metadataFileName;
    QDomElement el = root.firstChild().toElement();
    for ( ; !el.isNull(); el = el.nextSibling().toElement() )
    {
        if ( el.tagName() == "Files" )
        {
            QDomElement fileEl = el.firstChild().toElement();
            for ( ; !fileEl.isNull(); fileEl = fileEl.nextSibling().toElement() )
            {
                if ( fileEl.tagName() == "DocumentFileName" )
                    documentFileName = fileEl.text();
                else if ( fileEl.tagName() == "MetadataFileName" )
                    metadataFileName = fileEl.text();
            }
        }
    }
    if ( documentFileName.isEmpty() )
        return false;

    const KArchiveEntry * docEntry = mainDir->entry( documentFileName );
    if ( !docEntry || !docEntry->isFile() )
        return false;

    std::auto_ptr< ArchiveData > archiveData( new ArchiveData() );
    const int dotPos = documentFileName.indexOf( '.' );
    if ( dotPos != -1 )
        archiveData->document.setSuffix( documentFileName.mid( dotPos ) );
    if ( !archiveData->document.open() )
        return false;

    QString tempFileName = archiveData->document.fileName();
    {
    std::auto_ptr< QIODevice > docEntryDevice( static_cast< const KZipFileEntry * >( docEntry )->createDevice() );
    copyQIODevice( docEntryDevice.get(), &archiveData->document );
    archiveData->document.close();
    }

    std::auto_ptr< KTemporaryFile > tempMetadataFileName;
    const KArchiveEntry * metadataEntry = mainDir->entry( metadataFileName );
    if ( metadataEntry && metadataEntry->isFile() )
    {
        std::auto_ptr< QIODevice > metadataEntryDevice( static_cast< const KZipFileEntry * >( metadataEntry )->createDevice() );
        tempMetadataFileName.reset( new KTemporaryFile() );
        tempMetadataFileName->setSuffix( ".xml" );
        tempMetadataFileName->setAutoRemove( false );
        if ( tempMetadataFileName->open() )
        {
            copyQIODevice( metadataEntryDevice.get(), tempMetadataFileName.get() );
            archiveData->metadataFileName = tempMetadataFileName->fileName();
            tempMetadataFileName->close();
        }
    }

    const KMimeType::Ptr docMime = KMimeType::findByPath( tempFileName, 0, true /* local file */ );
    d->m_archiveData = archiveData.get();
    d->m_archivedFileName = documentFileName;
    bool ret = openDocument( tempFileName, url, docMime );

    if ( ret )
    {
        archiveData.release();
    }
    else
    {
        d->m_archiveData = 0;
    }

    return ret;
}

bool Document::saveDocumentArchive( const QString &fileName )
{
    if ( !d->m_generator )
        return false;

    /* If we opened an archive, use the name of original file (eg foo.pdf)
     * instead of the archive's one (eg foo.okular) */
    QString docFileName = d->m_archiveData ? d->m_archivedFileName : d->m_url.fileName();
    if ( docFileName == QLatin1String( "-" ) )
        return false;

    QString docPath = d->m_docFileName;
    const QFileInfo fi( docPath );
    if ( fi.isSymLink() )
        docPath = fi.symLinkTarget();

    KZip okularArchive( fileName );
    if ( !okularArchive.open( QIODevice::WriteOnly ) )
        return false;

    const KUser user;
#ifndef Q_OS_WIN
    const KUserGroup userGroup( user.gid() );
#else
    const KUserGroup userGroup( QString( "" ) );
#endif

    QDomDocument contentDoc( "OkularArchive" );
    QDomProcessingInstruction xmlPi = contentDoc.createProcessingInstruction(
            QString::fromLatin1( "xml" ), QString::fromLatin1( "version=\"1.0\" encoding=\"utf-8\"" ) );
    contentDoc.appendChild( xmlPi );
    QDomElement root = contentDoc.createElement( "OkularArchive" );
    contentDoc.appendChild( root );

    QDomElement filesNode = contentDoc.createElement( "Files" );
    root.appendChild( filesNode );

    QDomElement fileNameNode = contentDoc.createElement( "DocumentFileName" );
    filesNode.appendChild( fileNameNode );
    fileNameNode.appendChild( contentDoc.createTextNode( docFileName ) );

    QDomElement metadataFileNameNode = contentDoc.createElement( "MetadataFileName" );
    filesNode.appendChild( metadataFileNameNode );
    metadataFileNameNode.appendChild( contentDoc.createTextNode( "metadata.xml" ) );

    // If the generator can save annotations natively, do it
    KTemporaryFile modifiedFile;
    bool annotationsSavedNatively = false;
    if ( d->canAddAnnotationsNatively() )
    {
        if ( !modifiedFile.open() )
            return false;

        modifiedFile.close(); // We're only interested in the file name

        QString errorText;
        if ( saveChanges( modifiedFile.fileName(), &errorText ) )
        {
            docPath = modifiedFile.fileName(); // Save this instead of the original file
            annotationsSavedNatively = true;
        }
        else
        {
            kWarning(OkularDebug) << "saveChanges failed: " << errorText;
            kDebug(OkularDebug) << "Falling back to saving a copy of the original file";
        }
    }

    KTemporaryFile metadataFile;
    PageItems saveWhat = annotationsSavedNatively ? None : AnnotationPageItems;
    if ( !d->savePageDocumentInfo( &metadataFile, saveWhat ) )
        return false;

    const QByteArray contentDocXml = contentDoc.toByteArray();
    okularArchive.writeFile( "content.xml", user.loginName(), userGroup.name(),
                             contentDocXml.constData(), contentDocXml.length() );

    okularArchive.addLocalFile( docPath, docFileName );
    okularArchive.addLocalFile( metadataFile.fileName(), "metadata.xml" );

    if ( !okularArchive.close() )
        return false;

    return true;
}

QPrinter::Orientation Document::orientation() const
{
    double width, height;
    int landscape, portrait;
    const Okular::Page *currentPage;

    // if some pages are landscape and others are not, the most common wins, as
    // QPrinter does not accept a per-page setting
    landscape = 0;
    portrait = 0;
    for (uint i = 0; i < pages(); i++)
    {
        currentPage = page(i);
        width = currentPage->width();
        height = currentPage->height();
        if (currentPage->orientation() == Okular::Rotation90 || currentPage->orientation() == Okular::Rotation270) qSwap(width, height);
        if (width > height) landscape++;
        else portrait++;
    }
    return (landscape > portrait) ? QPrinter::Landscape : QPrinter::Portrait;
}

void Document::setAnnotationEditingEnabled( bool enable )
{
    d->m_annotationEditingEnabled = enable;
    foreachObserver( notifySetup( d->m_pagesVector, 0 ) );
}

void DocumentPrivate::requestDone( PixmapRequest * req )
{
    if ( !req )
        return;

    if ( !m_generator || m_closingLoop )
    {
        m_pixmapRequestsMutex.lock();
        m_executingPixmapRequests.removeAll( req );
        m_pixmapRequestsMutex.unlock();
        delete req;
        if ( m_closingLoop )
            m_closingLoop->exit();
        return;
    }

#ifndef NDEBUG
    if ( !m_generator->canGeneratePixmap() )
        kDebug(OkularDebug) << "requestDone with generator not in READY state.";
#endif

    // [MEM] 1.1 find and remove a previous entry for the same page and id
    QLinkedList< AllocatedPixmap * >::iterator aIt = m_allocatedPixmaps.begin();
    QLinkedList< AllocatedPixmap * >::iterator aEnd = m_allocatedPixmaps.end();
    for ( ; aIt != aEnd; ++aIt )
        if ( (*aIt)->page == req->pageNumber() && (*aIt)->observer == req->observer() )
        {
            AllocatedPixmap * p = *aIt;
            m_allocatedPixmaps.erase( aIt );
            m_allocatedPixmapsTotalMemory -= p->memory;
            delete p;
            break;
        }

    DocumentObserver *observer = req->observer();
    if ( m_observers.contains(observer) )
    {
        // [MEM] 1.2 append memory allocation descriptor to the FIFO
        qulonglong memoryBytes = 0;
        const TilesManager *tm = req->d->tilesManager();
        if ( tm )
            memoryBytes = tm->totalMemory();
        else
            memoryBytes = 4 * req->width() * req->height();

        AllocatedPixmap * memoryPage = new AllocatedPixmap( req->observer(), req->pageNumber(), memoryBytes );
        m_allocatedPixmaps.append( memoryPage );
        m_allocatedPixmapsTotalMemory += memoryBytes;

        // 2. notify an observer that its pixmap changed
        observer->notifyPageChanged( req->pageNumber(), DocumentObserver::Pixmap );
    }
#ifndef NDEBUG
    else
        kWarning(OkularDebug) << "Receiving a done request for the defunct observer" << observer;
#endif

    // 3. delete request
    m_pixmapRequestsMutex.lock();
    m_executingPixmapRequests.removeAll( req );
    m_pixmapRequestsMutex.unlock();
    delete req;

    // 4. start a new generation if some is pending
    m_pixmapRequestsMutex.lock();
    bool hasPixmaps = !m_pixmapRequestsStack.isEmpty();
    m_pixmapRequestsMutex.unlock();
    if ( hasPixmaps )
        sendGeneratorPixmapRequest();
}

void DocumentPrivate::setPageBoundingBox( int page, const NormalizedRect& boundingBox )
{
    Page * kp = m_pagesVector[ page ];
    if ( !m_generator || !kp )
        return;

    if ( kp->boundingBox() == boundingBox )
        return;
    kp->setBoundingBox( boundingBox );

    // notify observers about the change
    foreachObserverD( notifyPageChanged( page, DocumentObserver::BoundingBox ) );

    // TODO: For generators that generate the bbox by pixmap scanning, if the first generated pixmap is very small, the bounding box will forever be inaccurate.
    // TODO: Crop computation should also consider annotations, actions, etc. to make sure they're not cropped away.
    // TODO: Help compute bounding box for generators that create a QPixmap without a QImage, like text and plucker.
    // TODO: Don't compute the bounding box if no one needs it (e.g., Trim Borders is off).

}

void DocumentPrivate::calculateMaxTextPages()
{
    int multipliers = qMax(1, qRound(getTotalMemory() / 536870912.0)); // 512 MB
    switch (SettingsCore::memoryLevel())
    {
        case SettingsCore::EnumMemoryLevel::Low:
            m_maxAllocatedTextPages = multipliers * 2;
        break;

        case SettingsCore::EnumMemoryLevel::Normal:
            m_maxAllocatedTextPages = multipliers * 50;
        break;

        case SettingsCore::EnumMemoryLevel::Aggressive:
            m_maxAllocatedTextPages = multipliers * 250;
        break;

        case SettingsCore::EnumMemoryLevel::Greedy:
            m_maxAllocatedTextPages = multipliers * 1250;
        break;
    }
}

void DocumentPrivate::textGenerationDone( Page *page )
{
    if ( !m_generator || m_closingLoop ) return;

    // 1. If we reached the cache limit, delete the first text page from the fifo
    if (m_allocatedTextPagesFifo.size() == m_maxAllocatedTextPages)
    {
        int pageToKick = m_allocatedTextPagesFifo.takeFirst();
        if (pageToKick != page->number()) // this should never happen but better be safe than sorry
        {
            m_pagesVector.at(pageToKick)->setTextPage( 0 ); // deletes the textpage
        }
    }

    // 2. Add the page to the fifo of generated text pages
    m_allocatedTextPagesFifo.append( page->number() );
}

void Document::setRotation( int r )
{
    d->setRotationInternal( r, true );
}

void DocumentPrivate::setRotationInternal( int r, bool notify )
{
    Rotation rotation = (Rotation)r;
    if ( !m_generator || ( m_rotation == rotation ) )
	return;

    // tell the pages to rotate
    QVector< Okular::Page * >::const_iterator pIt = m_pagesVector.constBegin();
    QVector< Okular::Page * >::const_iterator pEnd = m_pagesVector.constEnd();
    for ( ; pIt != pEnd; ++pIt )
        (*pIt)->d->rotateAt( rotation );
    if ( notify )
    {
        // notify the generator that the current rotation has changed
        m_generator->rotationChanged( rotation, m_rotation );
    }
    // set the new rotation
    m_rotation = rotation;

    if ( notify )
    {
        foreachObserverD( notifySetup( m_pagesVector, DocumentObserver::NewLayoutForPages ) );
        foreachObserverD( notifyContentsCleared( DocumentObserver::Pixmap | DocumentObserver::Highlights | DocumentObserver::Annotations ) );
    }
    kDebug(OkularDebug) << "Rotated:" << r;
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
    QVector< Okular::Page * >::const_iterator pIt = d->m_pagesVector.constBegin();
    QVector< Okular::Page * >::const_iterator pEnd = d->m_pagesVector.constEnd();
    for ( ; pIt != pEnd; ++pIt )
        (*pIt)->d->changeSize( size );
    // clear 'memory allocation' descriptors
    qDeleteAll( d->m_allocatedPixmaps );
    d->m_allocatedPixmaps.clear();
    d->m_allocatedPixmapsTotalMemory = 0;
    // notify the generator that the current page size has changed
    d->m_generator->pageSizeChanged( size, d->m_pageSize );
    // set the new page size
    d->m_pageSize = size;

    foreachObserver( notifySetup( d->m_pagesVector, DocumentObserver::NewLayoutForPages ) );
    foreachObserver( notifyContentsCleared( DocumentObserver::Pixmap | DocumentObserver::Highlights ) );
    kDebug(OkularDebug) << "New PageSize id:" << sizeid;
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

bool DocumentViewport::operator<( const DocumentViewport & vp ) const
{
    // TODO: Check autoFit and Position

    if ( pageNumber != vp.pageNumber )
        return pageNumber < vp.pageNumber;

    if ( !rePos.enabled && vp.rePos.enabled )
        return true;

    if ( !vp.rePos.enabled )
        return false;

    if ( rePos.normalizedY != vp.rePos.normalizedY )
        return rePos.normalizedY < vp.rePos.normalizedY;

    return rePos.normalizedX < vp.rePos.normalizedX;
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

void DocumentInfo::set( Key key, const QString &value )
{
    const QString keyString = getKeyString( key );
    if ( !keyString.isEmpty() )
        set( keyString, value, getKeyTitle( key ) );
    else
        kWarning(OkularDebug) << "Invalid key passed";
}

QString DocumentInfo::get( const QString &key ) const
{
    const QDomElement docElement = documentElement();

    // check whether key already exists
    const QDomNodeList list = docElement.elementsByTagName( key );
    if ( list.count() > 0 )
        return list.item( 0 ).toElement().attribute( "value" );
    else
        return QString();
}

QString DocumentInfo::getKeyString( Key key ) //const
{
    switch ( key ) {
        case Title:
            return "title";
            break;
        case Subject:
            return "subject";
            break;
        case Description:
            return "description";
            break;
        case Author:
            return "author";
            break;
        case Creator:
            return "creator";
            break;
        case Producer:
            return "producer";
            break;
        case Copyright:
            return "copyright";
            break;
        case Pages:
            return "pages";
            break;
        case CreationDate:
            return "creationDate";
            break;
        case ModificationDate:
            return "modificationDate";
            break;
        case MimeType:
            return "mimeType";
            break;
        case Category:
            return "category";
            break;
        case Keywords:
            return "keywords";
            break;
        case FilePath:
            return "filePath";
            break;
        case DocumentSize:
            return "documentSize";
            break;
        case PagesSize:
            return "pageSize";
            break;
        default:
            return QString();
            break;
    }
}

QString DocumentInfo::getKeyTitle( Key key ) //const
{
    switch ( key ) {
        case Title:
            return i18n( "Title" );
            break;
        case Subject:
            return i18n( "Subject" );
            break;
        case Description:
            return i18n( "Description" );
            break;
        case Author:
            return i18n( "Author" );
            break;
        case Creator:
            return i18n( "Creator" );
            break;
        case Producer:
            return i18n( "Producer" );
            break;
        case Copyright:
            return i18n( "Copyright" );
            break;
        case Pages:
            return i18n( "Pages" );
            break;
        case CreationDate:
            return i18n( "Created" );
            break;
        case ModificationDate:
            return i18n( "Modified" );
            break;
        case MimeType:
            return i18n( "Mime Type" );
            break;
        case Category:
            return i18n( "Category" );
            break;
        case Keywords:
            return i18n( "Keywords" );
            break;
        case FilePath:
            return i18n( "File Path" );
            break;
        case DocumentSize:
            return i18n( "File Size" );
            break;
        case PagesSize:
            return i18n("Page Size");
            break;
        default:
            return QString();
            break;
    }
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

#undef foreachObserver
#undef foreachObserverD

#include "document.moc"

/* kate: replace-tabs on; indent-width 4; */
