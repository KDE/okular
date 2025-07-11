/*
    SPDX-FileCopyrightText: 2004-2005 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2004-2008 Albert Astals Cid <aacid@kde.org>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017, 2018 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "document.h"
#include "document_p.h"
#include "documentcommands_p.h"

#include <algorithm>
#include <limits.h>
#include <memory>
#ifdef Q_OS_WIN
#define _WIN32_WINNT 0x0500
#include <windows.h>
#elif defined(Q_OS_FREEBSD)
// clang-format off
// FreeBSD really wants this include order
#include <sys/types.h>
#include <sys/sysctl.h>
// clang-format on
#include <vm/vm_param.h>
#endif

// qt/kde/system includes
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMap>
#include <QMimeDatabase>
#include <QPageSize>
#include <QPrintDialog>
#include <QRegularExpression>
#include <QScreen>
#include <QStack>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTimer>
#include <QUndoCommand>
#include <QWindow>
#include <QtAlgorithms>

#include <KApplicationTrader>
#include <KAuthorized>
#include <KConfigDialog>
#include <KFormat>
#include <KIO/Global>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KMacroExpander>
#include <KPluginMetaData>
#include <KProcess>
#include <KShell>
#include <kio_version.h>
#include <kzip.h>

// local includes
#include "action.h"
#include "annotations.h"
#include "annotations_p.h"
#include "audioplayer.h"
#include "bookmarkmanager.h"
#include "chooseenginedialog_p.h"
#include "debug_p.h"
#include "form.h"
#include "generator_p.h"
#include "interfaces/configinterface.h"
#include "interfaces/guiinterface.h"
#include "interfaces/printinterface.h"
#include "interfaces/saveinterface.h"
#include "misc.h"
#include "observer.h"
#include "page.h"
#include "page_p.h"
#include "pagecontroller_p.h"
#include "script/event_p.h"
#include "scripter.h"
#include "settings_core.h"
#include "sourcereference.h"
#include "sourcereference_p.h"
#include "texteditors_p.h"
#include "tile.h"
#include "tilesmanager_p.h"
#include "utils.h"
#include "utils_p.h"
#include "view.h"
#include "view_p.h"

#include <config-okular.h>

#if HAVE_MALLOC_TRIM
#include "malloc.h"
#endif

using namespace Okular;

struct AllocatedPixmap {
    // owner of the page
    DocumentObserver *observer;
    int page;
    qulonglong memory;
    // public constructor: initialize data
    AllocatedPixmap(DocumentObserver *o, int p, qulonglong m)
        : observer(o)
        , page(p)
        , memory(m)
    {
    }
};

struct ArchiveData {
    ArchiveData()
    {
    }

    QString originalFileName;
    QTemporaryFile document;
    QTemporaryFile metadataFile;
};

struct RunningSearch {
    // store search properties
    int continueOnPage;
    RegularAreaRect continueOnMatch;
    QSet<int> highlightedPages;

    // fields related to previous searches (used for 'continueSearch')
    QString cachedString;
    Document::SearchType cachedType;
    Qt::CaseSensitivity cachedCaseSensitivity;
    bool cachedViewportMove : 1;
    bool isCurrentlySearching : 1;
    QColor cachedColor;
    int pagesDone;
};

#define foreachObserver(cmd)                                                                                                                                                                                                                   \
    {                                                                                                                                                                                                                                          \
        for (auto *it : std::as_const(d->m_observers)) {                                                                                                                                                                                       \
            it->cmd;                                                                                                                                                                                                                           \
        }                                                                                                                                                                                                                                      \
    }

#define foreachObserverD(cmd)                                                                                                                                                                                                                  \
    {                                                                                                                                                                                                                                          \
        for (auto *it : std::as_const(m_observers)) {                                                                                                                                                                                          \
            it->cmd;                                                                                                                                                                                                                           \
        }                                                                                                                                                                                                                                      \
    }

#define OKULAR_HISTORY_MAXSTEPS 100
#define OKULAR_HISTORY_SAVEDSTEPS 10

// how often to run slotTimedMemoryCheck
constexpr int kMemCheckTime = 2000; // in msec
// getFreeMemory is called every two seconds when checking to see if the system is low on memory. If this timeout was left at kMemCheckTime, half of these checks are useless (when okular is idle) since the cache is used when the cache is
// <=2 seconds old. This means that after the system is out of memory, up to 4 seconds (instead of 2) could go by before okular starts to free memory.
constexpr int kFreeMemCacheTimeout = kMemCheckTime - 100;

/***** Document ******/

QString DocumentPrivate::pagesSizeString() const
{
    if (m_generator) {
        if (m_generator->pagesSizeMetric() != Generator::None) {
            QSizeF size = m_parent->allPagesSize();
            // Single page size
            if (size.isValid()) {
                return localizedSize(size);
            }

            // Multiple page sizes
            QHash<QString, int> pageSizeFrequencies;

            // Compute frequencies of each page size
            for (const Page *page : std::as_const(m_pagesVector)) {
                QString sizeString = localizedSize(QSizeF(page->width(), page->height()));
                pageSizeFrequencies[sizeString] = pageSizeFrequencies.value(sizeString, 0) + 1;
            }

            // Figure out which page size is most frequent
            int largestFrequencySeen = 0;
            QString mostCommonPageSize;
            for (const auto &[key, value] : pageSizeFrequencies.asKeyValueRange()) {
                if (value > largestFrequencySeen) {
                    largestFrequencySeen = value;
                    mostCommonPageSize = key;
                }
            }
            QString finalText = i18nc("@info %1 is a page size", "Most pages are %1.", mostCommonPageSize);

            return finalText;
        } else {
            return QString();
        }
    } else {
        return QString();
    }
}

QString DocumentPrivate::namePaperSize(double inchesWidth, double inchesHeight) const
{
    const QPageLayout::Orientation orientation = inchesWidth > inchesHeight ? QPageLayout::Landscape : QPageLayout::Portrait;

    const QSize pointsSize(inchesWidth * 72.0, inchesHeight * 72.0);
    const QPageSize::PageSizeId paperSize = QPageSize::id(pointsSize, QPageSize::FuzzyOrientationMatch);

    const QString paperName = QPageSize::name(paperSize);

    if (orientation == QPageLayout::Portrait) {
        return i18nc("paper type and orientation (eg: Portrait A4)", "Portrait %1", paperName);
    } else {
        return i18nc("paper type and orientation (eg: Portrait A4)", "Landscape %1", paperName);
    }
}

QString DocumentPrivate::localizedSize(const QSizeF size) const
{
    double inchesWidth = 0, inchesHeight = 0;
    switch (m_generator->pagesSizeMetric()) {
    case Generator::Points:
        inchesWidth = size.width() / 72.0;
        inchesHeight = size.height() / 72.0;
        break;

    case Generator::Pixels: {
        const QSizeF dpi = m_generator->dpi();
        inchesWidth = size.width() / dpi.width();
        inchesHeight = size.height() / dpi.height();
    } break;

    case Generator::None:
        break;
    }
    if (QLocale::system().measurementSystem() == QLocale::ImperialSystem) {
        return i18nc("%1 is width, %2 is height, %3 is paper size name", "%1 × %2 in (%3)", inchesWidth, inchesHeight, namePaperSize(inchesWidth, inchesHeight));
    } else {
        return i18nc("%1 is width, %2 is height, %3 is paper size name", "%1 × %2 mm (%3)", QString::number(inchesWidth * 25.4, 'd', 0), QString::number(inchesHeight * 25.4, 'd', 0), namePaperSize(inchesWidth, inchesHeight));
    }
}

qulonglong DocumentPrivate::calculateMemoryToFree()
{
    // [MEM] choose memory parameters based on configuration profile
    qulonglong clipValue = 0;
    qulonglong memoryToFree = 0;

    switch (SettingsCore::memoryLevel()) {
    case SettingsCore::EnumMemoryLevel::Low:
        memoryToFree = m_allocatedPixmapsTotalMemory;
        break;

    case SettingsCore::EnumMemoryLevel::Normal: {
        qulonglong thirdTotalMemory = getTotalMemory() / 3;
        qulonglong freeMemory = getFreeMemory();
        if (m_allocatedPixmapsTotalMemory > thirdTotalMemory) {
            memoryToFree = m_allocatedPixmapsTotalMemory - thirdTotalMemory;
        }
        if (m_allocatedPixmapsTotalMemory > freeMemory) {
            clipValue = (m_allocatedPixmapsTotalMemory - freeMemory) / 2;
        }
    } break;

    case SettingsCore::EnumMemoryLevel::Aggressive: {
        qulonglong freeMemory = getFreeMemory();
        if (m_allocatedPixmapsTotalMemory > freeMemory) {
            clipValue = (m_allocatedPixmapsTotalMemory - freeMemory) / 2;
        }
    } break;
    case SettingsCore::EnumMemoryLevel::Greedy: {
        qulonglong freeSwap;
        qulonglong freeMemory = getFreeMemory(&freeSwap);
        const qulonglong memoryLimit = qMin(qMax(freeMemory, getTotalMemory() / 2), freeMemory + freeSwap);
        if (m_allocatedPixmapsTotalMemory > memoryLimit) {
            clipValue = (m_allocatedPixmapsTotalMemory - memoryLimit) / 2;
        }
    } break;
    }

    if (clipValue > memoryToFree) {
        memoryToFree = clipValue;
    }

    return memoryToFree;
}

void DocumentPrivate::cleanupPixmapMemory()
{
    cleanupPixmapMemory(calculateMemoryToFree());
}

void DocumentPrivate::cleanupPixmapMemory(qulonglong memoryToFree)
{
    if (memoryToFree < 1) {
        return;
    }

    const int currentViewportPage = (*m_viewportIterator).pageNumber;

    // Create a QMap of visible rects, indexed by page number
    QMap<int, VisiblePageRect *> visibleRects;
    for (auto *it : std::as_const(m_pageRects)) {
        visibleRects.insert(it->pageNumber, it);
    }

    // Free memory starting from pages that are farthest from the current one
    int pagesFreed = 0;
    while (memoryToFree > 0) {
        AllocatedPixmap *p = searchLowestPriorityPixmap(true, true);
        if (!p) { // No pixmap to remove
            break;
        }

        qCDebug(OkularCoreDebug).nospace() << "Evicting cache pixmap observer=" << p->observer << " page=" << p->page;

        // m_allocatedPixmapsTotalMemory can't underflow because we always add or remove
        // the memory used by the AllocatedPixmap so at most it can reach zero
        m_allocatedPixmapsTotalMemory -= p->memory;
        // Make sure memoryToFree does not underflow
        if (p->memory > memoryToFree) {
            memoryToFree = 0;
        } else {
            memoryToFree -= p->memory;
        }
        pagesFreed++;
        // delete pixmap
        m_pagesVector.at(p->page)->deletePixmap(p->observer);
        // delete allocation descriptor
        delete p;
    }

    // If we're still on low memory, try to free individual tiles

    // Store pages that weren't completely removed

    std::list<AllocatedPixmap *> pixmapsToKeep;
    while (memoryToFree > 0) {
        int clean_hits = 0;
        for (DocumentObserver *observer : std::as_const(m_observers)) {
            AllocatedPixmap *p = searchLowestPriorityPixmap(false, true, observer);
            if (!p) { // No pixmap to remove
                continue;
            }

            clean_hits++;

            TilesManager *tilesManager = m_pagesVector.at(p->page)->d->tilesManager(observer);
            if (tilesManager && tilesManager->totalMemory() > 0) {
                qulonglong memoryDiff = p->memory;
                NormalizedRect visibleRect;
                if (visibleRects.contains(p->page)) {
                    visibleRect = visibleRects[p->page]->rect;
                }

                // Free non visible tiles
                tilesManager->cleanupPixmapMemory(memoryToFree, visibleRect, currentViewportPage);

                p->memory = tilesManager->totalMemory();
                memoryDiff -= p->memory;
                memoryToFree = (memoryDiff < memoryToFree) ? (memoryToFree - memoryDiff) : 0;
                m_allocatedPixmapsTotalMemory -= memoryDiff;

                if (p->memory > 0) {
                    pixmapsToKeep.push_back(p);
                } else {
                    delete p;
                }
            } else {
                pixmapsToKeep.push_back(p);
            }
        }

        if (clean_hits == 0) {
            break;
        }
    }

    m_allocatedPixmaps.splice(m_allocatedPixmaps.end(), pixmapsToKeep);
    Q_UNUSED(pagesFreed);
    // p--rintf("freeMemory A:[%d -%d = %d] \n", m_allocatedPixmaps.count() + pagesFreed, pagesFreed, m_allocatedPixmaps.count() );
}

/* Returns the next pixmap to evict from cache, or NULL if no suitable pixmap
 * if found. If unloadableOnly is set, only unloadable pixmaps are returned. If
 * thenRemoveIt is set, the pixmap is removed from m_allocatedPixmaps before
 * returning it
 */
AllocatedPixmap *DocumentPrivate::searchLowestPriorityPixmap(bool unloadableOnly, bool thenRemoveIt, DocumentObserver *observer)
{
    auto pIt = m_allocatedPixmaps.begin();
    auto pEnd = m_allocatedPixmaps.end();
    auto farthestPixmap = pEnd;
    const int currentViewportPage = m_viewportIterator->pageNumber;

    /* Find the pixmap that is farthest from the current viewport */
    int maxDistance = -1;
    while (pIt != pEnd) {
        const AllocatedPixmap *p = *pIt;
        // Filter by observer
        if (observer == nullptr || p->observer == observer) {
            const int distance = qAbs(p->page - currentViewportPage);
            if (maxDistance < distance && (!unloadableOnly || p->observer->canUnloadPixmap(p->page))) {
                maxDistance = distance;
                farthestPixmap = pIt;
            }
        }
        ++pIt;
    }

    /* No pixmap to remove */
    if (farthestPixmap == pEnd) {
        return nullptr;
    }

    AllocatedPixmap *selectedPixmap = *farthestPixmap;
    if (thenRemoveIt) {
        m_allocatedPixmaps.erase(farthestPixmap);
    }
    return selectedPixmap;
}

qulonglong DocumentPrivate::getTotalMemory()
{
    static qulonglong cachedValue = 0;
    if (cachedValue) {
        return cachedValue;
    }

#if defined(Q_OS_LINUX)
    // if /proc/meminfo doesn't exist, return 128MB
    QFile memFile(QStringLiteral("/proc/meminfo"));
    if (!memFile.open(QIODevice::ReadOnly)) {
        return (cachedValue = 134217728);
    }

    QTextStream readStream(&memFile);
    while (true) {
        QString entry = readStream.readLine();
        if (entry.isNull()) {
            break;
        }
        if (entry.startsWith(QLatin1String("MemTotal:"))) {
            return (cachedValue = (Q_UINT64_C(1024) * entry.section(QLatin1Char(' '), -2, -2).toULongLong()));
        }
    }
#elif defined(Q_OS_FREEBSD)
    qulonglong physmem;
    int mib[] = {CTL_HW, HW_PHYSMEM};
    size_t len = sizeof(physmem);
    if (sysctl(mib, 2, &physmem, &len, NULL, 0) == 0) {
        return (cachedValue = physmem);
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
    GlobalMemoryStatusEx(&stat);

    return (cachedValue = stat.ullTotalPhys);
#endif
    return (cachedValue = 134217728);
}

qulonglong DocumentPrivate::getFreeMemory(qulonglong *freeSwap)
{
    static QDeadlineTimer cacheTimer(0);
    static qulonglong cachedValue = 0;
    static qulonglong cachedFreeSwap = 0;

    if (!cacheTimer.hasExpired()) {
        if (freeSwap) {
            *freeSwap = cachedFreeSwap;
        }
        return cachedValue;
    }

    /* Initialize the returned free swap value to 0. It is overwritten if the
     * actual value is available */
    if (freeSwap) {
        *freeSwap = 0;
    }

#if defined(Q_OS_LINUX)
    // if /proc/meminfo doesn't exist, return MEMORY FULL
    QFile memFile(QStringLiteral("/proc/meminfo"));
    if (!memFile.open(QIODevice::ReadOnly)) {
        return 0;
    }

    // read /proc/meminfo and sum up the contents of 'MemFree', 'Buffers'
    // and 'Cached' fields. consider swapped memory as used memory.
    qulonglong memoryFree = 0;
    QString entry;
    QTextStream readStream(&memFile);
    static const int nElems = 5;
    const QString names[nElems] = {QStringLiteral("MemFree:"), QStringLiteral("Buffers:"), QStringLiteral("Cached:"), QStringLiteral("SwapFree:"), QStringLiteral("SwapTotal:")};
    qulonglong values[nElems] = {0, 0, 0, 0, 0};
    bool foundValues[nElems] = {false, false, false, false, false};
    while (true) {
        entry = readStream.readLine();
        if (entry.isNull()) {
            break;
        }
        for (int i = 0; i < nElems; ++i) {
            if (entry.startsWith(names[i])) {
                values[i] = entry.section(QLatin1Char(' '), -2, -2).toULongLong(&foundValues[i]);
            }
        }
    }
    memFile.close();
    bool found = true;
    for (int i = 0; found && i < nElems; ++i) {
        found = found && foundValues[i];
    }
    if (found) {
        /* MemFree + Buffers + Cached - SwapUsed =
         * = MemFree + Buffers + Cached - (SwapTotal - SwapFree) =
         * = MemFree + Buffers + Cached + SwapFree - SwapTotal */
        memoryFree = values[0] + values[1] + values[2] + values[3];
        if (values[4] > memoryFree) {
            memoryFree = 0;
        } else {
            memoryFree -= values[4];
        }
    } else {
        return 0;
    }

    cacheTimer.setRemainingTime(kFreeMemCacheTimeout);

    if (freeSwap) {
        *freeSwap = (cachedFreeSwap = (Q_UINT64_C(1024) * values[3]));
    }
    return (cachedValue = (Q_UINT64_C(1024) * memoryFree));
#elif defined(Q_OS_FREEBSD)
    qulonglong cache, inact, free, psize;
    size_t cachelen, inactlen, freelen, psizelen;
    cachelen = sizeof(cache);
    inactlen = sizeof(inact);
    freelen = sizeof(free);
    psizelen = sizeof(psize);
    // sum up inactive, cached and free memory
    if (sysctlbyname("vm.stats.vm.v_cache_count", &cache, &cachelen, NULL, 0) == 0 && sysctlbyname("vm.stats.vm.v_inactive_count", &inact, &inactlen, NULL, 0) == 0 &&
        sysctlbyname("vm.stats.vm.v_free_count", &free, &freelen, NULL, 0) == 0 && sysctlbyname("vm.stats.vm.v_page_size", &psize, &psizelen, NULL, 0) == 0) {
        cacheTimer.setRemainingTime(kFreeMemCacheTimeout);
        return (cachedValue = (cache + inact + free) * psize);
    } else {
        return 0;
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
    GlobalMemoryStatusEx(&stat);

    cacheTimer.setRemainingTime(kFreeMemCacheTimeout);

    if (freeSwap)
        *freeSwap = (cachedFreeSwap = stat.ullAvailPageFile);
    return (cachedValue = stat.ullAvailPhys);
#else
    // tell the memory is full.. will act as in LOW profile
    return 0;
#endif
}

bool DocumentPrivate::loadDocumentInfo(LoadDocumentInfoFlags loadWhat)
// note: load data and stores it internally (document or pages). observers
// are still uninitialized at this point so don't access them
{
    // qCDebug(OkularCoreDebug).nospace() << "Using '" << d->m_xmlFileName << "' as document info file.";
    if (m_xmlFileName.isEmpty()) {
        return false;
    }

    QFile infoFile(m_xmlFileName);
    return loadDocumentInfo(infoFile, loadWhat);
}

bool DocumentPrivate::loadDocumentInfo(QFile &infoFile, LoadDocumentInfoFlags loadWhat)
{
    if (!infoFile.exists() || !infoFile.open(QIODevice::ReadOnly)) {
        // Use the default layout provided by the generator
        if (loadWhat & LoadGeneralInfo) {
            Generator::PageLayout defaultViewMode = m_generator->defaultPageLayout();
            if (defaultViewMode == Generator::NoLayout) {
                return false;
            }

            for (View *view : std::as_const(m_views)) {
                setDefaultViewMode(view, defaultViewMode);
            }
        }
        return false;
    }

    // Load DOM from XML file
    QDomDocument doc(QStringLiteral("documentInfo"));
    if (!doc.setContent(&infoFile)) {
        qCDebug(OkularCoreDebug) << "Can't load XML pair! Check for broken xml.";
        infoFile.close();
        return false;
    }
    infoFile.close();

    QDomElement root = doc.documentElement();

    if (root.tagName() != QLatin1String("documentInfo")) {
        return false;
    }

    bool loadedAnything = false; // set if something gets actually loaded

    // Parse the DOM tree
    QDomNode topLevelNode = root.firstChild();
    while (topLevelNode.isElement()) {
        QString catName = topLevelNode.toElement().tagName();

        // Restore page attributes (bookmark, annotations, ...) from the DOM
        if (catName == QLatin1String("pageList") && (loadWhat & LoadPageInfo)) {
            QDomNode pageNode = topLevelNode.firstChild();
            while (pageNode.isElement()) {
                QDomElement pageElement = pageNode.toElement();
                if (pageElement.hasAttribute(QStringLiteral("number"))) {
                    // get page number (node's attribute)
                    bool ok;
                    int pageNumber = pageElement.attribute(QStringLiteral("number")).toInt(&ok);

                    // pass the domElement to the right page, to read config data from
                    if (ok && pageNumber >= 0 && pageNumber < (int)m_pagesVector.count()) {
                        if (m_pagesVector[pageNumber]->d->restoreLocalContents(pageElement)) {
                            loadedAnything = true;
                        }
                    }
                }
                pageNode = pageNode.nextSibling();
            }
        }

        // Restore 'general info' from the DOM
        else if (catName == QLatin1String("generalInfo") && (loadWhat & LoadGeneralInfo)) {
            QDomNode infoNode = topLevelNode.firstChild();
            while (infoNode.isElement()) {
                QDomElement infoElement = infoNode.toElement();

                // restore viewports history
                if (infoElement.tagName() == QLatin1String("history")) {
                    // clear history
                    m_viewportHistory.clear();
                    // append old viewports
                    QDomNode historyNode = infoNode.firstChild();
                    while (historyNode.isElement()) {
                        QDomElement historyElement = historyNode.toElement();
                        if (historyElement.hasAttribute(QStringLiteral("viewport"))) {
                            QString vpString = historyElement.attribute(QStringLiteral("viewport"));
                            m_viewportIterator = m_viewportHistory.insert(m_viewportHistory.end(), DocumentViewport(vpString));
                            loadedAnything = true;
                        }
                        historyNode = historyNode.nextSibling();
                    }
                    // consistency check
                    if (m_viewportHistory.empty()) {
                        m_viewportIterator = m_viewportHistory.insert(m_viewportHistory.end(), DocumentViewport());
                    }
                } else if (infoElement.tagName() == QLatin1String("rotation")) {
                    QString str = infoElement.text();
                    bool ok = true;
                    int newrotation = !str.isEmpty() ? (str.toInt(&ok) % 4) : 0;
                    if (ok && newrotation != 0) {
                        setRotationInternal(newrotation, false);
                        loadedAnything = true;
                    }
                } else if (infoElement.tagName() == QLatin1String("views")) {
                    QDomNode viewNode = infoNode.firstChild();
                    while (viewNode.isElement()) {
                        QDomElement viewElement = viewNode.toElement();
                        if (viewElement.tagName() == QLatin1String("view")) {
                            const QString viewName = viewElement.attribute(QStringLiteral("name"));
                            for (View *view : std::as_const(m_views)) {
                                if (view->name() == viewName) {
                                    loadViewsInfo(view, viewElement);
                                    loadedAnything = true;
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

    return loadedAnything;
}

void DocumentPrivate::loadViewsInfo(View *view, const QDomElement &e)
{
    QDomNode viewNode = e.firstChild();
    while (viewNode.isElement()) {
        QDomElement viewElement = viewNode.toElement();

        if (viewElement.tagName() == QLatin1String("zoom")) {
            const QString valueString = viewElement.attribute(QStringLiteral("value"));
            bool newzoom_ok = true;
            const double newzoom = !valueString.isEmpty() ? valueString.toDouble(&newzoom_ok) : 1.0;
            if (newzoom_ok && newzoom != 0 && view->supportsCapability(View::Zoom) && (view->capabilityFlags(View::Zoom) & (View::CapabilityRead | View::CapabilitySerializable))) {
                view->setCapability(View::Zoom, newzoom);
            }
            const QString modeString = viewElement.attribute(QStringLiteral("mode"));
            bool newmode_ok = true;
            const int newmode = !modeString.isEmpty() ? modeString.toInt(&newmode_ok) : 2;
            if (newmode_ok && view->supportsCapability(View::ZoomModality) && (view->capabilityFlags(View::ZoomModality) & (View::CapabilityRead | View::CapabilitySerializable))) {
                view->setCapability(View::ZoomModality, newmode);
            }
        } else if (viewElement.tagName() == QLatin1String("viewMode")) {
            const QString modeString = viewElement.attribute(QStringLiteral("mode"));
            bool newmode_ok = true;
            const int newmode = !modeString.isEmpty() ? modeString.toInt(&newmode_ok) : 2;
            if (newmode_ok && view->supportsCapability(View::ViewModeModality) && (view->capabilityFlags(View::ViewModeModality) & (View::CapabilityRead | View::CapabilitySerializable))) {
                view->setCapability(View::ViewModeModality, newmode);
            }
        } else if (viewElement.tagName() == QLatin1String("continuous")) {
            const QString modeString = viewElement.attribute(QStringLiteral("mode"));
            bool newmode_ok = true;
            const int newmode = !modeString.isEmpty() ? modeString.toInt(&newmode_ok) : 2;
            if (newmode_ok && view->supportsCapability(View::Continuous) && (view->capabilityFlags(View::Continuous) & (View::CapabilityRead | View::CapabilitySerializable))) {
                view->setCapability(View::Continuous, newmode);
            }
        } else if (viewElement.tagName() == QLatin1String("trimMargins")) {
            const QString valueString = viewElement.attribute(QStringLiteral("value"));
            bool newmode_ok = true;
            const int newmode = !valueString.isEmpty() ? valueString.toInt(&newmode_ok) : 2;
            if (newmode_ok && view->supportsCapability(View::TrimMargins) && (view->capabilityFlags(View::TrimMargins) & (View::CapabilityRead | View::CapabilitySerializable))) {
                view->setCapability(View::TrimMargins, newmode);
            }
        }

        viewNode = viewNode.nextSibling();
    }
}

void DocumentPrivate::setDefaultViewMode(View *view, Generator::PageLayout defaultViewMode)
{
    if (view->supportsCapability(View::ViewModeModality) && (view->capabilityFlags(View::ViewModeModality) & (View::CapabilityRead | View::CapabilitySerializable))) {
        view->setCapability(View::ViewModeModality, (int)defaultViewMode);
    }

    if (SettingsCore::useFileInfoForViewContinuous()) {
        if (view->supportsCapability(View::Continuous) && (view->capabilityFlags(View::Continuous) & (View::CapabilityRead | View::CapabilitySerializable))) {
            view->setCapability(View::Continuous, (int)m_generator->defaultPageContinuous());
        }
    }
}

void DocumentPrivate::saveViewsInfo(View *view, QDomElement &e) const
{
    if (view->supportsCapability(View::Zoom) && (view->capabilityFlags(View::Zoom) & (View::CapabilityRead | View::CapabilitySerializable)) && view->supportsCapability(View::ZoomModality) &&
        (view->capabilityFlags(View::ZoomModality) & (View::CapabilityRead | View::CapabilitySerializable))) {
        QDomElement zoomEl = e.ownerDocument().createElement(QStringLiteral("zoom"));
        e.appendChild(zoomEl);
        bool ok = true;
        const double zoom = view->capability(View::Zoom).toDouble(&ok);
        if (ok && zoom != 0) {
            zoomEl.setAttribute(QStringLiteral("value"), QString::number(zoom));
        }
        const int mode = view->capability(View::ZoomModality).toInt(&ok);
        if (ok) {
            zoomEl.setAttribute(QStringLiteral("mode"), mode);
        }
    }
    if (view->supportsCapability(View::Continuous) && (view->capabilityFlags(View::Continuous) & (View::CapabilityRead | View::CapabilitySerializable))) {
        QDomElement contEl = e.ownerDocument().createElement(QStringLiteral("continuous"));
        e.appendChild(contEl);
        const bool mode = view->capability(View::Continuous).toBool();
        contEl.setAttribute(QStringLiteral("mode"), mode);
    }
    if (view->supportsCapability(View::ViewModeModality) && (view->capabilityFlags(View::ViewModeModality) & (View::CapabilityRead | View::CapabilitySerializable))) {
        QDomElement viewEl = e.ownerDocument().createElement(QStringLiteral("viewMode"));
        e.appendChild(viewEl);
        bool ok = true;
        const int mode = view->capability(View::ViewModeModality).toInt(&ok);
        if (ok) {
            viewEl.setAttribute(QStringLiteral("mode"), mode);
        }
    }
    if (view->supportsCapability(View::TrimMargins) && (view->capabilityFlags(View::TrimMargins) & (View::CapabilityRead | View::CapabilitySerializable))) {
        QDomElement contEl = e.ownerDocument().createElement(QStringLiteral("trimMargins"));
        e.appendChild(contEl);
        const bool value = view->capability(View::TrimMargins).toBool();
        contEl.setAttribute(QStringLiteral("value"), value);
    }
}

QUrl DocumentPrivate::giveAbsoluteUrl(const QString &fileName) const
{
    if (!QDir::isRelativePath(fileName)) {
        return QUrl::fromLocalFile(fileName);
    }

    if (!m_url.isValid()) {
        return QUrl();
    }

    return QUrl(KIO::upUrl(m_url).toString() + fileName);
}

bool DocumentPrivate::openRelativeFile(const QString &fileName)
{
    const QUrl newUrl = giveAbsoluteUrl(fileName);
    if (newUrl.isEmpty()) {
        return false;
    }

    qCDebug(OkularCoreDebug).nospace() << "openRelativeFile: '" << newUrl << "'";

    Q_EMIT m_parent->openUrl(newUrl);
    return m_url == newUrl;
}

Generator *DocumentPrivate::loadGeneratorLibrary(const KPluginMetaData &service)
{
    const auto result = KPluginFactory::instantiatePlugin<Okular::Generator>(service);

    if (!result) {
        qCWarning(OkularCoreDebug).nospace() << "Failed to load plugin " << service.fileName() << ": " << result.errorText;
        return nullptr;
    }

    GeneratorInfo info(result.plugin, service);
    m_loadedGenerators.insert(service.pluginId(), info);
    return result.plugin;
}

void DocumentPrivate::loadAllGeneratorLibraries()
{
    if (m_generatorsLoaded) {
        return;
    }

    loadServiceList(availableGenerators());

    m_generatorsLoaded = true;
}

void DocumentPrivate::loadServiceList(const QList<KPluginMetaData> &offers)
{
    int count = offers.count();
    if (count <= 0) {
        return;
    }

    for (const auto &offer : std::as_const(offers)) {
        auto id = offer.pluginId();
        // don't load already loaded generators
        if (m_loadedGenerators.contains(id)) {
            continue;
        }

        auto *g = loadGeneratorLibrary(offer);
        (void)g;
    }
}

void DocumentPrivate::unloadGenerator(GeneratorInfo &info)
{
    delete info.generator;
    info.generator = nullptr;
}

void DocumentPrivate::cacheExportFormats()
{
    if (m_exportCached) {
        return;
    }

    const auto formats = m_generator->exportFormats();
    for (const auto &format : formats) {
        if (format.mimeType().name() == QLatin1String("text/plain")) {
            m_exportToText = format;
        } else {
            m_exportFormats.append(format);
        }
    }

    m_exportCached = true;
}

ConfigInterface *DocumentPrivate::generatorConfig(GeneratorInfo &info)
{
    if (info.configChecked) {
        return info.config;
    }

    info.config = qobject_cast<Okular::ConfigInterface *>(info.generator);
    info.configChecked = true;
    return info.config;
}

SaveInterface *DocumentPrivate::generatorSave(GeneratorInfo &info)
{
    if (info.saveChecked) {
        return info.save;
    }

    info.save = qobject_cast<Okular::SaveInterface *>(info.generator);
    info.saveChecked = true;
    return info.save;
}

Document::OpenResult DocumentPrivate::openDocumentInternal(const KPluginMetaData &offer, bool isstdin, const QString &docFile, const QByteArray &filedata, const QString &password)
{
    QString propName = offer.pluginId();
    QHash<QString, GeneratorInfo>::const_iterator genIt = m_loadedGenerators.constFind(propName);
    m_walletGenerator = nullptr;
    if (genIt != m_loadedGenerators.constEnd()) {
        m_generator = genIt.value().generator;
    } else {
        m_generator = loadGeneratorLibrary(offer);
        if (!m_generator) {
            return Document::OpenError;
        }
        genIt = m_loadedGenerators.constFind(propName);
        Q_ASSERT(genIt != m_loadedGenerators.constEnd());
    }
    Q_ASSERT_X(m_generator, "Document::load()", "null generator?!");

    m_generator->d_func()->m_document = this;

    // connect error reporting signals
    m_openError.clear();
    QMetaObject::Connection errorToOpenErrorConnection = QObject::connect(m_generator, &Generator::error, m_parent, [this](const QString &message) { m_openError = message; });
    QObject::connect(m_generator, &Generator::warning, m_parent, &Document::warning);
    QObject::connect(m_generator, &Generator::notice, m_parent, &Document::notice);

    QApplication::setOverrideCursor(Qt::WaitCursor);

    const QWindow *window = m_widget && m_widget->window() ? m_widget->window()->windowHandle() : nullptr;
    const QSizeF dpi = Utils::realDpi(window);
    qCDebug(OkularCoreDebug) << "Output DPI:" << dpi;
    m_generator->setDPI(dpi);

    Document::OpenResult openResult = Document::OpenError;
    if (!isstdin) {
        openResult = m_generator->loadDocumentWithPassword(docFile, m_pagesVector, password);
    } else if (!filedata.isEmpty()) {
        if (m_generator->hasFeature(Generator::ReadRawData)) {
            openResult = m_generator->loadDocumentFromDataWithPassword(filedata, m_pagesVector, password);
        } else {
            m_tempFile = new QTemporaryFile();
            if (!m_tempFile->open()) {
                delete m_tempFile;
                m_tempFile = nullptr;
            } else {
                m_tempFile->write(filedata);
                QString tmpFileName = m_tempFile->fileName();
                m_tempFile->close();
                openResult = m_generator->loadDocumentWithPassword(tmpFileName, m_pagesVector, password);
            }
        }
    }

    QApplication::restoreOverrideCursor();
    if (openResult != Document::OpenSuccess || m_pagesVector.size() <= 0) {
        m_generator->d_func()->m_document = nullptr;
        QObject::disconnect(m_generator, nullptr, m_parent, nullptr);

        // TODO this is a bit of a hack, since basically means that
        // you can only call walletDataForFile after calling openDocument
        // but since in reality it's what happens I've decided not to refactor/break API
        // One solution is just kill walletDataForFile and make OpenResult be an object
        // where the wallet data is also returned when OpenNeedsPassword
        m_walletGenerator = m_generator;
        m_generator = nullptr;

        qDeleteAll(m_pagesVector);
        m_pagesVector.clear();
        delete m_tempFile;
        m_tempFile = nullptr;

        // TODO: Q_EMIT a message telling the document is empty
        if (openResult == Document::OpenSuccess) {
            openResult = Document::OpenError;
        }
    } else {
        /*
         *  Now that the documen is opened, the tab (if using tabs) is visible, which mean that
         * we can now connect the error reporting signal directly to the parent
         */

        QObject::disconnect(errorToOpenErrorConnection);
        QObject::connect(m_generator, &Generator::error, m_parent, &Document::error);
    }

    return openResult;
}

bool DocumentPrivate::savePageDocumentInfo(QTemporaryFile *infoFile, int what) const
{
    if (infoFile->open()) {
        // 1. Create DOM
        QDomDocument doc(QStringLiteral("documentInfo"));
        QDomProcessingInstruction xmlPi = doc.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"utf-8\""));
        doc.appendChild(xmlPi);
        QDomElement root = doc.createElement(QStringLiteral("documentInfo"));
        doc.appendChild(root);

        // 2.1. Save page attributes (bookmark state, annotations, ... ) to DOM
        QDomElement pageList = doc.createElement(QStringLiteral("pageList"));
        root.appendChild(pageList);
        // <page list><page number='x'>.... </page> save pages that hold data
        for (Page *const page : m_pagesVector) {
            page->d->saveLocalContents(pageList, doc, PageItems(what));
        }

        // 3. Save DOM to XML file
        QString xml = doc.toString();

        QTextStream os(infoFile);
        os.setEncoding(QStringConverter::Utf8);
        os << xml;
        return true;
    }
    return false;
}

DocumentViewport DocumentPrivate::nextDocumentViewport() const
{
    DocumentViewport ret = m_nextDocumentViewport;
    if (!m_nextDocumentDestination.isEmpty() && m_generator) {
        DocumentViewport vp(m_parent->metaData(QStringLiteral("NamedViewport"), m_nextDocumentDestination).toString());
        if (vp.isValid()) {
            ret = vp;
        }
    }
    return ret;
}

void DocumentPrivate::performAddPageAnnotation(int page, Annotation *annotation)
{
    Okular::SaveInterface *iface = qobject_cast<Okular::SaveInterface *>(m_generator);
    AnnotationProxy *proxy = iface ? iface->annotationProxy() : nullptr;

    // find out the page to attach annotation
    Page *kp = m_pagesVector[page];
    if (!m_generator || !kp) {
        return;
    }

    // the annotation belongs already to a page
    if (annotation->d_ptr->m_page) {
        return;
    }

    // add annotation to the page
    kp->addAnnotation(annotation);

    // tell the annotation proxy
    if (proxy && proxy->supports(AnnotationProxy::Addition)) {
        proxy->notifyAddition(annotation, page);
    }

    // notify observers about the change
    notifyAnnotationChanges(page);

    if (annotation->flags() & Annotation::ExternallyDrawn) {
        // Redraw everything, including ExternallyDrawn annotations
        refreshPixmaps(page);
    }
}

void DocumentPrivate::performRemovePageAnnotation(int page, Annotation *annotation)
{
    Okular::SaveInterface *iface = qobject_cast<Okular::SaveInterface *>(m_generator);
    AnnotationProxy *proxy = iface ? iface->annotationProxy() : nullptr;
    bool isExternallyDrawn;

    // find out the page
    Page *kp = m_pagesVector[page];
    if (!m_generator || !kp) {
        return;
    }

    if (annotation->flags() & Annotation::ExternallyDrawn) {
        isExternallyDrawn = true;
    } else {
        isExternallyDrawn = false;
    }

    // try to remove the annotation
    if (m_parent->canRemovePageAnnotation(annotation)) {
        // tell the annotation proxy
        if (proxy && proxy->supports(AnnotationProxy::Removal)) {
            proxy->notifyRemoval(annotation, page);
        }

        kp->removeAnnotation(annotation); // Also destroys the object

        // in case of success, notify observers about the change
        notifyAnnotationChanges(page);

        if (isExternallyDrawn) {
            // Redraw everything, including ExternallyDrawn annotations
            refreshPixmaps(page);
        }
    }
}

void DocumentPrivate::performModifyPageAnnotation(int page, Annotation *annotation, bool appearanceChanged)
{
    Okular::SaveInterface *iface = qobject_cast<Okular::SaveInterface *>(m_generator);
    AnnotationProxy *proxy = iface ? iface->annotationProxy() : nullptr;

    // find out the page
    const Page *kp = m_pagesVector[page];
    if (!m_generator || !kp) {
        return;
    }

    // tell the annotation proxy
    if (proxy && proxy->supports(AnnotationProxy::Modification)) {
        proxy->notifyModification(annotation, page, appearanceChanged);
    }

    // notify observers about the change
    notifyAnnotationChanges(page);
    if (appearanceChanged && (annotation->flags() & Annotation::ExternallyDrawn)) {
        /* When an annotation is being moved, the generator will not render it.
         * Therefore there's no need to refresh pixmaps after the first time */
        if (annotation->flags() & (Annotation::BeingMoved | Annotation::BeingResized)) {
            if (m_annotationBeingModified) {
                return;
            } else { // First time: take note
                m_annotationBeingModified = true;
            }
        } else {
            m_annotationBeingModified = false;
        }

        // Redraw everything, including ExternallyDrawn annotations
        qCDebug(OkularCoreDebug) << "Refreshing Pixmaps";
        refreshPixmaps(page);
    }
}

void DocumentPrivate::performSetAnnotationContents(const QString &newContents, Annotation *annot, int pageNumber)
{
    bool appearanceChanged = false;

    // Check if appearanceChanged should be true
    switch (annot->subType()) {
    // If it's an in-place TextAnnotation, set the inplace text
    case Okular::Annotation::AText: {
        const Okular::TextAnnotation *txtann = static_cast<Okular::TextAnnotation *>(annot);
        if (txtann->textType() == Okular::TextAnnotation::InPlace) {
            appearanceChanged = true;
        }
        break;
    }
    // If it's a LineAnnotation, check if caption text is visible
    case Okular::Annotation::ALine: {
        const Okular::LineAnnotation *lineann = static_cast<Okular::LineAnnotation *>(annot);
        if (lineann->showCaption()) {
            appearanceChanged = true;
        }
        break;
    }
    default:
        break;
    }

    // Set contents
    annot->setContents(newContents);

    // Tell the document the annotation has been modified
    performModifyPageAnnotation(pageNumber, annot, appearanceChanged);
}

void DocumentPrivate::recalculateForms()
{
    const QVariant fco = m_parent->metaData(QStringLiteral("FormCalculateOrder"));
    const QList<int> formCalculateOrder = fco.value<QList<int>>();
    for (int formId : formCalculateOrder) {
        for (Page *const page : std::as_const(m_pagesVector)) {
            if (page) {
                bool pageNeedsRefresh = false;
                const QList<Okular::FormField *> forms = page->formFields();
                for (FormField *form : forms) {
                    if (form->id() == formId) {
                        const Action *action = form->additionalAction(FormField::CalculateField);
                        if (action) {
                            std::shared_ptr<Event> event;
                            if (dynamic_cast<FormFieldText *>(form) || dynamic_cast<FormFieldChoice *>(form)) {
                                // Prepare text calculate event
                                event = Event::createFormCalculateEvent(form, page);
                                const ScriptAction *linkscript = static_cast<const ScriptAction *>(action);
                                executeScriptEvent(event, linkscript);
                                // The value maybe changed in javascript so save it first.
                                QString oldVal = form->value().toString();

                                if (event) {
                                    // Update text field from calculate
                                    const QString newVal = event->value().toString();
                                    if (newVal != oldVal) {
                                        form->setValue(QVariant(newVal));
                                        form->setAppearanceValue(QVariant(newVal));
                                        bool returnCode = true;
                                        if (form->additionalAction(Okular::FormField::FieldModified) && !form->isReadOnly()) {
                                            m_parent->processKeystrokeCommitAction(form->additionalAction(Okular::FormField::FieldModified), form, returnCode);
                                        }
                                        if (const Okular::Action *validateAction = form->additionalAction(Okular::FormField::ValidateField)) {
                                            if (returnCode) {
                                                m_parent->processValidateAction(validateAction, form, returnCode);
                                            }
                                        }
                                        if (!returnCode) {
                                            continue;
                                        } else {
                                            form->commitValue(form->value().toString());
                                        }
                                        if (const Okular::Action *formatAction = form->additionalAction(Okular::FormField::FormatField)) {
                                            // The format action handles the refresh.
                                            m_parent->processFormatAction(formatAction, form);
                                        } else {
                                            form->commitFormattedValue(form->value().toString());
                                            Q_EMIT m_parent->refreshFormWidget(form);
                                            pageNeedsRefresh = true;
                                        }
                                    }
                                }
                            }
                        } else {
                            qWarning() << "Form that is part of calculate order doesn't have a calculate action";
                        }
                    }
                }
                if (pageNeedsRefresh) {
                    refreshPixmaps(page->number());
                }
            }
        }
    }
}

void DocumentPrivate::saveDocumentInfo() const
{
    if (m_xmlFileName.isEmpty()) {
        return;
    }

    QFile infoFile(m_xmlFileName);
    qCDebug(OkularCoreDebug) << "About to save document info to" << m_xmlFileName;
    if (!infoFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(OkularCoreDebug) << "Failed to open docdata file" << m_xmlFileName;
        return;
    }
    // 1. Create DOM
    QDomDocument doc(QStringLiteral("documentInfo"));
    QDomProcessingInstruction xmlPi = doc.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"utf-8\""));
    doc.appendChild(xmlPi);
    QDomElement root = doc.createElement(QStringLiteral("documentInfo"));
    root.setAttribute(QStringLiteral("url"), m_url.toDisplayString(QUrl::PreferLocalFile));
    doc.appendChild(root);

    // 2.1. Save page attributes (bookmark state, annotations, ... ) to DOM
    //  -> do this if there are not-yet-migrated annots or forms in docdata/
    if (m_docdataMigrationNeeded) {
        QDomElement pageList = doc.createElement(QStringLiteral("pageList"));
        root.appendChild(pageList);
        // OriginalAnnotationPageItems and OriginalFormFieldPageItems tell to
        // store the same unmodified annotation list and form contents that we
        // read when we opened the file and ignore any change made by the user.
        // Since we don't store annotations and forms in docdata/ any more, this is
        // necessary to preserve annotations/forms that previous Okular version
        // had stored there.
        const PageItems saveWhat = AllPageItems | OriginalAnnotationPageItems | OriginalFormFieldPageItems;
        // <page list><page number='x'>.... </page> save pages that hold data
        for (Page *const page : std::as_const(m_pagesVector)) {
            page->d->saveLocalContents(pageList, doc, saveWhat);
        }
    }

    // 2.2. Save document info (current viewport, history, ... ) to DOM
    QDomElement generalInfo = doc.createElement(QStringLiteral("generalInfo"));
    root.appendChild(generalInfo);
    // create rotation node
    if (m_rotation != Rotation0) {
        QDomElement rotationNode = doc.createElement(QStringLiteral("rotation"));
        generalInfo.appendChild(rotationNode);
        rotationNode.appendChild(doc.createTextNode(QString::number((int)m_rotation)));
    }
    // <general info><history> ... </history> save history up to OKULAR_HISTORY_SAVEDSTEPS viewports
    const auto currentViewportIterator = std::list<DocumentViewport>::const_iterator(m_viewportIterator);
    std::list<DocumentViewport>::const_iterator backIterator = currentViewportIterator;
    if (backIterator != m_viewportHistory.end()) {
        // go back up to OKULAR_HISTORY_SAVEDSTEPS steps from the current viewportIterator
        int backSteps = OKULAR_HISTORY_SAVEDSTEPS;
        while (backSteps-- && backIterator != m_viewportHistory.begin()) {
            --backIterator;
        }

        // create history root node
        QDomElement historyNode = doc.createElement(QStringLiteral("history"));
        generalInfo.appendChild(historyNode);

        // add old[backIterator] and present[viewportIterator] items
        auto endIt = currentViewportIterator;
        ++endIt;
        while (backIterator != endIt) {
            QString name = (backIterator == currentViewportIterator) ? QStringLiteral("current") : QStringLiteral("oldPage");
            QDomElement historyEntry = doc.createElement(name);
            historyEntry.setAttribute(QStringLiteral("viewport"), (*backIterator).toString());
            historyNode.appendChild(historyEntry);
            ++backIterator;
        }
    }
    // create views root node
    QDomElement viewsNode = doc.createElement(QStringLiteral("views"));
    generalInfo.appendChild(viewsNode);
    for (View *view : std::as_const(m_views)) {
        QDomElement viewEntry = doc.createElement(QStringLiteral("view"));
        viewEntry.setAttribute(QStringLiteral("name"), view->name());
        viewsNode.appendChild(viewEntry);
        saveViewsInfo(view, viewEntry);
    }

    // 3. Save DOM to XML file
    QString xml = doc.toString();
    QTextStream os(&infoFile);
    os.setEncoding(QStringConverter::Utf8);
    os << xml;
    infoFile.close();
}

void DocumentPrivate::slotTimedMemoryCheck()
{
    // [MEM] clean memory (for 'free mem dependent' profiles only)
    if (SettingsCore::memoryLevel() != SettingsCore::EnumMemoryLevel::Low && m_allocatedPixmapsTotalMemory > 1024 * 1024) {
        cleanupPixmapMemory();
    }
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
    if (memoryToFree) {
        const AllocatedPixmap *pixmapToReplace = searchLowestPriorityPixmap(true);
        if (pixmapToReplace) {
            maxDistance = qAbs(pixmapToReplace->page - currentViewportPage);
        }
    }

    // find a request
    PixmapRequest *request = nullptr;
    m_pixmapRequestsMutex.lock();
    while (!m_pixmapRequestsStack.empty() && !request) {
        PixmapRequest *r = m_pixmapRequestsStack.back();
        if (!r) {
            m_pixmapRequestsStack.pop_back();
            continue;
        }

        QRect requestRect = r->isTile() ? r->normalizedRect().geometry(r->width(), r->height()) : QRect(0, 0, r->width(), r->height());
        TilesManager *tilesManager = r->d->tilesManager();
        const double normalizedArea = r->normalizedRect().width() * r->normalizedRect().height();
        const QScreen *screen = nullptr;
        if (m_widget) {
            const QWindow *window = m_widget->window()->windowHandle();
            if (window) {
                screen = window->screen();
            }
        }
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
        const long screenSize = screen->devicePixelRatio() * screen->size().width() * screen->devicePixelRatio() * screen->size().height();

        // Make sure the page is the right size to receive the pixmap
        r->page()->setPageSize(r->observer(), r->width(), r->height());

        // If it's a preload but the generator is not threaded no point in trying to preload
        if (r->preload() && !m_generator->hasFeature(Generator::Threaded)) {
            m_pixmapRequestsStack.pop_back();
            delete r;
        }
        // request only if page isn't already present and request has valid id
        else if ((!r->d->mForce && r->page()->hasPixmap(r->observer(), r->width(), r->height(), r->normalizedRect())) || !m_observers.contains(r->observer())) {
            m_pixmapRequestsStack.pop_back();
            delete r;
        } else if (!r->d->mForce && r->preload() && qAbs(r->pageNumber() - currentViewportPage) >= maxDistance) {
            m_pixmapRequestsStack.pop_back();
            // qCDebug(OkularCoreDebug) << "Ignoring request that doesn't fit in cache";
            delete r;
        }
        // Ignore requests for pixmaps that are already being generated
        else if (tilesManager && tilesManager->isRequesting(r->normalizedRect(), r->width(), r->height())) {
            m_pixmapRequestsStack.pop_back();
            delete r;
        }
        // If the requested area is above 4*screenSize pixels, and we're not rendering most of the page,  switch on the tile manager
        else if (!tilesManager && m_generator->hasFeature(Generator::TiledRendering) && (long)r->width() * (long)r->height() > 4L * screenSize && normalizedArea < 0.75) {
            // if the image is too big. start using tiles
            qCDebug(OkularCoreDebug).nospace() << "Start using tiles on page " << r->pageNumber() << " (" << r->width() << "x" << r->height() << " px);";

            // fill the tiles manager with the last rendered pixmap
            const QPixmap *pixmap = r->page()->_o_nearestPixmap(r->observer(), r->width(), r->height());
            if (pixmap) {
                tilesManager = new TilesManager(r->pageNumber(), pixmap->width(), pixmap->height(), r->page()->rotation());
                tilesManager->setPixmap(pixmap, NormalizedRect(0, 0, 1, 1), true /*isPartialPixmap*/);
                tilesManager->setSize(r->width(), r->height());
            } else {
                // create new tiles manager
                tilesManager = new TilesManager(r->pageNumber(), r->width(), r->height(), r->page()->rotation());
            }
            tilesManager->setRequest(r->normalizedRect(), r->width(), r->height());
            r->page()->deletePixmap(r->observer());
            r->page()->d->setTilesManager(r->observer(), tilesManager);
            r->setTile(true);

            // Change normalizedRect to the smallest rect that contains all
            // visible tiles.
            if (!r->normalizedRect().isNull()) {
                NormalizedRect tilesRect;
                const auto tiles = tilesManager->tilesAt(r->normalizedRect(), TilesManager::TerminalTile);
                for (const Tile &tile : tiles) {
                    if (tilesRect.isNull()) {
                        tilesRect = tile.rect();
                    } else {
                        tilesRect |= tile.rect();
                    }
                }

                r->setNormalizedRect(tilesRect);
                request = r;
            } else {
                // Discard request if normalizedRect is null. This happens in
                // preload requests issued by PageView if the requested page is
                // not visible and the user has just switched from a non-tiled
                // zoom level to a tiled one
                m_pixmapRequestsStack.pop_back();
                delete r;
            }
        }
        // If the requested area is below 3*screenSize pixels, switch off the tile manager
        else if (tilesManager && (long)r->width() * (long)r->height() < 3L * screenSize) {
            qCDebug(OkularCoreDebug).nospace() << "Stop using tiles on page " << r->pageNumber() << " (" << r->width() << "x" << r->height() << " px);";

            // page is too small. stop using tiles.
            r->page()->deletePixmap(r->observer());
            r->setTile(false);

            request = r;
        } else if ((long)requestRect.width() * (long)requestRect.height() > 100L * screenSize && (SettingsCore::memoryLevel() != SettingsCore::EnumMemoryLevel::Greedy)) {
            m_pixmapRequestsStack.pop_back();
            if (!m_warnedOutOfMemory) {
                qCWarning(OkularCoreDebug).nospace() << "Running out of memory on page " << r->pageNumber() << " (" << r->width() << "x" << r->height() << " px);";
                qCWarning(OkularCoreDebug) << "this message will be reported only once.";
                m_warnedOutOfMemory = true;
            }
            delete r;
        } else {
            request = r;
        }
    }

    // if no request found (or already generated), return
    if (!request) {
        m_pixmapRequestsMutex.unlock();
        return;
    }

    // [MEM] preventive memory freeing
    qulonglong pixmapBytes = 0;
    TilesManager *tm = request->d->tilesManager();
    if (tm) {
        pixmapBytes = tm->totalMemory();
    } else {
        pixmapBytes = 4 * qulonglong(request->width()) * request->height();
    }

    if (pixmapBytes > (1024 * 1024)) {
        cleanupPixmapMemory(memoryToFree /* previously calculated value */);
    }

    // submit the request to the generator
    if (m_generator->canGeneratePixmap()) {
        QRect requestRect = !request->isTile() ? QRect(0, 0, request->width(), request->height()) : request->normalizedRect().geometry(request->width(), request->height());
        qCDebug(OkularCoreDebug).nospace() << "sending request observer=" << request->observer() << " " << requestRect.width() << "x" << requestRect.height() << "@" << request->pageNumber() << " async == " << request->asynchronous()
                                           << " isTile == " << request->isTile();
        m_pixmapRequestsStack.remove(request);

        if (tm) {
            tm->setRequest(request->normalizedRect(), request->width(), request->height());
        }

        if ((int)m_rotation % 2) {
            request->d->swap();
        }

        if (m_rotation != Rotation0 && !request->normalizedRect().isNull()) {
            request->setNormalizedRect(TilesManager::fromRotatedRect(request->normalizedRect(), m_rotation));
        }

        // If set elsewhere we already know we want it to be partial
        if (!request->partialUpdatesWanted()) {
            request->setPartialUpdatesWanted(request->asynchronous() && !request->page()->hasPixmap(request->observer()));
        }

        // we always have to unlock _before_ the generatePixmap() because
        // a sync generation would end with requestDone() -> deadlock, and
        // we can not really know if the generator can do async requests
        m_executingPixmapRequests.push_back(request);
        m_pixmapRequestsMutex.unlock();
        m_generator->generatePixmap(request);
    } else {
        m_pixmapRequestsMutex.unlock();
        // pino (7/4/2006): set the polling interval from 10 to 30
        QTimer::singleShot(30, m_parent, [this] { sendGeneratorPixmapRequest(); });
    }
}

void DocumentPrivate::rotationFinished(int page, Okular::Page *okularPage)
{
    const Okular::Page *wantedPage = m_pagesVector.value(page, nullptr);
    if (!wantedPage || wantedPage != okularPage) {
        return;
    }

    for (DocumentObserver *o : std::as_const(m_observers)) {
        o->notifyPageChanged(page, DocumentObserver::Pixmap | DocumentObserver::Annotations);
    }
}

void DocumentPrivate::slotFontReadingProgress(int page)
{
    Q_EMIT m_parent->fontReadingProgress(page);

    if (page >= (int)m_parent->pages() - 1) {
        Q_EMIT m_parent->fontReadingEnded();
        m_fontThread = nullptr;
        m_fontsCached = true;
    }
}

void DocumentPrivate::fontReadingGotFont(const Okular::FontInfo &font)
{
    // Try to avoid duplicate fonts
    if (m_fontsCache.indexOf(font) == -1) {
        m_fontsCache.append(font);

        Q_EMIT m_parent->gotFont(font);
    }
}

void DocumentPrivate::slotGeneratorConfigChanged()
{
    if (!m_generator) {
        return;
    }

    // reparse generator config and if something changed clear Pages
    bool configchanged = false;
    for (const auto &[key, value] : m_loadedGenerators.asKeyValueRange()) {
        Okular::ConfigInterface *iface = generatorConfig(value);
        if (iface) {
            bool it_changed = iface->reparseConfig();
            if (it_changed && (m_generator == value.generator)) {
                configchanged = true;
            }
        }
    }
    if (configchanged) {
        // invalidate pixmaps
        for (Page *const page : std::as_const(m_pagesVector)) {
            page->deletePixmaps();
        }

        // [MEM] remove allocation descriptors
        qDeleteAll(m_allocatedPixmaps);
        m_allocatedPixmaps.clear();
        m_allocatedPixmapsTotalMemory = 0;

        // send reload signals to observers
        foreachObserverD(notifyContentsCleared(DocumentObserver::Pixmap));
    }

    // free memory if in 'low' profile
    if (SettingsCore::memoryLevel() == SettingsCore::EnumMemoryLevel::Low && !m_allocatedPixmaps.empty() && !m_pagesVector.isEmpty()) {
        cleanupPixmapMemory();
    }
}

void DocumentPrivate::refreshPixmaps(int pageNumber)
{
    Page *page = m_pagesVector.value(pageNumber, nullptr);
    if (!page) {
        return;
    }

    QList<Okular::PixmapRequest *> pixmapsToRequest;
    for (const auto &[key, value] : page->d->m_pixmaps.asKeyValueRange()) {
        const QSize size = value.m_pixmap->size();
        PixmapRequest *p = new PixmapRequest(key, pageNumber, size.width(), size.height(), 1 /* dpr */, 1, PixmapRequest::Asynchronous);
        p->d->mForce = true;
        pixmapsToRequest << p;
    }

    // Need to do this ↑↓ in two steps since requestPixmaps can end up calling cancelRenderingBecauseOf
    // which changes m_pixmaps and thus breaks the loop above
    for (PixmapRequest *pr : std::as_const(pixmapsToRequest)) {
        const QList<Okular::PixmapRequest *> requestedPixmaps {pr};
        // TODO: Can it be called directly with the list, without the loop?
        m_parent->requestPixmaps(requestedPixmaps, Okular::Document::NoOption);
    }

    for (DocumentObserver *observer : std::as_const(m_observers)) {
        QList<Okular::PixmapRequest *> requestedPixmaps;

        TilesManager *tilesManager = page->d->tilesManager(observer);
        if (tilesManager) {
            tilesManager->markDirty();

            PixmapRequest *p = new PixmapRequest(observer, pageNumber, tilesManager->width(), tilesManager->height(), 1 /* dpr */, 1, PixmapRequest::Asynchronous);

            // Get the visible page rect
            NormalizedRect visibleRect;
            for (const auto *it : std::as_const(m_pageRects)) {
                if (it->pageNumber == pageNumber) {
                    visibleRect = it->rect;
                    break;
                }
            }

            if (!visibleRect.isNull()) {
                p->setNormalizedRect(visibleRect);
                p->setTile(true);
                p->d->mForce = true;
                requestedPixmaps.push_back(p);
            } else {
                delete p;
            }
        }

        m_parent->requestPixmaps(requestedPixmaps, Okular::Document::NoOption);
    }
}

void DocumentPrivate::_o_configChanged()
{
    // free text pages if needed
    calculateMaxTextPages();
    while (m_allocatedTextPagesFifo.count() > m_maxAllocatedTextPages) {
        int pageToKick = m_allocatedTextPagesFifo.takeFirst();
        m_pagesVector.at(pageToKick)->setTextPage(nullptr); // deletes the textpage
    }
}

void DocumentPrivate::doContinueDirectionMatchSearch(DoContinueDirectionMatchSearchStruct *searchStruct)
{
    RunningSearch *search = m_searches.value(searchStruct->searchID);

    if ((m_searchCancelled && !searchStruct->match) || !search) {
        // if the user cancelled but he just got a match, give him the match!
        QApplication::restoreOverrideCursor();

        if (search) {
            search->isCurrentlySearching = false;
        }

        Q_EMIT m_parent->searchFinished(searchStruct->searchID, Document::SearchCancelled);
        delete searchStruct->pagesToNotify;
        delete searchStruct;
        return;
    }

    const bool forward = search->cachedType == Document::NextMatch;
    bool doContinue = false;
    // if no match found, loop through the whole doc, starting from currentPage
    if (!searchStruct->match) {
        const int pageCount = m_pagesVector.count();
        if (search->pagesDone < pageCount) {
            doContinue = true;
            if (searchStruct->currentPage >= pageCount) {
                searchStruct->currentPage = 0;
                Q_EMIT m_parent->notice(i18n("Continuing search from beginning"), 3000);
            } else if (searchStruct->currentPage < 0) {
                searchStruct->currentPage = pageCount - 1;
                Q_EMIT m_parent->notice(i18n("Continuing search from bottom"), 3000);
            }
        }
    }

    if (doContinue) {
        // get page
        const Page *page = m_pagesVector[searchStruct->currentPage];
        // request search page if needed
        if (!page->hasTextPage()) {
            m_parent->requestTextPage(page->number());
        }

        // if found a match on the current page, end the loop
        searchStruct->match = page->findText(searchStruct->searchID, search->cachedString, forward ? FromTop : FromBottom, search->cachedCaseSensitivity);
        if (!searchStruct->match) {
            if (forward) {
                searchStruct->currentPage++;
            } else {
                searchStruct->currentPage--;
            }
            search->pagesDone++;
        } else {
            search->pagesDone = 1;
        }

        // Both of the previous if branches need to call doContinueDirectionMatchSearch
        QTimer::singleShot(0, m_parent, [this, searchStruct] { doContinueDirectionMatchSearch(searchStruct); });
    } else {
        doProcessSearchMatch(searchStruct->match, search, searchStruct->pagesToNotify, searchStruct->currentPage, searchStruct->searchID, search->cachedViewportMove, search->cachedColor);
        delete searchStruct;
    }
}

void DocumentPrivate::doProcessSearchMatch(RegularAreaRect *match, RunningSearch *search, QSet<int> *pagesToNotify, int currentPage, int searchID, bool moveViewport, const QColor &color)
{
    // reset cursor to previous shape
    QApplication::restoreOverrideCursor();

    bool foundAMatch = false;

    search->isCurrentlySearching = false;

    // if a match has been found..
    if (match) {
        // update the RunningSearch structure adding this match..
        foundAMatch = true;
        search->continueOnPage = currentPage;
        search->continueOnMatch = *match;
        search->highlightedPages.insert(currentPage);
        // ..add highlight to the page..
        m_pagesVector[currentPage]->d->setHighlight(searchID, match, color);

        // ..queue page for notifying changes..
        pagesToNotify->insert(currentPage);

        // Create a normalized rectangle around the search match that includes a 5% buffer on all sides.
        const Okular::NormalizedRect matchRectWithBuffer = Okular::NormalizedRect(match->first().left - 0.05, match->first().top - 0.05, match->first().right + 0.05, match->first().bottom + 0.05);

        const bool matchRectFullyVisible = isNormalizedRectangleFullyVisible(matchRectWithBuffer, currentPage);

        // ..move the viewport to show the first of the searched word sequence centered
        if (moveViewport && !matchRectFullyVisible) {
            DocumentViewport searchViewport(currentPage);
            searchViewport.rePos.enabled = true;
            searchViewport.rePos.normalizedX = (match->first().left + match->first().right) / 2.0;
            searchViewport.rePos.normalizedY = (match->first().top + match->first().bottom) / 2.0;
            m_parent->setViewport(searchViewport, nullptr, true);
        }
        delete match;
    }

    // notify observers about highlights changes
    for (int pageNumber : std::as_const(*pagesToNotify)) {
        for (DocumentObserver *observer : std::as_const(m_observers)) {
            observer->notifyPageChanged(pageNumber, DocumentObserver::Highlights);
        }
    }

    if (foundAMatch) {
        Q_EMIT m_parent->searchFinished(searchID, Document::MatchFound);
    } else {
        Q_EMIT m_parent->searchFinished(searchID, Document::NoMatchFound);
    }

    delete pagesToNotify;
}

void DocumentPrivate::doContinueAllDocumentSearch(QSet<int> *pagesToNotify, QHash<Page *, QList<RegularAreaRect *>> *pageMatches, int currentPage, int searchID)
{
    RunningSearch *search = m_searches.value(searchID);

    if (m_searchCancelled || !search) {
        typedef QList<RegularAreaRect *> Matches;

        QApplication::restoreOverrideCursor();

        if (search) {
            search->isCurrentlySearching = false;
        }

        Q_EMIT m_parent->searchFinished(searchID, Document::SearchCancelled);
        for (const Matches &mv : std::as_const(*pageMatches)) {
            qDeleteAll(mv);
        }
        delete pageMatches;
        delete pagesToNotify;
        return;
    }

    if (currentPage < m_pagesVector.count()) {
        // get page (from the first to the last)
        Page *page = m_pagesVector.at(currentPage);

        // request search page if needed
        if (!page->hasTextPage()) {
            int pageNumber = page->number(); // redundant? is it == currentPage ?
            m_parent->requestTextPage(pageNumber);
        }

        // loop on a page adding highlights for all found items
        RegularAreaRect *lastMatch = nullptr;
        while (true) {
            if (lastMatch) {
                lastMatch = page->findText(searchID, search->cachedString, NextResult, search->cachedCaseSensitivity, lastMatch);
            } else {
                lastMatch = page->findText(searchID, search->cachedString, FromTop, search->cachedCaseSensitivity);
            }

            if (!lastMatch) {
                break;
            }

            // add highlight rect to the matches map
            (*pageMatches)[page].append(lastMatch);
        }
        delete lastMatch;

        QTimer::singleShot(0, m_parent, [this, pagesToNotify, pageMatches, currentPage, searchID] { doContinueAllDocumentSearch(pagesToNotify, pageMatches, currentPage + 1, searchID); });
    } else {
        // reset cursor to previous shape
        QApplication::restoreOverrideCursor();

        search->isCurrentlySearching = false;
        bool foundAMatch = pageMatches->count() != 0;
        for (auto [key, value] : pageMatches->asKeyValueRange()) {
            for (RegularAreaRect *&match : value) {
                key->d->setHighlight(searchID, match, search->cachedColor);
                delete match;
                match = nullptr;
            }
            search->highlightedPages.insert(key->number());
            pagesToNotify->insert(key->number());
        }

        for (DocumentObserver *observer : std::as_const(m_observers)) {
            observer->notifySetup(m_pagesVector, 0);
        }

        // notify observers about highlights changes
        for (int pageNumber : std::as_const(*pagesToNotify)) {
            for (DocumentObserver *observer : std::as_const(m_observers)) {
                observer->notifyPageChanged(pageNumber, DocumentObserver::Highlights);
            }
        }

        if (foundAMatch) {
            Q_EMIT m_parent->searchFinished(searchID, Document::MatchFound);
        } else {
            Q_EMIT m_parent->searchFinished(searchID, Document::NoMatchFound);
        }

        delete pageMatches;
        delete pagesToNotify;
    }
}

void DocumentPrivate::doContinueGooglesDocumentSearch(QSet<int> *pagesToNotify, QHash<Page *, QList<MatchColor>> *pageMatches, int currentPage, int searchID, const QStringList &words)
{
    RunningSearch *search = m_searches.value(searchID);

    if (m_searchCancelled || !search) {
        using Matches = QList<MatchColor>;

        QApplication::restoreOverrideCursor();

        if (search) {
            search->isCurrentlySearching = false;
        }

        Q_EMIT m_parent->searchFinished(searchID, Document::SearchCancelled);

        for (Matches &mv : *pageMatches) {
            for (auto &[area, color] : mv) {
                delete area;
                area = nullptr;
            }
        }
        delete pageMatches;
        delete pagesToNotify;
        return;
    }

    const int wordCount = words.count();
    const int hueStep = (wordCount > 1) ? (60 / (wordCount - 1)) : 60;
    int baseHue, baseSat, baseVal;
    search->cachedColor.getHsv(&baseHue, &baseSat, &baseVal);

    if (currentPage < m_pagesVector.count()) {
        // get page (from the first to the last)
        Page *page = m_pagesVector.at(currentPage);

        // request search page if needed
        if (!page->hasTextPage()) {
            int pageNumber = page->number(); // redundant? is it == currentPage ?
            m_parent->requestTextPage(pageNumber);
        }

        // loop on a page adding highlights for all found items
        bool allMatched = wordCount > 0, anyMatched = false;
        for (int w = 0; w < wordCount; w++) {
            const QString &word = words[w];
            int newHue = baseHue - w * hueStep;
            if (newHue < 0) {
                newHue += 360;
            }
            QColor wordColor = QColor::fromHsv(newHue, baseSat, baseVal);
            RegularAreaRect *lastMatch = nullptr;
            // add all highlights for current word
            bool wordMatched = false;
            while (true) {
                if (lastMatch) {
                    lastMatch = page->findText(searchID, word, NextResult, search->cachedCaseSensitivity, lastMatch);
                } else {
                    lastMatch = page->findText(searchID, word, FromTop, search->cachedCaseSensitivity);
                }

                if (!lastMatch) {
                    break;
                }

                // add highligh rect to the matches map
                (*pageMatches)[page].append(MatchColor(lastMatch, wordColor));
                wordMatched = true;
            }
            allMatched = allMatched && wordMatched;
            anyMatched = anyMatched || wordMatched;
        }

        // if not all words are present in page, remove partial highlights
        const bool matchAll = search->cachedType == Document::GoogleAll;
        if (!allMatched && matchAll) {
            auto &matches = (*pageMatches)[page];
            for (auto &[area, color] : matches) {
                delete area;
                area = nullptr;
            }
            pageMatches->remove(page);
        }

        QTimer::singleShot(0, m_parent, [this, pagesToNotify, pageMatches, currentPage, searchID, words] { doContinueGooglesDocumentSearch(pagesToNotify, pageMatches, currentPage + 1, searchID, words); });
    } else {
        // reset cursor to previous shape
        QApplication::restoreOverrideCursor();

        search->isCurrentlySearching = false;
        bool foundAMatch = pageMatches->count() != 0;
        for (auto [page, matches] : pageMatches->asKeyValueRange()) {
            for (auto &[area, color] : matches) {
                page->d->setHighlight(searchID, area, color);
                delete area;
            }
            search->highlightedPages.insert(page->number());
            pagesToNotify->insert(page->number());
        }

        // send page lists to update observers (since some filter on bookmarks)
        for (DocumentObserver *observer : std::as_const(m_observers)) {
            observer->notifySetup(m_pagesVector, 0);
        }

        // notify observers about highlights changes
        for (int pageNumber : std::as_const(*pagesToNotify)) {
            for (DocumentObserver *observer : std::as_const(m_observers)) {
                observer->notifyPageChanged(pageNumber, DocumentObserver::Highlights);
            }
        }

        if (foundAMatch) {
            Q_EMIT m_parent->searchFinished(searchID, Document::MatchFound);
        } else {
            Q_EMIT m_parent->searchFinished(searchID, Document::NoMatchFound);
        }

        delete pageMatches;
        delete pagesToNotify;
    }
}

QVariant DocumentPrivate::documentMetaData(const Generator::DocumentMetaDataKey key, const QVariant &option) const
{
    switch (key) {
    case Generator::PaperColorMetaData: {
        bool giveDefault = option.toBool();
        QColor color;
        if ((SettingsCore::renderMode() == SettingsCore::EnumRenderMode::Paper) && SettingsCore::changeColors()) {
            color = SettingsCore::paperColor();
        } else if (giveDefault) {
            color = Qt::white;
        }
        return color;
    }

    case Generator::TextAntialiasMetaData:
        switch (SettingsCore::textAntialias()) {
        case SettingsCore::EnumTextAntialias::Enabled:
            return true;
        case SettingsCore::EnumTextAntialias::Disabled:
            return false;
        }
        break;

    case Generator::GraphicsAntialiasMetaData:
        switch (SettingsCore::graphicsAntialias()) {
        case SettingsCore::EnumGraphicsAntialias::Enabled:
            return true;
        case SettingsCore::EnumGraphicsAntialias::Disabled:
            return false;
        }
        break;

    case Generator::TextHintingMetaData:
        switch (SettingsCore::textHinting()) {
        case SettingsCore::EnumTextHinting::Enabled:
            return true;
        case SettingsCore::EnumTextHinting::Disabled:
            return false;
        }
        break;
    }
    return QVariant();
}

bool DocumentPrivate::isNormalizedRectangleFullyVisible(const Okular::NormalizedRect &rectOfInterest, int rectPage)
{
    const auto &visibleRects = m_parent->visiblePageRects();
    return std::ranges::any_of(visibleRects, [&](VisiblePageRect *const it) {
        return it->pageNumber == rectPage                                 //
            && it->rect.contains(rectOfInterest.left, rectOfInterest.top) //
            && it->rect.contains(rectOfInterest.right, rectOfInterest.bottom);
    });
}

struct pdfsyncpoint {
    QString file;
    qlonglong x;
    qlonglong y;
    int row;
    int column;
    int page;
};

void DocumentPrivate::loadSyncFile(const QString &filePath)
{
    QFile f(filePath + QLatin1String("sync"));
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }

    QTextStream ts(&f);
    // first row: core name of the pdf output
    const QString coreName = ts.readLine();
    // second row: version string, in the form 'Version %u'
    const QString versionstr = ts.readLine();
    // anchor the pattern with \A and \z to match the entire subject string
    // TODO: with Qt 5.12 QRegularExpression::anchoredPattern() can be used instead
    static QRegularExpression versionre(QStringLiteral("\\AVersion \\d+\\z"), QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = versionre.match(versionstr);
    if (!match.hasMatch()) {
        return;
    }

    QHash<int, pdfsyncpoint> points;
    QStack<QString> fileStack;
    int currentpage = -1;
    const QLatin1String texStr(".tex");
    const QChar spaceChar = QChar::fromLatin1(' ');

    fileStack.push(coreName + texStr);

    const QSizeF dpi = m_generator->dpi();

    QString line;
    while (!ts.atEnd()) {
        line = ts.readLine();
        const QStringList tokens = line.split(spaceChar, Qt::SkipEmptyParts);
        const int tokenSize = tokens.count();
        if (tokenSize < 1) {
            continue;
        }
        if (tokens.first() == QLatin1String("l") && tokenSize >= 3) {
            int id = tokens.at(1).toInt();
            QHash<int, pdfsyncpoint>::const_iterator it = points.constFind(id);
            if (it == points.constEnd()) {
                pdfsyncpoint pt;
                pt.x = 0;
                pt.y = 0;
                pt.row = tokens.at(2).toInt();
                pt.column = 0; // TODO
                pt.page = -1;
                pt.file = fileStack.top();
                points[id] = pt;
            }
        } else if (tokens.first() == QLatin1String("s") && tokenSize >= 2) {
            currentpage = tokens.at(1).toInt() - 1;
        } else if (tokens.first() == QLatin1String("p*") && tokenSize >= 4) {
            // TODO
            qCDebug(OkularCoreDebug) << "PdfSync: 'p*' line ignored";
        } else if (tokens.first() == QLatin1String("p") && tokenSize >= 4) {
            int id = tokens.at(1).toInt();
            QHash<int, pdfsyncpoint>::iterator it = points.find(id);
            if (it != points.end()) {
                it->x = tokens.at(2).toInt();
                it->y = tokens.at(3).toInt();
                it->page = currentpage;
            }
        } else if (line.startsWith(QLatin1Char('(')) && tokenSize == 1) {
            QString newfile = line;
            // chop the leading '('
            newfile.remove(0, 1);
            if (!newfile.endsWith(texStr)) {
                newfile += texStr;
            }
            fileStack.push(newfile);
        } else if (line == QLatin1String(")")) {
            if (!fileStack.isEmpty()) {
                fileStack.pop();
            } else {
                qCDebug(OkularCoreDebug) << "PdfSync: going one level down too much";
            }
        } else {
            qCDebug(OkularCoreDebug).nospace() << "PdfSync: unknown line format: '" << line << "'";
        }
    }

    QList<QList<Okular::SourceRefObjectRect *>> refRects(m_pagesVector.size());
    for (const pdfsyncpoint &pt : std::as_const(points)) {
        // drop pdfsync points not completely valid
        if (pt.page < 0 || pt.page >= m_pagesVector.size()) {
            continue;
        }

        // magic numbers for TeX's RSU's (Ridiculously Small Units) conversion to pixels
        Okular::NormalizedPoint p((pt.x * dpi.width()) / (72.27 * 65536.0 * m_pagesVector[pt.page]->width()), (pt.y * dpi.height()) / (72.27 * 65536.0 * m_pagesVector[pt.page]->height()));
        QString file = pt.file;
        Okular::SourceReference *sourceRef = new Okular::SourceReference(file, pt.row, pt.column);
        refRects[pt.page].append(new Okular::SourceRefObjectRect(p, sourceRef));
    }
    for (int i = 0; i < refRects.size(); ++i) {
        if (!refRects.at(i).isEmpty()) {
            m_pagesVector[i]->setSourceReferences(refRects.at(i));
        }
    }
}

void DocumentPrivate::clearAndWaitForRequests()
{
    m_pixmapRequestsMutex.lock();
    qDeleteAll(m_pixmapRequestsStack);
    m_pixmapRequestsStack.clear();
    m_pixmapRequestsMutex.unlock();

    QEventLoop loop;
    bool startEventLoop = false;
    do {
        m_pixmapRequestsMutex.lock();
        startEventLoop = !m_executingPixmapRequests.empty();

        if (m_generator->hasFeature(Generator::SupportsCancelling)) {
            for (PixmapRequest *executingRequest : std::as_const(m_executingPixmapRequests)) {
                executingRequest->d->mShouldAbortRender = 1;
            }

            if (m_generator->d_ptr->mTextPageGenerationThread) {
                m_generator->d_ptr->mTextPageGenerationThread->abortExtraction();
            }
        }

        m_pixmapRequestsMutex.unlock();
        if (startEventLoop) {
            m_closingLoop = &loop;
            loop.exec();
            m_closingLoop = nullptr;
        }
    } while (startEventLoop);
}

int DocumentPrivate::findFieldPageNumber(Okular::FormField *field)
{
    // Lookup the page of the FormField
    int foundPage = -1;
    for (uint pageIdx = 0, nPages = m_parent->pages(); pageIdx < nPages; pageIdx++) {
        const Page *p = m_parent->page(pageIdx);
        if (p && p->formFields().contains(field)) {
            foundPage = static_cast<int>(pageIdx);
            break;
        }
    }
    return foundPage;
}

void DocumentPrivate::executeScriptEvent(const std::shared_ptr<Event> &event, const Okular::ScriptAction *linkscript)
{
    if (!m_scripter) {
        m_scripter = new Scripter(this);
    }
    m_scripter->setEvent(event.get());
    m_scripter->execute(linkscript->scriptType(), linkscript->script());

    // Clear out the event after execution
    m_scripter->setEvent(nullptr);
}

Document::Document(QWidget *widget)
    : QObject(nullptr)
    , d(new DocumentPrivate(this))
{
    d->m_widget = widget;
    d->m_bookmarkManager = new BookmarkManager(d);
    d->m_viewportIterator = d->m_viewportHistory.insert(d->m_viewportHistory.end(), DocumentViewport());
    d->m_undoStack = new QUndoStack(this);

    connect(SettingsCore::self(), &SettingsCore::configChanged, this, [this] { d->_o_configChanged(); });
    connect(d->m_undoStack, &QUndoStack::canUndoChanged, this, &Document::canUndoChanged);
    connect(d->m_undoStack, &QUndoStack::canRedoChanged, this, &Document::canRedoChanged);
    connect(d->m_undoStack, &QUndoStack::cleanChanged, this, &Document::undoHistoryCleanChanged);

    qRegisterMetaType<Okular::FontInfo>();
}

Document::~Document()
{
    // delete generator, pages, and related stuff
    closeDocument();

    for (View *view : std::as_const(d->m_views)) {
        view->d_func()->document = nullptr;
    }

    // delete the bookmark manager
    delete d->m_bookmarkManager;

    // delete the loaded generators
    for (auto &generator : d->m_loadedGenerators) {
        d->unloadGenerator(generator);
    }
    d->m_loadedGenerators.clear();

    // delete the private structure
    delete d;
}

QString DocumentPrivate::docDataFileName(const QUrl &url, qint64 document_size)
{
    QString fn = url.fileName();
    fn = QString::number(document_size) + QLatin1Char('.') + fn + QStringLiteral(".xml");
    QString docdataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/okular/docdata");
    // make sure that the okular/docdata/ directory exists (probably this used to be handled by KStandardDirs)
    if (!QFileInfo::exists(docdataDir)) {
        qCDebug(OkularCoreDebug) << "creating docdata folder" << docdataDir;
        QDir().mkpath(docdataDir);
    }
    QString newokularfile = docdataDir + QLatin1Char('/') + fn;

    return newokularfile;
}

QList<KPluginMetaData> DocumentPrivate::availableGenerators()
{
    static QList<KPluginMetaData> result;
    if (result.isEmpty()) {
        result = KPluginMetaData::findPlugins(QStringLiteral("okular_generators"));
    }
    return result;
}

KPluginMetaData DocumentPrivate::generatorForMimeType(const QMimeType &type, QWidget *widget, const QList<KPluginMetaData> &triedOffers)
{
    // First try to find an exact match, and then look for more general ones (e. g. the plain text one)
    // Ideally we would rank these by "closeness", but that might be overdoing it

    const QList<KPluginMetaData> available = availableGenerators();
    QList<KPluginMetaData> offers;
    QList<KPluginMetaData> exactMatches;

    QMimeDatabase mimeDatabase;

    for (const KPluginMetaData &md : available) {
        if (triedOffers.contains(md)) {
            continue;
        }

        const QStringList mimetypes = md.mimeTypes();
        for (const QString &supported : mimetypes) {
            QMimeType mimeType = mimeDatabase.mimeTypeForName(supported);
            if (mimeType == type && !exactMatches.contains(md)) {
                exactMatches << md;
            }

            if (type.inherits(supported) && !offers.contains(md)) {
                offers << md;
            }
        }
    }

    if (!exactMatches.isEmpty()) {
        offers = exactMatches;
    }

    if (offers.isEmpty()) {
        return KPluginMetaData();
    }
    int hRank = 0;
    // best ranked offer search
    if (offers.size() > 1) {
        // sort the offers: the offers with an higher priority come before
        auto cmp = [](const KPluginMetaData &s1, const KPluginMetaData &s2) {
            const QString property = QStringLiteral("X-KDE-Priority");
            return s1.rawData().value(property).toInt() > s2.rawData().value(property).toInt();
        };
        std::stable_sort(offers.begin(), offers.end(), cmp);

        if (SettingsCore::chooseGenerators()) {
            QStringList list;
            list.reserve(offers.size());
            for (const auto &offer : std::as_const(offers)) {
                list << offer.pluginId();
            }
            ChooseEngineDialog choose(list, type, widget);

            if (choose.exec() == QDialog::Rejected) {
                return KPluginMetaData();
            }

            hRank = choose.selectedGenerator();
        }
    }
    Q_ASSERT(hRank < offers.size());
    return offers.at(hRank);
}

Document::OpenResult Document::openDocument(const QString &docFile, const QUrl &url, const QMimeType &_mime, const QString &password)
{
    QMimeDatabase db;
    QMimeType mime = _mime;
    QByteArray filedata;
    int fd = -1;
    if (url.scheme() == QLatin1String("fd")) {
        bool ok;
        fd = QStringView {url.path()}.mid(1).toInt(&ok);
        if (!ok) {
            return OpenError;
        }
    } else if (url.fileName() == QLatin1String("-")) {
        fd = 0;
    }
    bool triedMimeFromFileContent = false;
    if (fd < 0) {
        if (!mime.isValid()) {
            return OpenError;
        }

        d->m_url = url;
        d->m_docFileName = docFile;

        if (!d->updateMetadataXmlNameAndDocSize()) {
            return OpenError;
        }
    } else {
        QFile qstdin;
        const bool ret = qstdin.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle);
        if (!ret) {
            qWarning() << "failed to read" << url << filedata;
            return OpenError;
        }

        filedata = qstdin.readAll();
        mime = db.mimeTypeForData(filedata);
        if (!mime.isValid() || mime.isDefault()) {
            return OpenError;
        }
        d->m_docSize = filedata.size();
        triedMimeFromFileContent = true;
    }

    const bool fromFileDescriptor = fd >= 0;

    // 0. load Generator
    // request only valid non-disabled plugins suitable for the mimetype
    KPluginMetaData offer = DocumentPrivate::generatorForMimeType(mime, d->m_widget);
    if (!offer.isValid() && !triedMimeFromFileContent) {
        QMimeType newmime = db.mimeTypeForFile(docFile, QMimeDatabase::MatchContent);
        triedMimeFromFileContent = true;
        if (newmime != mime) {
            mime = newmime;
            offer = DocumentPrivate::generatorForMimeType(mime, d->m_widget);
        }
        if (!offer.isValid()) {
            // There's still no offers, do a final mime search based on the filename
            // We need this because sometimes (e.g. when downloading from a webserver) the mimetype we
            // use is the one fed by the server, that may be wrong
            newmime = db.mimeTypeForUrl(url);

            if (!newmime.isDefault() && newmime != mime) {
                mime = newmime;
                offer = DocumentPrivate::generatorForMimeType(mime, d->m_widget);
            }
        }
    }
    if (!offer.isValid()) {
        d->m_openError = i18n("Can not find a plugin which is able to handle the document being passed.");
        Q_EMIT error(d->m_openError, -1);
        qCWarning(OkularCoreDebug).nospace() << "No plugin for mimetype '" << mime.name() << "'.";
        return OpenError;
    }

    // 1. load Document
    OpenResult openResult = d->openDocumentInternal(offer, fromFileDescriptor, docFile, filedata, password);
    if (openResult == OpenError) {
        QList<KPluginMetaData> triedOffers;
        triedOffers << offer;
        offer = DocumentPrivate::generatorForMimeType(mime, d->m_widget, triedOffers);

        while (offer.isValid()) {
            openResult = d->openDocumentInternal(offer, fromFileDescriptor, docFile, filedata, password);

            if (openResult == OpenError) {
                triedOffers << offer;
                offer = DocumentPrivate::generatorForMimeType(mime, d->m_widget, triedOffers);
            } else {
                break;
            }
        }

        if (openResult == OpenError && !triedMimeFromFileContent) {
            QMimeType newmime = db.mimeTypeForFile(docFile, QMimeDatabase::MatchContent);
            triedMimeFromFileContent = true;
            if (newmime != mime) {
                mime = newmime;
                offer = DocumentPrivate::generatorForMimeType(mime, d->m_widget, triedOffers);
                while (offer.isValid()) {
                    openResult = d->openDocumentInternal(offer, fromFileDescriptor, docFile, filedata, password);

                    if (openResult == OpenError) {
                        triedOffers << offer;
                        offer = DocumentPrivate::generatorForMimeType(mime, d->m_widget, triedOffers);
                    } else {
                        break;
                    }
                }
            }
        }

        if (openResult == OpenSuccess) {
            // Clear errors, since we're trying various generators, maybe one of them errored out
            // but we finally succeeded
            // TODO one can still see the error message animating out but since this is a very rare
            //      condition we can leave this for future work
            Q_EMIT error(QString(), -1);
        }
    }
    if (openResult != OpenSuccess) {
        return openResult;
    }

    // no need to check for the existence of a synctex file, no parser will be
    // created if none exists
    d->m_synctex_scanner = synctex_scanner_new_with_output_file(QFile::encodeName(docFile).constData(), nullptr, 1);
    if (!d->m_synctex_scanner && QFile::exists(docFile + QLatin1String("sync"))) {
        d->loadSyncFile(docFile);
    }

    d->m_generatorName = offer.pluginId();
    d->m_pageController = new PageController();
    connect(d->m_pageController, &PageController::rotationFinished, this, [this](int p, Okular::Page *op) { d->rotationFinished(p, op); });

    for (Page *p : std::as_const(d->m_pagesVector)) {
        p->d->m_doc = d;
    }

    d->m_docdataMigrationNeeded = false;

    // 2. load Additional Data (bookmarks, local annotations and metadata) about the document
    if (d->m_archiveData) {
        // QTemporaryFile is weird and will return false in exists if fileName wasn't called before
        d->m_archiveData->metadataFile.fileName();
        d->loadDocumentInfo(d->m_archiveData->metadataFile, LoadPageInfo);
        d->loadDocumentInfo(LoadGeneralInfo);
    } else {
        if (d->loadDocumentInfo(LoadPageInfo)) {
            d->m_docdataMigrationNeeded = true;
        }
        d->loadDocumentInfo(LoadGeneralInfo);
    }

    d->m_bookmarkManager->setUrl(d->m_url);

    // 3. setup observers internal lists and data
    foreachObserver(notifySetup(d->m_pagesVector, DocumentObserver::DocumentChanged | DocumentObserver::UrlChanged));

    // 4. set initial page (restoring the page saved in xml if loaded)
    DocumentViewport loadedViewport = (*d->m_viewportIterator);
    if (loadedViewport.isValid()) {
        (*d->m_viewportIterator) = DocumentViewport();
        if (loadedViewport.pageNumber >= (int)d->m_pagesVector.size()) {
            loadedViewport.pageNumber = d->m_pagesVector.size() - 1;
        }
    } else {
        loadedViewport.pageNumber = 0;
    }
    setViewport(loadedViewport);

    // start bookmark saver timer
    if (!d->m_saveBookmarksTimer) {
        d->m_saveBookmarksTimer = new QTimer(this);
        connect(d->m_saveBookmarksTimer, &QTimer::timeout, this, [this] { d->saveDocumentInfo(); });
    }
    d->m_saveBookmarksTimer->start(5 * 60 * 1000);

    // start memory check timer
    if (!d->m_memCheckTimer) {
        d->m_memCheckTimer = new QTimer(this);
        connect(d->m_memCheckTimer, &QTimer::timeout, this, [this] { d->slotTimedMemoryCheck(); });
    }
    d->m_memCheckTimer->start(kMemCheckTime);

    const DocumentViewport nextViewport = d->nextDocumentViewport();
    if (nextViewport.isValid()) {
        setViewport(nextViewport);
        d->m_nextDocumentViewport = DocumentViewport();
        d->m_nextDocumentDestination = QString();
    }

    AudioPlayer::instance()->setDocument(fromFileDescriptor ? QUrl() : d->m_url, this);

    const QStringList docScripts = d->m_generator->metaData(QStringLiteral("DocumentScripts"), QStringLiteral("JavaScript")).toStringList();
    if (!docScripts.isEmpty()) {
        d->m_scripter = new Scripter(d);
        for (const QString &docscript : docScripts) {
            const Okular::ScriptAction *linkScript = new Okular::ScriptAction(Okular::JavaScript, docscript);
            std::shared_ptr<Event> event = Event::createDocEvent(Event::DocOpen);
            d->executeScriptEvent(event, linkScript);
        }
    }

    return OpenSuccess;
}

bool DocumentPrivate::updateMetadataXmlNameAndDocSize()
{
    // m_docFileName is always local so we can use QFileInfo on it
    QFileInfo fileReadTest(m_docFileName);
    if (!fileReadTest.isFile() && !fileReadTest.isReadable()) {
        return false;
    }

    m_docSize = fileReadTest.size();

    // determine the related "xml document-info" filename
    if (m_url.isLocalFile()) {
        const QString filePath = docDataFileName(m_url, m_docSize);
        qCDebug(OkularCoreDebug) << "Metadata file is now:" << filePath;
        m_xmlFileName = filePath;
    } else {
        qCDebug(OkularCoreDebug) << "Metadata file: disabled";
        m_xmlFileName = QString();
    }

    return true;
}

KXMLGUIClient *Document::guiClient()
{
    if (d->m_generator) {
        Okular::GuiInterface *iface = qobject_cast<Okular::GuiInterface *>(d->m_generator);
        if (iface) {
            return iface->guiClient();
        }
    }
    return nullptr;
}

void Document::closeDocument()
{
    // check if there's anything to close...
    if (!d->m_generator) {
        return;
    }

    if (const Okular::Action *action = d->m_generator->additionalDocumentAction(CloseDocument)) {
        processDocumentAction(action, CloseDocument);
    }

    Q_EMIT aboutToClose();

    delete d->m_pageController;
    d->m_pageController = nullptr;

    delete d->m_scripter;
    d->m_scripter = nullptr;

    // remove requests left in queue
    d->clearAndWaitForRequests();

    if (d->m_fontThread) {
        disconnect(d->m_fontThread, nullptr, this, nullptr);
        d->m_fontThread->stopExtraction();
        d->m_fontThread->wait();
        d->m_fontThread = nullptr;
    }

    // stop any audio playback
    AudioPlayer::instance()->stopPlaybacks();

    // close the current document and save document info if a document is still opened
    if (d->m_generator && d->m_pagesVector.size() > 0) {
        d->saveDocumentInfo();

        // free the content of the opaque backend actions (if any)
        // this is a bit awkward since backends can store "random stuff" in the
        // BackendOpaqueAction nativeId qvariant so we need to tell them to free it
        // ideally we would just do that in the BackendOpaqueAction destructor
        // but that's too late in the cleanup process, i.e. the generator has already closed its document
        // and the document generator is nullptr
        for (const Page *p : std::as_const(d->m_pagesVector)) {
            const QList<ObjectRect *> &oRects = p->objectRects();
            for (const ObjectRect *oRect : oRects) {
                if (oRect->objectType() == ObjectRect::Action) {
                    const Action *a = static_cast<const Action *>(oRect->object());
                    const BackendOpaqueAction *backendAction = dynamic_cast<const BackendOpaqueAction *>(a);
                    if (backendAction) {
                        d->m_generator->freeOpaqueActionContents(*backendAction);
                    }
                }
            }

            const QList<FormField *> forms = p->formFields();
            for (const FormField *form : forms) {
                const QList<Action *> additionalActions = form->additionalActions();
                for (const Action *a : additionalActions) {
                    const BackendOpaqueAction *backendAction = dynamic_cast<const BackendOpaqueAction *>(a);
                    if (backendAction) {
                        d->m_generator->freeOpaqueActionContents(*backendAction);
                    }
                }
            }
        }

        d->m_generator->closeDocument();
    }

    if (d->m_synctex_scanner) {
        synctex_scanner_free(d->m_synctex_scanner);
        d->m_synctex_scanner = nullptr;
    }

    // stop timers
    if (d->m_memCheckTimer) {
        d->m_memCheckTimer->stop();
    }
    if (d->m_saveBookmarksTimer) {
        d->m_saveBookmarksTimer->stop();
    }

    if (d->m_generator) {
        // disconnect the generator from this document ...
        d->m_generator->d_func()->m_document = nullptr;
        // .. and this document from the generator signals
        disconnect(d->m_generator, nullptr, this, nullptr);

        Q_ASSERT(d->m_loadedGenerators.contains(d->m_generatorName));
    }
    d->m_generator = nullptr;
    d->m_generatorName = QString();
    d->m_url = QUrl();
    d->m_walletGenerator = nullptr;
    d->m_docFileName = QString();
    d->m_xmlFileName = QString();
    delete d->m_tempFile;
    d->m_tempFile = nullptr;
    delete d->m_archiveData;
    d->m_archiveData = nullptr;
    d->m_docSize = -1;
    d->m_exportCached = false;
    d->m_exportFormats.clear();
    d->m_exportToText = ExportFormat();
    d->m_fontsCached = false;
    d->m_fontsCache.clear();
    d->m_rotation = Rotation0;

    // send an empty list to observers (to free their data)
    foreachObserver(notifySetup({}, DocumentObserver::DocumentChanged | DocumentObserver::UrlChanged));

    // delete pages and clear 'd->m_pagesVector' container
    qDeleteAll(d->m_pagesVector);
    d->m_pagesVector.clear();

    // clear 'memory allocation' descriptors
    qDeleteAll(d->m_allocatedPixmaps);
    d->m_allocatedPixmaps.clear();

    // clear 'running searches' descriptors
    qDeleteAll(d->m_searches);
    d->m_searches.clear();

    // clear the visible areas and notify the observers
    qDeleteAll(d->m_pageRects);
    d->m_pageRects.clear();
    foreachObserver(notifyVisibleRectsChanged());

    // reset internal variables

    d->m_viewportHistory.clear();
    d->m_viewportHistory.emplace_back();
    d->m_viewportIterator = d->m_viewportHistory.begin();
    d->m_allocatedPixmapsTotalMemory = 0;
    d->m_allocatedTextPagesFifo.clear();
    d->m_pageSize = PageSize();
    d->m_pageSizes.clear();

    d->m_documentInfo = DocumentInfo();
    d->m_documentInfoAskedKeys.clear();

    AudioPlayer::instance()->resetDocument();

    d->m_undoStack->clear();
    d->m_docdataMigrationNeeded = false;

#if HAVE_MALLOC_TRIM
    // trim unused memory, glibc should do this but it seems it does not
    // this can greatly decrease the [perceived] memory consumption of okular
    // see: https://sourceware.org/bugzilla/show_bug.cgi?id=14827
    malloc_trim(0);
#endif
}

void Document::addObserver(DocumentObserver *pObserver)
{
    Q_ASSERT(!d->m_observers.contains(pObserver));
    d->m_observers << pObserver;

    // if the observer is added while a document is already opened, tell it
    if (!d->m_pagesVector.isEmpty()) {
        pObserver->notifySetup(d->m_pagesVector, DocumentObserver::DocumentChanged | DocumentObserver::UrlChanged);
        pObserver->notifyViewportChanged(false /*disables smoothMove*/);
    }
}

void Document::removeObserver(DocumentObserver *pObserver)
{
    // remove observer from the set. it won't receive notifications anymore
    if (d->m_observers.contains(pObserver)) {
        // free observer's pixmap data
        for (Page *const page : std::as_const(d->m_pagesVector)) {
            page->deletePixmap(pObserver);
        }

        // [MEM] free observer's allocation descriptors
        auto aIt = d->m_allocatedPixmaps.begin();
        auto aEnd = d->m_allocatedPixmaps.end();
        while (aIt != aEnd) {
            AllocatedPixmap *p = *aIt;
            if (p->observer == pObserver) {
                aIt = d->m_allocatedPixmaps.erase(aIt);
                delete p;
            } else {
                ++aIt;
            }
        }

        for (PixmapRequest *executingRequest : std::as_const(d->m_executingPixmapRequests)) {
            if (executingRequest->observer() == pObserver) {
                d->cancelRenderingBecauseOf(executingRequest, nullptr);
            }
        }

        // remove observer entry from the set
        d->m_observers.remove(pObserver);
    }
}

void Document::reparseConfig()
{
    // reparse generator config and if something changed clear Pages
    bool configchanged = false;
    if (d->m_generator) {
        Okular::ConfigInterface *iface = qobject_cast<Okular::ConfigInterface *>(d->m_generator);
        if (iface) {
            configchanged = iface->reparseConfig();
        }
    }
    if (configchanged) {
        // invalidate pixmaps
        for (Page *const page : std::as_const(d->m_pagesVector)) {
            page->deletePixmaps();
        }

        // [MEM] remove allocation descriptors
        qDeleteAll(d->m_allocatedPixmaps);
        d->m_allocatedPixmaps.clear();
        d->m_allocatedPixmapsTotalMemory = 0;

        // send reload signals to observers
        foreachObserver(notifyContentsCleared(DocumentObserver::Pixmap));
    }

    // free memory if in 'low' profile
    if (SettingsCore::memoryLevel() == SettingsCore::EnumMemoryLevel::Low && !d->m_allocatedPixmaps.empty() && !d->m_pagesVector.isEmpty()) {
        d->cleanupPixmapMemory();
    }
}

bool Document::isOpened() const
{
    return d->m_generator;
}

bool Document::canConfigurePrinter() const
{
    if (d->m_generator) {
        const Okular::PrintInterface *iface = qobject_cast<Okular::PrintInterface *>(d->m_generator);
        return iface ? true : false;
    } else {
        return false;
    }
}

std::pair<SigningResult, QString> Document::sign(const NewSignatureData &data, const QString &newPath)
{
    if (d->m_generator->canSign()) {
        return d->m_generator->sign(data, newPath);
    } else {
        return {GenericSigningError, i18nc("Unsupported action", "Signing not implemented for this document type")};
    }
}

Okular::CertificateStore *Document::certificateStore() const
{
    return d->m_generator ? d->m_generator->certificateStore() : nullptr;
}

void Document::setEditorCommandOverride(const QString &editCmd)
{
    d->editorCommandOverride = editCmd;
}

QString Document::editorCommandOverride() const
{
    return d->editorCommandOverride;
}

DocumentInfo Document::documentInfo() const
{
    QSet<DocumentInfo::Key> keys;
    for (Okular::DocumentInfo::Key ks = Okular::DocumentInfo::Title; ks < Okular::DocumentInfo::Invalid; ks = Okular::DocumentInfo::Key(ks + 1)) {
        keys << ks;
    }

    return documentInfo(keys);
}

DocumentInfo Document::documentInfo(const QSet<DocumentInfo::Key> &keys) const
{
    DocumentInfo result = d->m_documentInfo;
    const QSet<DocumentInfo::Key> missingKeys = keys - d->m_documentInfoAskedKeys;

    if (d->m_generator && !missingKeys.isEmpty()) {
        DocumentInfo info = d->m_generator->generateDocumentInfo(missingKeys);

        if (missingKeys.contains(DocumentInfo::FilePath)) {
            info.set(DocumentInfo::FilePath, currentDocument().toDisplayString());
        }

        if (d->m_docSize != -1 && missingKeys.contains(DocumentInfo::DocumentSize)) {
            const QString sizeString = KFormat().formatByteSize(d->m_docSize);
            info.set(DocumentInfo::DocumentSize, sizeString);
        }
        if (missingKeys.contains(DocumentInfo::PagesSize)) {
            const QString pagesSize = d->pagesSizeString();
            if (!pagesSize.isEmpty()) {
                info.set(DocumentInfo::PagesSize, pagesSize);
            }
        }

        if (missingKeys.contains(DocumentInfo::Pages) && info.get(DocumentInfo::Pages).isEmpty()) {
            info.set(DocumentInfo::Pages, QString::number(this->pages()));
        }

        d->m_documentInfo.d->values.insert(info.d->values);
        d->m_documentInfo.d->titles.insert(info.d->titles);
        result.d->values.insert(info.d->values);
        result.d->titles.insert(info.d->titles);
    }
    d->m_documentInfoAskedKeys += keys;

    return result;
}

const DocumentSynopsis *Document::documentSynopsis() const
{
    return d->m_generator ? d->m_generator->generateDocumentSynopsis() : nullptr;
}

void Document::startFontReading()
{
    if (!d->m_generator || !d->m_generator->hasFeature(Generator::FontInfo) || d->m_fontThread) {
        return;
    }

    if (d->m_fontsCached) {
        // in case we have cached fonts, simulate a reading
        // this way the API is the same, and users no need to care about the
        // internal caching
        for (int i = 0; i < d->m_fontsCache.count(); ++i) {
            Q_EMIT gotFont(d->m_fontsCache.at(i));
            Q_EMIT fontReadingProgress(i / pages());
        }
        Q_EMIT fontReadingEnded();
        return;
    }

    d->m_fontThread = new FontExtractionThread(d->m_generator, pages());
    connect(d->m_fontThread, &FontExtractionThread::gotFont, this, [this](const Okular::FontInfo &f) { d->fontReadingGotFont(f); });
    connect(d->m_fontThread.data(), &FontExtractionThread::progress, this, [this](int p) { d->slotFontReadingProgress(p); });

    d->m_fontThread->startExtraction(/*d->m_generator->hasFeature( Generator::Threaded )*/ true);
}

void Document::stopFontReading()
{
    if (!d->m_fontThread) {
        return;
    }

    disconnect(d->m_fontThread, nullptr, this, nullptr);
    d->m_fontThread->stopExtraction();
    d->m_fontThread = nullptr;
    d->m_fontsCache.clear();
}

bool Document::canProvideFontInformation() const
{
    return d->m_generator ? d->m_generator->hasFeature(Generator::FontInfo) : false;
}

bool Document::canSign() const
{
    return d->m_generator ? d->m_generator->canSign() : false;
}

const QList<EmbeddedFile *> *Document::embeddedFiles() const
{
    return d->m_generator ? d->m_generator->embeddedFiles() : nullptr;
}

const Page *Document::page(int n) const
{
    return (n >= 0 && n < d->m_pagesVector.count()) ? d->m_pagesVector.at(n) : nullptr;
}

const DocumentViewport &Document::viewport() const
{
    return (*d->m_viewportIterator);
}

const QList<VisiblePageRect *> &Document::visiblePageRects() const
{
    return d->m_pageRects;
}

void Document::setVisiblePageRects(const QList<VisiblePageRect *> &visiblePageRects, DocumentObserver *excludeObserver)
{
    qDeleteAll(d->m_pageRects);
    d->m_pageRects = visiblePageRects;
    // notify change to all other (different from id) observers
    for (DocumentObserver *o : std::as_const(d->m_observers)) {
        if (o != excludeObserver) {
            o->notifyVisibleRectsChanged();
        }
    }
}

uint Document::currentPage() const
{
    return (*d->m_viewportIterator).pageNumber;
}

uint Document::pages() const
{
    return d->m_pagesVector.size();
}

QUrl Document::currentDocument() const
{
    return d->m_url;
}

bool Document::isAllowed(Permission action) const
{
    if (action == Okular::AllowNotes && (d->m_docdataMigrationNeeded || !d->m_annotationEditingEnabled)) {
        return false;
    }
    if (action == Okular::AllowFillForms && d->m_docdataMigrationNeeded) {
        return false;
    }

#if !OKULAR_FORCE_DRM
    if (KAuthorized::authorize(QStringLiteral("skip_drm")) && !SettingsCore::obeyDRM()) {
        return true;
    }
#endif

    return d->m_generator ? d->m_generator->isAllowed(action) : false;
}

bool Document::supportsSearching() const
{
    return d->m_generator ? d->m_generator->hasFeature(Generator::TextExtraction) : false;
}

bool Document::supportsPageSizes() const
{
    return d->m_generator ? d->m_generator->hasFeature(Generator::PageSizes) : false;
}

bool Document::supportsTiles() const
{
    return d->m_generator ? d->m_generator->hasFeature(Generator::TiledRendering) : false;
}

PageSize::List Document::pageSizes() const
{
    if (d->m_generator) {
        if (d->m_pageSizes.isEmpty()) {
            d->m_pageSizes = d->m_generator->pageSizes();
        }
        return d->m_pageSizes;
    }
    return PageSize::List();
}

bool Document::canExportToText() const
{
    if (!d->m_generator) {
        return false;
    }

    d->cacheExportFormats();
    return !d->m_exportToText.isNull();
}

bool Document::exportToText(const QString &fileName) const
{
    if (!d->m_generator) {
        return false;
    }

    d->cacheExportFormats();
    if (d->m_exportToText.isNull()) {
        return false;
    }

    return d->m_generator->exportTo(fileName, d->m_exportToText);
}

ExportFormat::List Document::exportFormats() const
{
    if (!d->m_generator) {
        return ExportFormat::List();
    }

    d->cacheExportFormats();
    return d->m_exportFormats;
}

bool Document::exportTo(const QString &fileName, const ExportFormat &format) const
{
    return d->m_generator ? d->m_generator->exportTo(fileName, format) : false;
}

bool Document::historyAtBegin() const
{
    return d->m_viewportIterator == d->m_viewportHistory.begin();
}

bool Document::historyAtEnd() const
{
    return d->m_viewportIterator == --(d->m_viewportHistory.end());
}

QVariant Document::metaData(const QString &key, const QVariant &option) const
{
    // if option starts with "src:" assume that we are handling a
    // source reference
    if (key == QLatin1String("NamedViewport") && option.toString().startsWith(QLatin1String("src:"), Qt::CaseInsensitive) && d->m_synctex_scanner) {
        const QString reference = option.toString();

        // The reference is of form "src:1111Filename", where "1111"
        // points to line number 1111 in the file "Filename".
        // Extract the file name and the numeral part from the reference string.
        // This will fail if Filename starts with a digit.
        QString name, lineString;
        // Remove "src:". Presence of substring has been checked before this
        // function is called.
        name = reference.mid(4);
        // split
        int nameLength = name.length();
        int i = 0;
        for (i = 0; i < nameLength; ++i) {
            if (!name[i].isDigit()) {
                break;
            }
        }
        lineString = name.left(i);
        name = name.mid(i);
        // Remove spaces.
        name = name.trimmed();
        lineString = lineString.trimmed();
        // Convert line to integer.
        bool ok;
        int line = lineString.toInt(&ok);
        if (!ok) {
            line = -1;
        }

        // Use column == -1 for now.
        if (synctex_display_query(d->m_synctex_scanner, QFile::encodeName(name).constData(), line, -1, 0) > 0) {
            synctex_node_p node;
            // For now use the first hit. Could possibly be made smarter
            // in case there are multiple hits.
            while ((node = synctex_scanner_next_result(d->m_synctex_scanner))) {
                Okular::DocumentViewport view;

                // TeX pages start at 1.
                view.pageNumber = synctex_node_page(node) - 1;

                if (view.pageNumber >= 0) {
                    const QSizeF dpi = d->m_generator->dpi();

                    // TeX small points ...
                    double px = (synctex_node_visible_h(node) * dpi.width()) / 72.27;
                    double py = (synctex_node_visible_v(node) * dpi.height()) / 72.27;
                    view.rePos.normalizedX = px / page(view.pageNumber)->width();
                    view.rePos.normalizedY = (py + 0.5) / page(view.pageNumber)->height();
                    view.rePos.enabled = true;
                    view.rePos.pos = Okular::DocumentViewport::Center;

                    return view.toString();
                }
            }
        }
    }
    return d->m_generator ? d->m_generator->metaData(key, option) : QVariant();
}

Rotation Document::rotation() const
{
    return d->m_rotation;
}

QSizeF Document::allPagesSize() const
{
    const auto getSize = [](const Page *page) { return QSizeF(page->width(), page->height()); };
    if (d->m_pagesVector.count() != 0) {
        const auto sample = getSize(d->m_pagesVector[0]);
        const auto isSameSize = [&](const Page *page) { return getSize(page) == sample; };
        if (std::ranges::all_of(d->m_pagesVector, isSameSize)) {
            return sample;
        }
    }
    return QSizeF();
}

QString Document::pageSizeString(int page) const
{
    if (d->m_generator) {
        if (d->m_generator->pagesSizeMetric() != Generator::None) {
            const Page *p = d->m_pagesVector.at(page);
            return d->localizedSize(QSizeF(p->width(), p->height()));
        }
    }
    return QString();
}

static bool shouldCancelRenderingBecauseOf(const PixmapRequest &executingRequest, const PixmapRequest &otherRequest)
{
    // New request has higher priority -> cancel
    if (executingRequest.priority() > otherRequest.priority()) {
        return true;
    }

    // New request has lower priority -> don't cancel
    if (executingRequest.priority() < otherRequest.priority()) {
        return false;
    }

    // New request has same priority and is from a different observer -> don't cancel
    // AFAIK this never happens since all observers have different priorities
    if (executingRequest.observer() != otherRequest.observer()) {
        return false;
    }

    // Same priority and observer, different page number -> don't cancel
    // may still end up cancelled later in the parent caller if none of the requests
    // is of the executingRequest page and RemoveAllPrevious is specified
    if (executingRequest.pageNumber() != otherRequest.pageNumber()) {
        return false;
    }

    // Same priority, observer, page, different size -> cancel
    if (executingRequest.width() != otherRequest.width()) {
        return true;
    }

    // Same priority, observer, page, different size -> cancel
    if (executingRequest.height() != otherRequest.height()) {
        return true;
    }

    // Same priority, observer, page, different tiling -> cancel
    if (executingRequest.isTile() != otherRequest.isTile()) {
        return true;
    }

    // Same priority, observer, page, different tiling -> cancel
    if (executingRequest.isTile()) {
        const NormalizedRect bothRequestsRect = executingRequest.normalizedRect() | otherRequest.normalizedRect();
        if (!(bothRequestsRect == executingRequest.normalizedRect())) {
            return true;
        }
    }

    return false;
}

bool DocumentPrivate::cancelRenderingBecauseOf(PixmapRequest *executingRequest, PixmapRequest *newRequest)
{
    // No point in aborting the rendering already finished, let it go through
    if (!executingRequest->d->mResultImage.isNull()) {
        return false;
    }

    if (newRequest && newRequest->asynchronous() && executingRequest->partialUpdatesWanted()) {
        newRequest->setPartialUpdatesWanted(true);
    }

    TilesManager *tm = executingRequest->d->tilesManager();
    if (tm) {
        tm->setPixmap(nullptr, executingRequest->normalizedRect(), true /*isPartialPixmap*/);
        tm->setRequest(NormalizedRect(), 0, 0);
    }
    PagePrivate::PixmapObject object = executingRequest->page()->d->m_pixmaps.take(executingRequest->observer());
    delete object.m_pixmap;
    object.m_pixmap = nullptr;

    if (executingRequest->d->mShouldAbortRender != 0) {
        return false;
    }

    executingRequest->d->mShouldAbortRender = 1;

    if (m_generator->d_ptr->mTextPageGenerationThread && m_generator->d_ptr->mTextPageGenerationThread->page() == executingRequest->page()) {
        m_generator->d_ptr->mTextPageGenerationThread->abortExtraction();
    }

    return true;
}

void Document::requestPixmaps(const QList<PixmapRequest *> &requests)
{
    requestPixmaps(requests, RemoveAllPrevious);
}

void Document::requestPixmaps(const QList<PixmapRequest *> &requests, PixmapRequestFlags reqOptions)
{
    if (requests.isEmpty()) {
        return;
    }

    if (!d->m_pageController) {
        // delete requests..
        qDeleteAll(requests);
        // ..and return
        return;
    }

    QSet<DocumentObserver *> observersPixmapCleared;

    // 1. [CLEAN STACK] remove previous requests of requesterID
    const DocumentObserver *requesterObserver = requests.first()->observer();
    QSet<int> requestedPages;
    {
        for (const PixmapRequest *request : requests) {
            Q_ASSERT(request->observer() == requesterObserver);
            requestedPages.insert(request->pageNumber());
        }
    }
    const bool removeAllPrevious = reqOptions & RemoveAllPrevious;
    d->m_pixmapRequestsMutex.lock();
    auto sIt = d->m_pixmapRequestsStack.begin();
    auto sEnd = d->m_pixmapRequestsStack.end();
    while (sIt != sEnd) {
        if ((*sIt)->observer() == requesterObserver && (removeAllPrevious || requestedPages.contains((*sIt)->pageNumber()))) {
            // delete request and remove it from stack
            delete *sIt;
            sIt = d->m_pixmapRequestsStack.erase(sIt);
        } else {
            ++sIt;
        }
    }

    // 1.B [PREPROCESS REQUESTS] tweak some values of the requests
    for (PixmapRequest *request : requests) {
        // set the 'page field' (see PixmapRequest) and check if it is valid
        qCDebug(OkularCoreDebug).nospace() << "request observer=" << request->observer() << " " << request->width() << "x" << request->height() << "@" << request->pageNumber();
        if (d->m_pagesVector.value(request->pageNumber()) == nullptr) {
            // skip requests referencing an invalid page (must not happen)
            delete request;
            request = nullptr;
            continue;
        }

        request->d->mPage = d->m_pagesVector.value(request->pageNumber());

        if (request->isTile()) {
            // Change the current request rect so that only invalid tiles are
            // requested. Also make sure the rect is tile-aligned.
            NormalizedRect tilesRect;
            const QList<Tile> tiles = request->d->tilesManager()->tilesAt(request->normalizedRect(), TilesManager::TerminalTile);
            for (const Tile &tile : tiles) {
                if (!tile.isValid()) {
                    if (tilesRect.isNull()) {
                        tilesRect = tile.rect();
                    } else {
                        tilesRect |= tile.rect();
                    }
                }
            }

            request->setNormalizedRect(tilesRect);
        }

        if (!request->asynchronous()) {
            request->d->mPriority = 0;
        }
    }

    // 1.C [CANCEL REQUESTS] cancel those requests that are running and should be cancelled because of the new requests coming in
    if (d->m_generator->hasFeature(Generator::SupportsCancelling)) {
        for (PixmapRequest *executingRequest : std::as_const(d->m_executingPixmapRequests)) {
            bool newRequestsContainExecutingRequestPage = false;
            bool requestCancelled = false;
            for (PixmapRequest *newRequest : requests) {
                if (newRequest->pageNumber() == executingRequest->pageNumber() && requesterObserver == executingRequest->observer()) {
                    newRequestsContainExecutingRequestPage = true;
                }

                if (shouldCancelRenderingBecauseOf(*executingRequest, *newRequest)) {
                    requestCancelled = d->cancelRenderingBecauseOf(executingRequest, newRequest);
                }
            }

            // If we were told to remove all the previous requests and the executing request page is not part of the new requests, cancel it
            if (!requestCancelled && removeAllPrevious && requesterObserver == executingRequest->observer() && !newRequestsContainExecutingRequestPage) {
                requestCancelled = d->cancelRenderingBecauseOf(executingRequest, nullptr);
            }

            if (requestCancelled) {
                observersPixmapCleared << executingRequest->observer();
            }
        }
    }

    // 2. [ADD TO STACK] add requests to stack
    for (PixmapRequest *request : requests) {
        // add request to the 'stack' at the right place
        if (request->priority() == 0) {
            // add priority zero requests to the top of the stack
            d->m_pixmapRequestsStack.push_back(request);
        } else {
            // insert in stack sorted by priority
            auto it = std::ranges::find_if(d->m_pixmapRequestsStack, [&](const auto &it) { //
                return it->priority() <= request->priority();
            });
            d->m_pixmapRequestsStack.insert(it, request);
        }
    }
    d->m_pixmapRequestsMutex.unlock();

    // 3. [START FIRST GENERATION] if <NO>generator is ready, start a new generation,
    // or else (if gen is running) it will be started when the new contents will
    // come from generator (in requestDone())</NO>
    // all handling of requests put into sendGeneratorPixmapRequest
    //    if ( generator->canRequestPixmap() )
    d->sendGeneratorPixmapRequest();

    for (DocumentObserver *o : std::as_const(observersPixmapCleared)) {
        o->notifyContentsCleared(Okular::DocumentObserver::Pixmap);
    }
}

void Document::requestTextPage(uint pageNumber)
{
    Page *kp = d->m_pagesVector[pageNumber];
    if (!d->m_generator || !kp) {
        return;
    }

    // Memory management for TextPages

    d->m_generator->generateTextPage(kp);
}

void DocumentPrivate::notifyAnnotationChanges(int page)
{
    foreachObserverD(notifyPageChanged(page, DocumentObserver::Annotations));
}

void DocumentPrivate::notifyFormChanges(int /*page*/)
{
    recalculateForms();
}

void Document::recalculateForms()
{
    d->recalculateForms();
}

void Document::addPageAnnotation(int page, Annotation *annotation)
{
    // Transform annotation's base boundary rectangle into unrotated coordinates
    Page *p = d->m_pagesVector[page];
    QTransform t = p->d->rotationMatrix();
    annotation->d_ptr->baseTransform(t.inverted());
    QUndoCommand *uc = new AddAnnotationCommand(this->d, annotation, page);
    d->m_undoStack->push(uc);
}

bool Document::canModifyPageAnnotation(const Annotation *annotation) const
{
    if (!annotation || (annotation->flags() & Annotation::DenyWrite)) {
        return false;
    }

    if (!isAllowed(Okular::AllowNotes)) {
        return false;
    }

    if ((annotation->flags() & Annotation::External) && !d->canModifyExternalAnnotations()) {
        return false;
    }

    switch (annotation->subType()) {
    case Annotation::AText:
    case Annotation::ALine:
    case Annotation::AGeom:
    case Annotation::AHighlight:
    case Annotation::AStamp:
    case Annotation::AInk:
        return true;
    case Annotation::AWidget:
#if HAVE_NEW_SIGNATURE_API
        return dynamic_cast<const SignatureAnnotation *>(annotation);
#endif
        return false;
    default:
        return false;
    }
}

void Document::prepareToModifyAnnotationProperties(Annotation *annotation)
{
    Q_ASSERT(d->m_prevPropsOfAnnotBeingModified.isNull());
    if (!d->m_prevPropsOfAnnotBeingModified.isNull()) {
        qCCritical(OkularCoreDebug) << "Error: Document::prepareToModifyAnnotationProperties has already been called since last call to Document::modifyPageAnnotationProperties";
        return;
    }
    d->m_prevPropsOfAnnotBeingModified = annotation->getAnnotationPropertiesDomNode();
}

void Document::modifyPageAnnotationProperties(int page, Annotation *annotation)
{
    Q_ASSERT(!d->m_prevPropsOfAnnotBeingModified.isNull());
    if (d->m_prevPropsOfAnnotBeingModified.isNull()) {
        qCCritical(OkularCoreDebug) << "Error: Document::prepareToModifyAnnotationProperties must be called before Annotation is modified";
        return;
    }
    QDomNode prevProps = d->m_prevPropsOfAnnotBeingModified;
    QUndoCommand *uc = new Okular::ModifyAnnotationPropertiesCommand(d, annotation, page, prevProps, annotation->getAnnotationPropertiesDomNode());
    d->m_undoStack->push(uc);
    d->m_prevPropsOfAnnotBeingModified.clear();
}

void Document::translatePageAnnotation(int page, Annotation *annotation, const NormalizedPoint &delta)
{
    int complete = (annotation->flags() & Okular::Annotation::BeingMoved) == 0;
    QUndoCommand *uc = new Okular::TranslateAnnotationCommand(d, annotation, page, delta, complete);
    d->m_undoStack->push(uc);
}

void Document::adjustPageAnnotation(int page, Annotation *annotation, const Okular::NormalizedPoint &delta1, const Okular::NormalizedPoint &delta2)
{
    const bool complete = (annotation->flags() & Okular::Annotation::BeingResized) == 0;
    QUndoCommand *uc = new Okular::AdjustAnnotationCommand(d, annotation, page, delta1, delta2, complete);
    d->m_undoStack->push(uc);
}

void Document::editPageAnnotationContents(int page, Annotation *annotation, const QString &newContents, int newCursorPos, int prevCursorPos, int prevAnchorPos)
{
    QString prevContents = annotation->contents();
    QUndoCommand *uc = new EditAnnotationContentsCommand(d, annotation, page, newContents, newCursorPos, prevContents, prevCursorPos, prevAnchorPos);
    d->m_undoStack->push(uc);
}

bool Document::canRemovePageAnnotation(const Annotation *annotation) const
{
    if (!annotation || (annotation->flags() & Annotation::DenyDelete)) {
        return false;
    }

    if ((annotation->flags() & Annotation::External) && !d->canRemoveExternalAnnotations()) {
        return false;
    }

    switch (annotation->subType()) {
    case Annotation::AText:
    case Annotation::ALine:
    case Annotation::AGeom:
    case Annotation::AHighlight:
    case Annotation::AStamp:
    case Annotation::AInk:
    case Annotation::ACaret:
        return true;
    default:
        return false;
    }
}

void Document::removePageAnnotation(int page, Annotation *annotation)
{
    QUndoCommand *uc = new RemoveAnnotationCommand(this->d, annotation, page);
    d->m_undoStack->push(uc);
}

void Document::removePageAnnotations(int page, const QList<Annotation *> &annotations)
{
    d->m_undoStack->beginMacro(i18nc("remove a collection of annotations from the page", "remove annotations"));
    for (Annotation *annotation : annotations) {
        QUndoCommand *uc = new RemoveAnnotationCommand(this->d, annotation, page);
        d->m_undoStack->push(uc);
    }
    d->m_undoStack->endMacro();
}

bool DocumentPrivate::canAddAnnotationsNatively() const
{
    Okular::SaveInterface *iface = qobject_cast<Okular::SaveInterface *>(m_generator);

    if (iface && iface->supportsOption(Okular::SaveInterface::SaveChanges) && iface->annotationProxy() && iface->annotationProxy()->supports(AnnotationProxy::Addition)) {
        return true;
    }

    return false;
}

bool DocumentPrivate::canModifyExternalAnnotations() const
{
    Okular::SaveInterface *iface = qobject_cast<Okular::SaveInterface *>(m_generator);

    if (iface && iface->supportsOption(Okular::SaveInterface::SaveChanges) && iface->annotationProxy() && iface->annotationProxy()->supports(AnnotationProxy::Modification)) {
        return true;
    }

    return false;
}

bool DocumentPrivate::canRemoveExternalAnnotations() const
{
    Okular::SaveInterface *iface = qobject_cast<Okular::SaveInterface *>(m_generator);

    if (iface && iface->supportsOption(Okular::SaveInterface::SaveChanges) && iface->annotationProxy() && iface->annotationProxy()->supports(AnnotationProxy::Removal)) {
        return true;
    }

    return false;
}

void Document::setPageTextSelection(int page, std::unique_ptr<RegularAreaRect> &&rect, const QColor &color)
{
    Page *kp = d->m_pagesVector[page];
    if (!d->m_generator || !kp) {
        return;
    }

    // add or remove the selection basing whether rect is null or not
    if (rect) {
        kp->d->setTextSelections(*rect, color);
    } else {
        kp->d->deleteTextSelections();
    }

    // notify observers about the change
    foreachObserver(notifyPageChanged(page, DocumentObserver::TextSelection));
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

void Document::setViewport(const DocumentViewport &viewport, DocumentObserver *excludeObserver, bool smoothMove, bool updateHistory)
{
    if (!viewport.isValid()) {
        qCDebug(OkularCoreDebug) << "invalid viewport:" << viewport.toString();
        return;
    }
    if (viewport.pageNumber >= int(d->m_pagesVector.count())) {
        // qCDebug(OkularCoreDebug) << "viewport out of document:" << viewport.toString();
        return;
    }

    // if already broadcasted, don't redo it
    DocumentViewport &oldViewport = *d->m_viewportIterator;
    // disabled by enrico on 2005-03-18 (less debug output)
    // if ( viewport == oldViewport )
    //    qCDebug(OkularCoreDebug) << "setViewport with the same viewport.";

    const int oldPageNumber = oldViewport.pageNumber;

    // set internal viewport taking care of history
    if (oldViewport.pageNumber == viewport.pageNumber || !oldViewport.isValid() || !updateHistory) {
        // if page is unchanged save the viewport at current position in queue
        oldViewport = viewport;
    } else {
        // remove elements after viewportIterator in queue
        d->m_viewportHistory.erase(++d->m_viewportIterator, d->m_viewportHistory.end());

        // keep the list to a reasonable size by removing head when needed
        if (d->m_viewportHistory.size() >= OKULAR_HISTORY_MAXSTEPS) {
            d->m_viewportHistory.pop_front();
        }

        // add the item at the end of the queue
        d->m_viewportIterator = d->m_viewportHistory.insert(d->m_viewportHistory.end(), viewport);
    }

    const int currentViewportPage = (*d->m_viewportIterator).pageNumber;

    const bool currentPageChanged = (oldPageNumber != currentViewportPage);

    // notify change to all other (different from id) observers
    for (DocumentObserver *o : std::as_const(d->m_observers)) {
        if (o != excludeObserver) {
            o->notifyViewportChanged(smoothMove);
        }

        if (currentPageChanged) {
            o->notifyCurrentPageChanged(oldPageNumber, currentViewportPage);
        }
    }
}

void Document::setViewportPage(int page, DocumentObserver *excludeObserver, bool smoothMove)
{
    // clamp page in range [0 ... numPages-1]
    if (page < 0) {
        page = 0;
    } else if (page > (int)d->m_pagesVector.count()) {
        page = d->m_pagesVector.count() - 1;
    }

    // make a viewport from the page and broadcast it
    setViewport(DocumentViewport(page), excludeObserver, smoothMove);
}

void Document::setZoom(int factor, DocumentObserver *excludeObserver)
{
    // notify change to all other (different from id) observers
    for (DocumentObserver *o : std::as_const(d->m_observers)) {
        if (o != excludeObserver) {
            o->notifyZoom(factor);
        }
    }
}

void Document::setPrevViewport()
// restore viewport from the history
{
    if (d->m_viewportIterator != d->m_viewportHistory.begin()) {
        const int oldViewportPage = d->m_viewportIterator->pageNumber;

        // restore previous viewport and notify it to observers
        --d->m_viewportIterator;
        foreachObserver(notifyViewportChanged(true));

        const int currentViewportPage = d->m_viewportIterator->pageNumber;
        if (oldViewportPage != currentViewportPage) {
            foreachObserver(notifyCurrentPageChanged(oldViewportPage, currentViewportPage));
        }
    }
}

void Document::setNextViewport()
// restore next viewport from the history
{
    std::list<DocumentViewport>::const_iterator nextIterator = d->m_viewportIterator;
    ++nextIterator;
    if (nextIterator != d->m_viewportHistory.end()) {
        const int oldViewportPage = d->m_viewportIterator->pageNumber;

        // restore next viewport and notify it to observers
        ++d->m_viewportIterator;
        foreachObserver(notifyViewportChanged(true));

        const int currentViewportPage = d->m_viewportIterator->pageNumber;
        if (oldViewportPage != currentViewportPage) {
            foreachObserver(notifyCurrentPageChanged(oldViewportPage, currentViewportPage));
        }
    }
}

void Document::setNextDocumentViewport(const DocumentViewport &viewport)
{
    d->m_nextDocumentViewport = viewport;
}

void Document::setNextDocumentDestination(const QString &namedDestination)
{
    d->m_nextDocumentDestination = namedDestination;
}

void Document::searchText(int searchID, const QString &text, bool fromStart, Qt::CaseSensitivity caseSensitivity, SearchType type, bool moveViewport, const QColor &color)
{
    d->m_searchCancelled = false;

    // safety checks: don't perform searches on empty or unsearchable docs
    if (!d->m_generator || !d->m_generator->hasFeature(Generator::TextExtraction) || d->m_pagesVector.isEmpty()) {
        Q_EMIT searchFinished(searchID, NoMatchFound);
        return;
    }

    // if searchID search not recorded, create new descriptor and init params
    auto searchIt = d->m_searches.find(searchID);
    if (searchIt == d->m_searches.end()) {
        RunningSearch *search = new RunningSearch();
        search->continueOnPage = -1;
        searchIt = d->m_searches.insert(searchID, search);
    }
    RunningSearch *s = *searchIt;

    // update search structure
    bool newText = text != s->cachedString;
    s->cachedString = text;
    s->cachedType = type;
    s->cachedCaseSensitivity = caseSensitivity;
    s->cachedViewportMove = moveViewport;
    s->cachedColor = color;
    s->isCurrentlySearching = true;

    // global data for search
    QSet<int> *pagesToNotify = new QSet<int>;

    // remove highlights from pages and queue them for notifying changes
    *pagesToNotify += s->highlightedPages;
    for (const int pageNumber : std::as_const(s->highlightedPages)) {
        d->m_pagesVector.at(pageNumber)->d->deleteHighlights(searchID);
    }
    s->highlightedPages.clear();

    // set hourglass cursor
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 1. ALLDOC - process all document marking pages
    if (type == AllDocument) {
        QHash<Page *, QList<RegularAreaRect *>> *pageMatches = new QHash<Page *, QList<RegularAreaRect *>>;

        // search and highlight 'text' (as a solid phrase) on all pages
        QTimer::singleShot(0, this, [this, pagesToNotify, pageMatches, searchID] { d->doContinueAllDocumentSearch(pagesToNotify, pageMatches, 0, searchID); });
    }
    // 2. NEXTMATCH - find next matching item (or start from top)
    // 3. PREVMATCH - find previous matching item (or start from bottom)
    else if (type == NextMatch || type == PreviousMatch) {
        // find out from where to start/resume search from
        const bool forward = type == NextMatch;
        const int viewportPage = (*d->m_viewportIterator).pageNumber;
        const int fromStartSearchPage = forward ? 0 : d->m_pagesVector.count() - 1;
        int currentPageNumber = fromStart ? fromStartSearchPage : ((s->continueOnPage != -1) ? s->continueOnPage : viewportPage);
        const Page *lastPage = fromStart ? nullptr : d->m_pagesVector[currentPageNumber];
        int pagesDone = 0;

        // continue checking last TextPage first (if it is the current page)
        RegularAreaRect *match = nullptr;
        if (lastPage && lastPage->number() == s->continueOnPage) {
            if (newText) {
                match = lastPage->findText(searchID, text, forward ? FromTop : FromBottom, caseSensitivity);
            } else {
                match = lastPage->findText(searchID, text, forward ? NextResult : PreviousResult, caseSensitivity, &s->continueOnMatch);
            }
            if (!match) {
                if (forward) {
                    currentPageNumber++;
                } else {
                    currentPageNumber--;
                }
                pagesDone++;
            }
        }

        s->pagesDone = pagesDone;

        DoContinueDirectionMatchSearchStruct *searchStruct = new DoContinueDirectionMatchSearchStruct();
        searchStruct->pagesToNotify = pagesToNotify;
        searchStruct->match = match;
        searchStruct->currentPage = currentPageNumber;
        searchStruct->searchID = searchID;

        QTimer::singleShot(0, this, [this, searchStruct] { d->doContinueDirectionMatchSearch(searchStruct); });
    }
    // 4. GOOGLE* - process all document marking pages
    else if (type == GoogleAll || type == GoogleAny) {
        QHash<Page *, QList<QPair<RegularAreaRect *, QColor>>> *pageMatches = new QHash<Page *, QList<QPair<RegularAreaRect *, QColor>>>;
        const QStringList words = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);

        // search and highlight every word in 'text' on all pages
        QTimer::singleShot(0, this, [this, pagesToNotify, pageMatches, searchID, words] { d->doContinueGooglesDocumentSearch(pagesToNotify, pageMatches, 0, searchID, words); });
    }
}

void Document::continueSearch(int searchID)
{
    // check if searchID is present in runningSearches
    auto it = d->m_searches.constFind(searchID);
    if (it == d->m_searches.constEnd()) {
        Q_EMIT searchFinished(searchID, NoMatchFound);
        return;
    }

    // start search with cached parameters from last search by searchID
    RunningSearch *p = *it;
    if (!p->isCurrentlySearching) {
        searchText(searchID, p->cachedString, false, p->cachedCaseSensitivity, p->cachedType, p->cachedViewportMove, p->cachedColor);
    }
}

void Document::continueSearch(int searchID, SearchType type)
{
    // check if searchID is present in runningSearches
    auto it = d->m_searches.constFind(searchID);
    if (it == d->m_searches.constEnd()) {
        Q_EMIT searchFinished(searchID, NoMatchFound);
        return;
    }

    // start search with cached parameters from last search by searchID
    RunningSearch *p = *it;
    if (!p->isCurrentlySearching) {
        searchText(searchID, p->cachedString, false, p->cachedCaseSensitivity, type, p->cachedViewportMove, p->cachedColor);
    }
}

void Document::resetSearch(int searchID)
{
    // if we are closing down, don't bother doing anything
    if (!d->m_generator) {
        return;
    }

    // check if searchID is present in runningSearches
    auto searchIt = d->m_searches.find(searchID);
    if (searchIt == d->m_searches.end()) {
        return;
    }

    // get previous parameters for search
    RunningSearch *s = *searchIt;

    // unhighlight pages and inform observers about that
    for (const int pageNumber : std::as_const(s->highlightedPages)) {
        d->m_pagesVector.at(pageNumber)->d->deleteHighlights(searchID);
        foreachObserver(notifyPageChanged(pageNumber, DocumentObserver::Highlights));
    }

    // send the setup signal too (to update views that filter on matches)
    foreachObserver(notifySetup(d->m_pagesVector, 0));

    // remove search from the runningSearches list and delete it
    d->m_searches.erase(searchIt);
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

void Document::editFormText(int pageNumber, Okular::FormFieldText *form, const QString &newContents, int newCursorPos, int prevCursorPos, int prevAnchorPos)
{
    QUndoCommand *uc = new EditFormTextCommand(this->d, form, pageNumber, newContents, newCursorPos, form->text(), prevCursorPos, prevAnchorPos);
    d->m_undoStack->push(uc);
}

void Document::editFormText(int pageNumber, Okular::FormFieldText *form, const QString &newContents, int newCursorPos, int prevCursorPos, int prevAnchorPos, const QString &oldContents)
{
    QUndoCommand *uc = new EditFormTextCommand(this->d, form, pageNumber, newContents, newCursorPos, oldContents, prevCursorPos, prevAnchorPos);
    d->m_undoStack->push(uc);
}

void Document::editFormList(int pageNumber, FormFieldChoice *form, const QList<int> &newChoices)
{
    const QList<int> prevChoices = form->currentChoices();
    QUndoCommand *uc = new EditFormListCommand(this->d, form, pageNumber, newChoices, prevChoices);
    d->m_undoStack->push(uc);
}

void Document::editFormCombo(int pageNumber, FormFieldChoice *form, const QString &newText, int newCursorPos, int prevCursorPos, int prevAnchorPos)
{
    QString prevText;
    if (form->currentChoices().isEmpty()) {
        prevText = form->editChoice();
    } else {
        prevText = form->choices().at(form->currentChoices().constFirst());
    }

    QUndoCommand *uc = new EditFormComboCommand(this->d, form, pageNumber, newText, newCursorPos, prevText, prevCursorPos, prevAnchorPos);
    d->m_undoStack->push(uc);
}

void Document::editFormButtons(int pageNumber, const QList<FormFieldButton *> &formButtons, const QList<bool> &newButtonStates)
{
    QUndoCommand *uc = new EditFormButtonsCommand(this->d, pageNumber, formButtons, newButtonStates);
    d->m_undoStack->push(uc);
}

void Document::reloadDocument() const
{
    const int numOfPages = pages();
    for (int i = currentPage(); i >= 0; i--) {
        d->refreshPixmaps(i);
    }
    for (int i = currentPage() + 1; i < numOfPages; i++) {
        d->refreshPixmaps(i);
    }
}

BookmarkManager *Document::bookmarkManager() const
{
    return d->m_bookmarkManager;
}

QList<int> Document::bookmarkedPageList() const
{
    QList<int> list;
    uint docPages = pages();

    // pages are 0-indexed internally, but 1-indexed externally
    for (uint i = 0; i < docPages; i++) {
        if (bookmarkManager()->isBookmarked(i)) {
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

    for (uint i = 0; i < docPages; ++i) {
        if (bookmarkManager()->isBookmarked(i)) {
            if (startId < 0) {
                startId = i;
            }
            if (endId < 0) {
                endId = startId;
            } else {
                ++endId;
            }
        } else if (startId >= 0 && endId >= 0) {
            if (!range.isEmpty()) {
                range += QLatin1Char(',');
            }

            if (endId - startId > 0) {
                range += QStringLiteral("%1-%2").arg(startId + 1).arg(endId + 1);
            } else {
                range += QString::number(startId + 1);
            }
            startId = -1;
            endId = -1;
        }
    }
    if (startId >= 0 && endId >= 0) {
        if (!range.isEmpty()) {
            range += QLatin1Char(',');
        }

        if (endId - startId > 0) {
            range += QStringLiteral("%1-%2").arg(startId + 1).arg(endId + 1);
        } else {
            range += QString::number(startId + 1);
        }
    }
    return range;
}

struct ExecuteNextActionsHelper : public QObject, private DocumentObserver {
    Q_OBJECT
public:
    explicit ExecuteNextActionsHelper(Document *doc)
        : m_doc(doc)
    {
        doc->addObserver(this);
        connect(doc, &Document::aboutToClose, this, [this] { b = false; });
    }

    ~ExecuteNextActionsHelper() override
    {
        m_doc->removeObserver(this);
    }

    void notifySetup(const QList<Okular::Page *> & /*pages*/, int setupFlags) override
    {
        if (setupFlags == DocumentChanged || setupFlags == UrlChanged) {
            b = false;
        }
    }

    bool shouldExecuteNextAction() const
    {
        return b;
    }

private:
    Document *const m_doc;
    bool b = true;
};

void Document::processAction(const Action *action)
{
    if (!action) {
        return;
    }

    // Don't execute next actions if the action itself caused the closing of the document
    const ExecuteNextActionsHelper executeNextActionsHelper(this);

    switch (action->actionType()) {
    case Action::Goto: {
        const GotoAction *go = static_cast<const GotoAction *>(action);
        d->m_nextDocumentViewport = go->destViewport();
        d->m_nextDocumentDestination = go->destinationName();

        // Explanation of why d->m_nextDocumentViewport is needed:
        // all openRelativeFile does is launch a signal telling we
        // want to open another URL, the problem is that when the file is
        // non local, the loading is done asynchronously so you can't
        // do a setViewport after the if as it was because you are doing the setViewport
        // on the old file and when the new arrives there is no setViewport for it and
        // it does not show anything

        // first open filename if link is pointing outside this document
        const QString filename = go->fileName();
        if (go->isExternal() && !d->openRelativeFile(filename)) {
            qCWarning(OkularCoreDebug).nospace() << "Action: Error opening '" << filename << "'.";
            break;
        } else {
            const DocumentViewport nextViewport = d->nextDocumentViewport();
            // skip local links that point to nowhere (broken ones)
            if (!nextViewport.isValid()) {
                break;
            }

            setViewport(nextViewport, nullptr, true);
            d->m_nextDocumentViewport = DocumentViewport();
            d->m_nextDocumentDestination = QString();
        }

    } break;

    case Action::Execute: {
        const ExecuteAction *exe = static_cast<const ExecuteAction *>(action);
        const QString fileName = exe->fileName();
        if (fileName.endsWith(QLatin1String(".pdf"), Qt::CaseInsensitive)) {
            d->openRelativeFile(fileName);
            break;
        }

        // Albert: the only pdf i have that has that kind of link don't define
        // an application and use the fileName as the file to open
        QUrl url = d->giveAbsoluteUrl(fileName);
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForUrl(url);
        // Check executables
        if (KIO::OpenUrlJob::isExecutableFile(url, mime.name())) {
            // Don't have any pdf that uses this code path, just a guess on how it should work
            if (!exe->parameters().isEmpty()) {
                url = d->giveAbsoluteUrl(exe->parameters());
                mime = db.mimeTypeForUrl(url);

                if (KIO::OpenUrlJob::isExecutableFile(url, mime.name())) {
                    // this case is a link pointing to an executable with a parameter
                    // that also is an executable, possibly a hand-crafted pdf
                    Q_EMIT error(i18n("The document is trying to execute an external application and, for your safety, Okular does not allow that."), -1);
                    break;
                }
            } else {
                // this case is a link pointing to an executable with no parameters
                // core developers find unacceptable executing it even after asking the user
                Q_EMIT error(i18n("The document is trying to execute an external application and, for your safety, Okular does not allow that."), -1);
                break;
            }
        }

        KIO::OpenUrlJob *job = new KIO::OpenUrlJob(url, mime.name());
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, d->m_widget));
        job->start();
        connect(job, &KIO::OpenUrlJob::result, this, [this, mime](KJob *job) {
            if (job->error()) {
                Q_EMIT error(i18n("No application found for opening file of mimetype %1.", mime.name()), -1);
            }
        });
    } break;

    case Action::DocAction: {
        const DocumentAction *docaction = static_cast<const DocumentAction *>(action);
        switch (docaction->documentActionType()) {
        case DocumentAction::PageFirst:
            setViewportPage(0);
            break;
        case DocumentAction::PagePrev:
            if ((*d->m_viewportIterator).pageNumber > 0) {
                setViewportPage((*d->m_viewportIterator).pageNumber - 1);
            }
            break;
        case DocumentAction::PageNext:
            if ((*d->m_viewportIterator).pageNumber < (int)d->m_pagesVector.count() - 1) {
                setViewportPage((*d->m_viewportIterator).pageNumber + 1);
            }
            break;
        case DocumentAction::PageLast:
            setViewportPage(d->m_pagesVector.count() - 1);
            break;
        case DocumentAction::HistoryBack:
            setPrevViewport();
            break;
        case DocumentAction::HistoryForward:
            setNextViewport();
            break;
        case DocumentAction::Quit:
            Q_EMIT quit();
            break;
        case DocumentAction::Presentation:
            Q_EMIT linkPresentation();
            break;
        case DocumentAction::EndPresentation:
            Q_EMIT linkEndPresentation();
            break;
        case DocumentAction::Find:
            Q_EMIT linkFind();
            break;
        case DocumentAction::GoToPage:
            Q_EMIT linkGoToPage();
            break;
        case DocumentAction::Close:
            Q_EMIT close();
            break;
        case DocumentAction::Print:
            Q_EMIT requestPrint();
            break;
        case DocumentAction::SaveAs:
            Q_EMIT requestSaveAs();
            break;
        }
    } break;

    case Action::Browse: {
        const BrowseAction *browse = static_cast<const BrowseAction *>(action);
        // if the url is a mailto one, invoke mailer
        if (browse->url().scheme() == QLatin1String("mailto")) {
            QDesktopServices::openUrl(browse->url());
        } else if (auto ref = extractLilyPondSourceReference(browse->url())) {
            processSourceReference(*ref);
        } else {
            const QUrl url = browse->url();

            // fix for #100366, documents with relative links that are the form of http:foo.pdf
            if ((url.scheme() == QLatin1String("http")) && url.host().isEmpty() && url.fileName().endsWith(QLatin1String("pdf"))) {
                d->openRelativeFile(url.fileName());
                break;
            }

            // handle documents with relative path
            QUrl realUrl;
            if (d->m_url.isValid()) {
                realUrl = KIO::upUrl(d->m_url).resolved(url);
            } else if (!url.isRelative()) {
                realUrl = url;
            }
            if (realUrl.isValid()) {
                auto *job = new KIO::OpenUrlJob(realUrl);
                job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, d->m_widget.data()));
                job->start();
            }
        }
    } break;

    case Action::Sound: {
        const SoundAction *linksound = static_cast<const SoundAction *>(action);
        AudioPlayer::instance()->playSound(linksound->sound(), linksound);
    } break;

    case Action::Script: {
        const ScriptAction *linkscript = static_cast<const ScriptAction *>(action);
        if (!d->m_scripter) {
            d->m_scripter = new Scripter(d);
        }
        d->m_scripter->execute(linkscript->scriptType(), linkscript->script());
    } break;

    case Action::Movie:
        Q_EMIT processMovieAction(static_cast<const MovieAction *>(action));
        break;
    case Action::Rendition: {
        const RenditionAction *linkrendition = static_cast<const RenditionAction *>(action);
        if (!linkrendition->script().isEmpty()) {
            if (!d->m_scripter) {
                d->m_scripter = new Scripter(d);
            }
            d->m_scripter->execute(linkrendition->scriptType(), linkrendition->script());
        }

        Q_EMIT processRenditionAction(static_cast<const RenditionAction *>(action));
    } break;
    case Action::BackendOpaque: {
        const BackendOpaqueAction *backendOpaqueAction = static_cast<const BackendOpaqueAction *>(action);
        Okular::BackendOpaqueAction::OpaqueActionResult res = d->m_generator->opaqueAction(backendOpaqueAction);
        if (res & Okular::BackendOpaqueAction::RefreshForms) {
            for (const Page *p : std::as_const(d->m_pagesVector)) {
                const QList<Okular::FormField *> forms = p->formFields();
                for (FormField *form : forms) {
                    Q_EMIT refreshFormWidget(form);
                }
                d->refreshPixmaps(p->number());
            }
        }
    } break;
    }

    if (executeNextActionsHelper.shouldExecuteNextAction()) {
        const QList<Action *> nextActions = action->nextActions();
        for (const Action *a : nextActions) {
            processAction(a);
        }
    }
}

void Document::processFormatAction(const Action *action, Okular::FormFieldText *fft)
{
    processFormatAction(action, static_cast<FormField *>(fft));
}

void Document::processFormatAction(const Action *action, Okular::FormField *ff)
{
    if (action->actionType() != Action::Script) {
        qCDebug(OkularCoreDebug) << "Unsupported action type" << action->actionType() << "for formatting.";
        return;
    }

    // Lookup the page of the FormFieldText
    int foundPage = d->findFieldPageNumber(ff);

    if (foundPage == -1) {
        qCDebug(OkularCoreDebug) << "Could not find page for formfield!";
        return;
    }

    const QString unformattedText = ff->value().toString();

    std::shared_ptr<Event> event = Event::createFormatEvent(ff, d->m_pagesVector[foundPage]);

    const ScriptAction *linkscript = static_cast<const ScriptAction *>(action);

    d->executeScriptEvent(event, linkscript);

    const QString formattedText = event->value().toString();
    ff->commitFormattedValue(formattedText);
    if (formattedText != unformattedText) {
        // We set the formattedText, because when we call refreshFormWidget
        // It will set the QLineEdit to this formattedText
        ff->setValue(QVariant(formattedText));
        ff->setAppearanceValue(QVariant(formattedText));
        Q_EMIT refreshFormWidget(ff);
        d->refreshPixmaps(foundPage);
        // Then we make the form have the unformatted text, to use
        // in calculations and other things
        ff->setValue(QVariant(unformattedText));
    } else if (ff->additionalAction(FormField::CalculateField)) {
        // When the field was calculated we need to refresh even
        // if the format script changed nothing. e.g. on error.
        // This is because the recalculateForms function delegated
        // the responsiblity for the refresh to us.
        Q_EMIT refreshFormWidget(ff);
        d->refreshPixmaps(foundPage);
    }
}

QString DocumentPrivate::evaluateKeystrokeEventChange(const QString &oldVal, const QString &newVal, int selStart, int selEnd)
{
    /*
        The change needs to be evaluated here in accordance with code points.
        selStart and selEnd parameters passed to this method should be been adjusted accordingly.

        Since QString methods work in terms of code units, we convert the strings to UTF-32.
    */
    std::u32string oldUcs4 = oldVal.toStdU32String();
    std::u32string newUcs4 = newVal.toStdU32String();
    if (selStart < 0 || selEnd < 0 || (selEnd - selStart) + (static_cast<int>(newUcs4.size()) - static_cast<int>(oldUcs4.size())) < 0) {
        // Prevent Okular from crashing if incorrect parameters are passed or some bug causes incorrect calculation
        return {};
    }
    const size_t changeLength = (selEnd - selStart) + (newUcs4.size() - oldUcs4.size());
    auto subview = std::u32string_view {newUcs4}.substr(selStart, changeLength);
    if (subview.empty()) {
        // If subview is empty (in scenarios when selStart is at end and changeLength is non-zero) fromUcs4 returns \u0000.
        // This should not happen, but just a guard.
        return {};
    }
    Q_ASSERT(subview.length() == changeLength);
    return QString::fromUcs4(subview.data(), subview.length());
}

void Document::processKeystrokeAction(const Action *action, Okular::FormField *ff, const QVariant &newValue, int prevCursorPos, int prevAnchorPos)
{
    if (action->actionType() != Action::Script) {
        qCDebug(OkularCoreDebug) << "Unsupported action type" << action->actionType() << "for keystroke.";
        return;
    }
    // Lookup the page of the FormFieldText
    int foundPage = d->findFieldPageNumber(ff);

    if (foundPage == -1) {
        qCDebug(OkularCoreDebug) << "Could not find page for formfield!";
        return;
    }

    std::shared_ptr<Event> event = Event::createKeystrokeEvent(ff, d->m_pagesVector[foundPage]);

    /* Set the selStart and selEnd event properties

       QString using UTF-16 counts a code point as made up of 1 or 2 16-bit code units.

       When encoded using 2 code units, the units are referred to as surrogate pairs.
       selectionStart() and selectionEnd() methods evaluate prevCursorPos and prevAnchorPos based on code units during selection.

       While this unit-based evaulation is suitable for detecting changes, for providing consistency with Adobe Reader for values of selStart and selEnd,
       it would be best to evaluate in terms of code points rather than the code units.

       To correct the values of selStart and selEnd accordingly, we iterate over the code units. If a surrogate pair is encountered, then selStart and
       selEnd are accordingly decremented.
    */
    int selStart = std::min(prevCursorPos, prevAnchorPos);
    int selEnd = std::max(prevCursorPos, prevAnchorPos);
    int codeUnit;
    int initialSelStart = selStart;
    int initialSelEnd = selEnd;
    QString inputString = ff->value().toString();
    for (codeUnit = 0; codeUnit < initialSelStart && codeUnit < inputString.size(); codeUnit++) {
        if (inputString.at(codeUnit).isHighSurrogate()) {
            // skip the low surrogate and decrement selStart and selEnd
            codeUnit++;
            selStart--;
            selEnd--;
        }
    }
    for (; codeUnit < initialSelEnd && codeUnit < inputString.size(); codeUnit++) {
        if (inputString.at(codeUnit).isHighSurrogate()) {
            // skip the low surrogate and decrement selEnd
            codeUnit++;
            selEnd--;
        }
    }
    std::u32string oldUcs4 = inputString.toStdU32String();
    std::u32string newUcs4 = newValue.toString().toStdU32String();
    // It is necessary to count size in terms of code points rather than code units for deletion.
    if (oldUcs4.size() - newUcs4.size() == 1 && selStart == selEnd) {
        // consider a one character removal as selection of that character and then its removal.
        selStart--;
    }
    event->setSelStart(selStart);
    event->setSelEnd(selEnd);
    // Use the corrected selStart and selEnd for evaluating the change.
    event->setChange(DocumentPrivate::evaluateKeystrokeEventChange(inputString, newValue.toString(), selStart, selEnd));
    const ScriptAction *linkscript = static_cast<const ScriptAction *>(action);

    d->executeScriptEvent(event, linkscript);

    if (event->returnCode()) {
        ff->setValue(newValue);
    } else {
        Q_EMIT refreshFormWidget(ff);
    }
}

void Document::processKeystrokeAction(const Action *action, Okular::FormFieldText *fft, const QVariant &newValue)
{
    // use -1 as default
    processKeystrokeAction(action, fft, newValue, -1, -1);
}

void Document::processKeystrokeCommitAction(const Action *action, Okular::FormFieldText *fft)
{
    bool returnCode = false;
    processKeystrokeCommitAction(action, fft, returnCode);
}

void Document::processKeystrokeCommitAction(const Action *action, Okular::FormField *ff, bool &returnCode)
{
    if (action->actionType() != Action::Script) {
        qCDebug(OkularCoreDebug) << "Unsupported action type" << action->actionType() << "for keystroke.";
        return;
    }
    // Lookup the page of the FormFieldText
    int foundPage = d->findFieldPageNumber(ff);

    if (foundPage == -1) {
        qCDebug(OkularCoreDebug) << "Could not find page for formfield!";
        return;
    }

    std::shared_ptr<Event> event = Event::createKeystrokeEvent(ff, d->m_pagesVector[foundPage]);
    event->setWillCommit(true);

    const ScriptAction *linkscript = static_cast<const ScriptAction *>(action);

    d->executeScriptEvent(event, linkscript);

    if (!event->returnCode()) {
        ff->setValue(QVariant(ff->committedFormattedValue()));
        Q_EMIT refreshFormWidget(ff);
        ff->setValue(QVariant(ff->committedValue()));
    } else {
        ff->setValue(QVariant(event->value().toString()));
        Q_EMIT refreshFormWidget(ff);
    }
    returnCode = event->returnCode();
}

void Document::processFocusAction(const Action *action, Okular::FormField *field)
{
    if (!action || action->actionType() != Action::Script) {
        return;
    }

    // Lookup the page of the FormFieldText
    int foundPage = d->findFieldPageNumber(field);

    if (foundPage == -1) {
        qCDebug(OkularCoreDebug) << "Could not find page for formfield!";
        return;
    }

    std::shared_ptr<Event> event = Event::createFormFocusEvent(field, d->m_pagesVector[foundPage]);

    const ScriptAction *linkscript = static_cast<const ScriptAction *>(action);

    d->executeScriptEvent(event, linkscript);
}

void Document::processValidateAction(const Action *action, Okular::FormFieldText *fft, bool &returnCode)
{
    processValidateAction(action, static_cast<FormField *>(fft), returnCode);
}

void Document::processValidateAction(const Action *action, Okular::FormField *ff, bool &returnCode)
{
    if (!action || action->actionType() != Action::Script) {
        return;
    }

    // Lookup the page of the FormFieldText
    int foundPage = d->findFieldPageNumber(ff);

    if (foundPage == -1) {
        qCDebug(OkularCoreDebug) << "Could not find page for formfield!";
        return;
    }

    std::shared_ptr<Event> event = Event::createFormValidateEvent(ff, d->m_pagesVector[foundPage]);

    const ScriptAction *linkscript = static_cast<const ScriptAction *>(action);

    d->executeScriptEvent(event, linkscript);
    if (!event->returnCode()) {
        ff->setValue(QVariant(ff->committedFormattedValue()));
        Q_EMIT refreshFormWidget(ff);
        ff->setValue(QVariant(ff->committedValue()));
    } else {
        ff->setValue(QVariant(event->value().toString()));
        Q_EMIT refreshFormWidget(ff);
    }
    returnCode = event->returnCode();
}

void Document::processKVCFActions(Okular::FormField *ff)
{
    if (ff->value().toString() == ff->committedValue()) {
        ff->setValue(QVariant(ff->committedFormattedValue()));
        Q_EMIT refreshFormWidget(ff);
        ff->setValue(QVariant(ff->committedValue()));
        return;
    }

    bool returnCode = true;
    if (ff->additionalAction(Okular::FormField::FieldModified) && !ff->isReadOnly()) {
        processKeystrokeCommitAction(ff->additionalAction(Okular::FormField::FieldModified), ff, returnCode);
    }

    if (const Okular::Action *action = ff->additionalAction(Okular::FormField::ValidateField)) {
        if (returnCode) {
            processValidateAction(action, ff, returnCode);
        }
    }

    if (!returnCode) {
        return;
    } else {
        ff->commitValue(ff->value().toString());
    }

    recalculateForms();

    if (const Okular::Action *action = ff->additionalAction(Okular::FormField::FormatField)) {
        processFormatAction(action, ff);
    } else {
        ff->commitFormattedValue(ff->value().toString());
    }
}

void Document::processDocumentAction(const Action *action, DocumentAdditionalActionType type)
{
    if (!action || action->actionType() != Action::Script) {
        return;
    }

    Event::EventType eventType = Okular::Event::UnknownEvent;

    switch (type) {
    case Document::CloseDocument:
        eventType = Okular::Event::DocWillClose;
        break;
    case Document::SaveDocumentStart:
        eventType = Okular::Event::DocWillSave;
        break;
    case Document::SaveDocumentFinish:
        eventType = Okular::Event::DocDidSave;
        break;
    case Document::PrintDocumentStart:
        eventType = Okular::Event::DocWillPrint;
        break;
    case Document::PrintDocumentFinish:
        eventType = Okular::Event::DocDidPrint;
        break;
    }

    std::shared_ptr<Event> event = Event::createDocEvent(eventType);

    const ScriptAction *linkScript = static_cast<const ScriptAction *>(action);

    d->executeScriptEvent(event, linkScript);
}

void Document::processFormMouseScriptAction(const Action *action, Okular::FormField *ff, MouseEventType fieldMouseEventType)
{
    if (!action || action->actionType() != Action::Script) {
        return;
    }

    Okular::Event::EventType eventType = Okular::Event::UnknownEvent;

    switch (fieldMouseEventType) {
    case Document::FieldMouseDown:
        eventType = Okular::Event::FieldMouseDown;
        break;
    case Document::FieldMouseEnter:
        eventType = Okular::Event::FieldMouseEnter;
        break;
    case Document::FieldMouseExit:
        eventType = Okular::Event::FieldMouseExit;
        break;
    case Document::FieldMouseUp:
        eventType = Okular::Event::FieldMouseUp;
        break;
    }

    // Lookup the page of the FormFieldText
    int foundPage = d->findFieldPageNumber(ff);

    if (foundPage == -1) {
        qCDebug(OkularCoreDebug) << "Could not find page for formfield!";
        return;
    }

    std::shared_ptr<Event> event = Event::createFieldMouseEvent(ff, d->m_pagesVector[foundPage], eventType);

    const ScriptAction *linkscript = static_cast<const ScriptAction *>(action);

    d->executeScriptEvent(event, linkscript);
}

void Document::processFormMouseUpScripAction(const Action *action, Okular::FormField *ff)
{
    processFormMouseScriptAction(action, ff, FieldMouseUp);
}

void Document::processSourceReference(const SourceReference *ref)
{
    if (ref) {
        processSourceReference(*ref);
    }
}

void Document::processSourceReference(const SourceReference &ref)
{
    const QUrl url = d->giveAbsoluteUrl(ref.fileName());
    if (!url.isLocalFile()) {
        qCDebug(OkularCoreDebug) << url.url() << "is not a local file.";
        return;
    }

    const QString absFileName = url.toLocalFile();
    if (!QFile::exists(absFileName)) {
        qCDebug(OkularCoreDebug) << "No such file:" << absFileName;
        return;
    }

    bool handled = false;
    Q_EMIT sourceReferenceActivated(absFileName, ref.row(), ref.column(), &handled);
    if (handled) {
        return;
    }

    static const QHash<int, QString> editors = buildEditorsMap();

    // prefer the editor from the command line
    QString p = d->editorCommandOverride;
    if (p.isEmpty()) {
        p = editors.value(SettingsCore::externalEditor());
    }
    if (p.isEmpty()) {
        p = SettingsCore::externalEditorCommand();
    }
    // custom editor not yet configured
    if (p.isEmpty()) {
        return;
    }

    // manually append the %f placeholder if not specified
    if (!p.contains(QLatin1String("%f"))) {
        p.append(QLatin1String(" %f"));
    }

    // replacing the placeholders
    QHash<QChar, QString> map;
    map.insert(QLatin1Char('f'), absFileName);
    map.insert(QLatin1Char('c'), QString::number(ref.column()));
    map.insert(QLatin1Char('l'), QString::number(ref.row()));
    const QString cmd = KMacroExpander::expandMacrosShellQuote(p, map);
    if (cmd.isEmpty()) {
        return;
    }
    QStringList args = KShell::splitArgs(cmd);
    if (args.isEmpty()) {
        return;
    }

    const QString prog = args.takeFirst();
    // Make sure prog is in PATH and not just in the CWD
    const QString progFullPath = QStandardPaths::findExecutable(prog);
    if (progFullPath.isEmpty()) {
        return;
    }

    KProcess::startDetached(progFullPath, args);
}

const SourceReference *Document::dynamicSourceReference(int pageNr, double absX, double absY)
{
    if (!d->m_synctex_scanner) {
        return nullptr;
    }

    const QSizeF dpi = d->m_generator->dpi();

    if (synctex_edit_query(d->m_synctex_scanner, pageNr + 1, absX * 72. / dpi.width(), absY * 72. / dpi.height()) > 0) {
        synctex_node_p node;
        // TODO what should we do if there is really more than one node?
        while ((node = synctex_scanner_next_result(d->m_synctex_scanner))) {
            int line = synctex_node_line(node);
            int col = synctex_node_column(node);
            // column extraction does not seem to be implemented in synctex so far. set the SourceReference default value.
            if (col == -1) {
                col = 0;
            }
            const char *name = synctex_scanner_get_name(d->m_synctex_scanner, synctex_node_tag(node));

            return new Okular::SourceReference(QFile::decodeName(name), line, col);
        }
    }
    return nullptr;
}

Document::PrintingType Document::printingSupport() const
{
    if (d->m_generator) {
        if (d->m_generator->hasFeature(Generator::PrintNative)) {
            return NativePrinting;
        }

#ifndef Q_OS_WIN
        if (d->m_generator->hasFeature(Generator::PrintPostscript)) {
            return PostscriptPrinting;
        }
#endif
    }

    return NoPrinting;
}

bool Document::supportsPrintToFile() const
{
    return d->m_generator ? d->m_generator->hasFeature(Generator::PrintToFile) : false;
}

Document::PrintError Document::print(QPrinter &printer)
{
    if (const Okular::Action *action = d->m_generator->additionalDocumentAction(PrintDocumentStart)) {
        processDocumentAction(action, PrintDocumentStart);
    }
    const Document::PrintError printError = d->m_generator ? d->m_generator->print(printer) : Document::UnknownPrintError;
    if (printError == Document::NoPrintError) {
        if (const Okular::Action *action = d->m_generator->additionalDocumentAction(PrintDocumentFinish)) {
            processDocumentAction(action, PrintDocumentFinish);
        }
    }
    return printError;
}

QString Document::printErrorString(PrintError error)
{
    switch (error) {
    case TemporaryFileOpenPrintError:
        return i18n("Could not open a temporary file");
    case FileConversionPrintError:
        return i18n("Print conversion failed");
    case PrintingProcessCrashPrintError:
        return i18n("Printing process crashed");
    case PrintingProcessStartPrintError:
        return i18n("Printing process could not start");
    case PrintToFilePrintError:
        return i18n("Printing to file failed");
    case InvalidPrinterStatePrintError:
        return i18n("Printer was in invalid state");
    case UnableToFindFilePrintError:
        return i18n("Unable to find file to print");
    case NoFileToPrintError:
        return i18n("There was no file to print");
    case NoBinaryToPrintError:
        return i18n("Could not find a suitable binary for printing. Make sure CUPS lpr binary is available");
    case InvalidPageSizePrintError:
        return i18n("The page print size is invalid");
    case NoPrintError:
        return QString();
    case UnknownPrintError:
        return QString();
    }

    return QString();
}

QWidget *Document::printConfigurationWidget() const
{
    if (d->m_generator) {
        PrintInterface *iface = qobject_cast<Okular::PrintInterface *>(d->m_generator);
        return iface ? iface->printConfigurationWidget() : nullptr;
    } else {
        return nullptr;
    }
}

void Document::fillConfigDialog(KConfigDialog *dialog)
{
    if (!dialog) {
        return;
    }

    // We know it's a BackendConfigDialog, but check anyway
    BackendConfigDialog *bcd = dynamic_cast<BackendConfigDialog *>(dialog);
    if (!bcd) {
        return;
    }

    // ensure that we have all the generators with settings loaded
    QList<KPluginMetaData> offers = DocumentPrivate::configurableGenerators();
    d->loadServiceList(offers);

    // We want the generators to be sorted by name so let's fill in a QMap
    // this sorts by internal id which is not awesome, but at least the sorting
    // is stable between runs that before it wasn't
    QMap<QString, GeneratorInfo> sortedGenerators;
    for (const auto &[key, value] : d->m_loadedGenerators.asKeyValueRange()) {
        sortedGenerators.insert(key, value);
    }

    bool pagesAdded = false;
    for (const auto &[key, value] : sortedGenerators.asKeyValueRange()) {
        Okular::ConfigInterface *iface = d->generatorConfig(value);
        if (iface) {
            iface->addPages(dialog);
            pagesAdded = true;

            if (value.generator == d->m_generator) {
                const int rowCount = bcd->thePageWidget()->model()->rowCount();
                KPageView *view = bcd->thePageWidget();
                view->setCurrentPage(view->model()->index(rowCount - 1, 0));
            }
        }
    }
    if (pagesAdded) {
        connect(dialog, &KConfigDialog::settingsChanged, this, [this] { d->slotGeneratorConfigChanged(); });
    }
}

QList<KPluginMetaData> DocumentPrivate::configurableGenerators()
{
    const QList<KPluginMetaData> available = availableGenerators();
    QList<KPluginMetaData> result;
    for (const KPluginMetaData &md : available) {
        if (md.rawData().value(QStringLiteral("X-KDE-okularHasInternalSettings")).toBool()) {
            result << md;
        }
    }
    return result;
}

KPluginMetaData Document::generatorInfo() const
{
    if (!d->m_generator) {
        return KPluginMetaData();
    }

    auto genIt = d->m_loadedGenerators.constFind(d->m_generatorName);
    Q_ASSERT(genIt != d->m_loadedGenerators.constEnd());
    return genIt.value().metadata;
}

int Document::configurableGenerators() const
{
    return DocumentPrivate::configurableGenerators().size();
}

QStringList Document::supportedMimeTypes() const
{
    // TODO: make it a static member of DocumentPrivate?
    QStringList result = d->m_supportedMimeTypes;
    if (result.isEmpty()) {
        const QList<KPluginMetaData> available = DocumentPrivate::availableGenerators();
        for (const KPluginMetaData &md : available) {
            result << md.mimeTypes();
        }

        // Remove duplicate mimetypes represented by different names
        QMimeDatabase mimeDatabase;
        QSet<QMimeType> uniqueMimetypes;
        for (const QString &mimeName : std::as_const(result)) {
            uniqueMimetypes.insert(mimeDatabase.mimeTypeForName(mimeName));
        }
        result.clear();
        for (const QMimeType &mimeType : uniqueMimetypes) {
            result.append(mimeType.name());
        }

        // Add the Okular archive mimetype
        result << QStringLiteral("application/vnd.kde.okular-archive");

        // Sorting by mimetype name doesn't make a ton of sense,
        // but ensures that the list is ordered the same way every time
        std::sort(result.begin(), result.end());

        d->m_supportedMimeTypes = result;
    }
    return result;
}

bool Document::canSwapBackingFile() const
{
    if (!d->m_generator) {
        return false;
    }

    return d->m_generator->hasFeature(Generator::SwapBackingFile);
}

bool Document::swapBackingFile(const QString &newFileName, const QUrl &url)
{
    if (!d->m_generator) {
        return false;
    }

    if (!d->m_generator->hasFeature(Generator::SwapBackingFile)) {
        return false;
    }

    // Save metadata about the file we're about to close
    d->saveDocumentInfo();

    d->clearAndWaitForRequests();

    qCDebug(OkularCoreDebug) << "Swapping backing file to" << newFileName;
    QList<Page *> newPagesVector;
    Generator::SwapBackingFileResult result = d->m_generator->swapBackingFile(newFileName, newPagesVector);
    if (result != Generator::SwapBackingFileError) {
        QList<ObjectRect *> rectsToDelete;
        QList<Annotation *> annotationsToDelete;
        QSet<PagePrivate *> pagePrivatesToDelete;

        if (result == Generator::SwapBackingFileReloadInternalData) {
            // Here we need to replace everything that the old generator
            // had created with what the new one has without making it look like
            // we have actually closed and opened the file again

            // Simple sanity check
            if (newPagesVector.count() != d->m_pagesVector.count()) {
                return false;
            }

            // Update the undo stack contents
            for (int i = 0; i < d->m_undoStack->count(); ++i) {
                // Trust me on the const_cast ^_^
                QUndoCommand *uc = const_cast<QUndoCommand *>(d->m_undoStack->command(i));
                if (OkularUndoCommand *ouc = dynamic_cast<OkularUndoCommand *>(uc)) {
                    const bool success = ouc->refreshInternalPageReferences(newPagesVector);
                    if (!success) {
                        qWarning() << "Document::swapBackingFile: refreshInternalPageReferences failed" << ouc;
                        return false;
                    }
                } else {
                    qWarning() << "Document::swapBackingFile: Unhandled undo command" << uc;
                    return false;
                }
            }

            for (int i = 0; i < d->m_pagesVector.count(); ++i) {
                // switch the PagePrivate* from newPage to oldPage
                // this way everyone still holding Page* doesn't get
                // disturbed by it
                Page *oldPage = d->m_pagesVector[i];
                Page *newPage = newPagesVector[i];
                newPage->d->adoptGeneratedContents(oldPage->d);

                pagePrivatesToDelete << oldPage->d;
                oldPage->d = newPage->d;
                oldPage->d->m_page = oldPage;
                oldPage->d->m_doc = d;
                newPage->d = nullptr;

                annotationsToDelete << oldPage->m_annotations;
                rectsToDelete << oldPage->m_rects;
                oldPage->m_annotations = newPage->m_annotations;
                oldPage->m_rects = newPage->m_rects;
            }
            qDeleteAll(newPagesVector);
            newPagesVector.clear();
        }

        d->m_url = url;
        d->m_docFileName = newFileName;
        d->updateMetadataXmlNameAndDocSize();
        d->m_bookmarkManager->setUrl(d->m_url);
        d->m_documentInfo = DocumentInfo();
        d->m_documentInfoAskedKeys.clear();

        if (d->m_synctex_scanner) {
            synctex_scanner_free(d->m_synctex_scanner);
            d->m_synctex_scanner = synctex_scanner_new_with_output_file(QFile::encodeName(newFileName).constData(), nullptr, 1);
            if (!d->m_synctex_scanner && QFile::exists(newFileName + QLatin1String("sync"))) {
                d->loadSyncFile(newFileName);
            }
        }

        foreachObserver(notifySetup(d->m_pagesVector, DocumentObserver::UrlChanged));

        qDeleteAll(annotationsToDelete);
        qDeleteAll(rectsToDelete);
        qDeleteAll(pagePrivatesToDelete);

        return true;
    } else {
        return false;
    }
}

bool Document::swapBackingFileArchive(const QString &newFileName, const QUrl &url)
{
    qCDebug(OkularCoreDebug) << "Swapping backing archive to" << newFileName;

    ArchiveData *newArchive = DocumentPrivate::unpackDocumentArchive(newFileName);
    if (!newArchive) {
        return false;
    }

    const QString tempFileName = newArchive->document.fileName();

    const bool success = swapBackingFile(tempFileName, url);

    if (success) {
        delete d->m_archiveData;
        d->m_archiveData = newArchive;
    }

    return success;
}

void Document::setHistoryClean(bool clean)
{
    if (clean) {
        d->m_undoStack->setClean();
    } else {
        d->m_undoStack->resetClean();
    }
}

bool Document::isHistoryClean() const
{
    return d->m_undoStack->isClean();
}

void Document::clearHistory()
{
    d->m_undoStack->clear();
}

bool Document::canSaveChanges() const
{
    if (!d->m_generator) {
        return false;
    }
    Q_ASSERT(!d->m_generatorName.isEmpty());

    auto genIt = d->m_loadedGenerators.find(d->m_generatorName);
    Q_ASSERT(genIt != d->m_loadedGenerators.end());
    SaveInterface *saveIface = d->generatorSave(genIt.value());
    if (!saveIface) {
        return false;
    }

    return saveIface->supportsOption(SaveInterface::SaveChanges);
}

bool Document::canSaveChanges(SaveCapability cap) const
{
    switch (cap) {
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

bool Document::saveChanges(const QString &fileName)
{
    QString errorText;
    return saveChanges(fileName, &errorText);
}

bool Document::saveChanges(const QString &fileName, QString *errorText)
{
    if (!d->m_generator || fileName.isEmpty()) {
        return false;
    }
    Q_ASSERT(!d->m_generatorName.isEmpty());

    auto genIt = d->m_loadedGenerators.find(d->m_generatorName);
    Q_ASSERT(genIt != d->m_loadedGenerators.end());
    SaveInterface *saveIface = d->generatorSave(genIt.value());
    if (!saveIface || !saveIface->supportsOption(SaveInterface::SaveChanges)) {
        return false;
    }

    if (const Okular::Action *action = d->m_generator->additionalDocumentAction(SaveDocumentStart)) {
        processDocumentAction(action, SaveDocumentStart);
    }

    const bool success = saveIface->save(fileName, SaveInterface::SaveChanges, errorText);
    if (success) {
        if (const Okular::Action *action = d->m_generator->additionalDocumentAction(SaveDocumentFinish)) {
            processDocumentAction(action, SaveDocumentFinish);
        }
    }
    return success;
}

void Document::registerView(View *view)
{
    if (!view) {
        return;
    }

    Document *viewDoc = view->viewDocument();
    if (viewDoc) {
        // check if already registered for this document
        if (viewDoc == this) {
            return;
        }

        viewDoc->unregisterView(view);
    }

    d->m_views.insert(view);
    view->d_func()->document = d;
}

void Document::unregisterView(View *view)
{
    if (!view) {
        return;
    }

    const Document *viewDoc = view->viewDocument();
    if (!viewDoc || viewDoc != this) {
        return;
    }

    view->d_func()->document = nullptr;
    d->m_views.remove(view);
}

QByteArray Document::fontData(const FontInfo &font) const
{
    if (d->m_generator) {
        return d->m_generator->requestFontData(font);
    }

    return {};
}

ArchiveData *DocumentPrivate::unpackDocumentArchive(const QString &archivePath)
{
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(archivePath, QMimeDatabase::MatchExtension);
    if (!mime.inherits(QStringLiteral("application/vnd.kde.okular-archive"))) {
        return nullptr;
    }

    KZip okularArchive(archivePath);
    if (!okularArchive.open(QIODevice::ReadOnly)) {
        return nullptr;
    }

    const KArchiveDirectory *mainDir = okularArchive.directory();

    // Check the archive doesn't have folders, we don't create them when saving the archive
    // and folders mean paths and paths mean path traversal issues
    const QStringList mainDirEntries = mainDir->entries();
    for (const QString &entry : mainDirEntries) {
        if (mainDir->entry(entry)->isDirectory()) {
            qWarning() << "Warning: Found a directory inside" << archivePath << " - Okular does not create files like that so it is most probably forged.";
            return nullptr;
        }
    }

    const KArchiveEntry *mainEntry = mainDir->entry(QStringLiteral("content.xml"));
    if (!mainEntry || !mainEntry->isFile()) {
        return nullptr;
    }

    std::unique_ptr<QIODevice> mainEntryDevice(static_cast<const KZipFileEntry *>(mainEntry)->createDevice());
    QDomDocument doc;
    if (!doc.setContent(mainEntryDevice.get())) {
        return nullptr;
    }
    mainEntryDevice.reset();

    QDomElement root = doc.documentElement();
    if (root.tagName() != QLatin1String("OkularArchive")) {
        return nullptr;
    }

    QString documentFileName;
    QString metadataFileName;
    QDomElement el = root.firstChild().toElement();
    for (; !el.isNull(); el = el.nextSibling().toElement()) {
        if (el.tagName() == QLatin1String("Files")) {
            QDomElement fileEl = el.firstChild().toElement();
            for (; !fileEl.isNull(); fileEl = fileEl.nextSibling().toElement()) {
                if (fileEl.tagName() == QLatin1String("DocumentFileName")) {
                    documentFileName = fileEl.text();
                } else if (fileEl.tagName() == QLatin1String("MetadataFileName")) {
                    metadataFileName = fileEl.text();
                }
            }
        }
    }
    if (documentFileName.isEmpty()) {
        return nullptr;
    }

    const KArchiveEntry *docEntry = mainDir->entry(documentFileName);
    if (!docEntry || !docEntry->isFile()) {
        return nullptr;
    }

    std::unique_ptr<ArchiveData> archiveData(new ArchiveData());
    const int dotPos = documentFileName.indexOf(QLatin1Char('.'));
    if (dotPos != -1) {
        archiveData->document.setFileTemplate(QDir::tempPath() + QLatin1String("/okular_XXXXXX") + documentFileName.mid(dotPos));
    }
    if (!archiveData->document.open()) {
        return nullptr;
    }

    archiveData->originalFileName = documentFileName;

    {
        std::unique_ptr<QIODevice> docEntryDevice(static_cast<const KZipFileEntry *>(docEntry)->createDevice());
        copyQIODevice(docEntryDevice.get(), &archiveData->document);
        archiveData->document.close();
    }

    const KArchiveEntry *metadataEntry = mainDir->entry(metadataFileName);
    if (metadataEntry && metadataEntry->isFile()) {
        std::unique_ptr<QIODevice> metadataEntryDevice(static_cast<const KZipFileEntry *>(metadataEntry)->createDevice());
        archiveData->metadataFile.setFileTemplate(QDir::tempPath() + QLatin1String("/okular_XXXXXX.xml"));
        if (archiveData->metadataFile.open()) {
            copyQIODevice(metadataEntryDevice.get(), &archiveData->metadataFile);
            archiveData->metadataFile.close();
        }
    }

    return archiveData.release();
}

Document::OpenResult Document::openDocumentArchive(const QString &docFile, const QUrl &url, const QString &password)
{
    d->m_archiveData = DocumentPrivate::unpackDocumentArchive(docFile);
    if (!d->m_archiveData) {
        return OpenError;
    }

    const QString tempFileName = d->m_archiveData->document.fileName();
    QMimeDatabase db;
    const QMimeType docMime = db.mimeTypeForFile(tempFileName, QMimeDatabase::MatchExtension);
    const OpenResult ret = openDocument(tempFileName, url, docMime, password);

    if (ret != OpenSuccess) {
        delete d->m_archiveData;
        d->m_archiveData = nullptr;
    }

    return ret;
}

bool Document::saveDocumentArchive(const QString &fileName)
{
    if (!d->m_generator) {
        return false;
    }

    /* If we opened an archive, use the name of original file (eg foo.pdf)
     * instead of the archive's one (eg foo.okular) */
    QString docFileName = d->m_archiveData ? d->m_archiveData->originalFileName : d->m_url.fileName();
    if (docFileName == QLatin1String("-")) {
        return false;
    }

    QString docPath = d->m_docFileName;
    const QFileInfo fi(docPath);
    if (fi.isSymLink()) {
        docPath = fi.symLinkTarget();
    }

    KZip okularArchive(fileName);
    if (!okularArchive.open(QIODevice::WriteOnly)) {
        return false;
    }

    const KUser user;
#ifndef Q_OS_WIN
    const KUserGroup userGroup(user.groupId());
#else
    const KUserGroup userGroup(QStringLiteral(""));
#endif

    QDomDocument contentDoc(QStringLiteral("OkularArchive"));
    QDomProcessingInstruction xmlPi = contentDoc.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"utf-8\""));
    contentDoc.appendChild(xmlPi);
    QDomElement root = contentDoc.createElement(QStringLiteral("OkularArchive"));
    contentDoc.appendChild(root);

    QDomElement filesNode = contentDoc.createElement(QStringLiteral("Files"));
    root.appendChild(filesNode);

    QDomElement fileNameNode = contentDoc.createElement(QStringLiteral("DocumentFileName"));
    filesNode.appendChild(fileNameNode);
    fileNameNode.appendChild(contentDoc.createTextNode(docFileName));

    QDomElement metadataFileNameNode = contentDoc.createElement(QStringLiteral("MetadataFileName"));
    filesNode.appendChild(metadataFileNameNode);
    metadataFileNameNode.appendChild(contentDoc.createTextNode(QStringLiteral("metadata.xml")));

    // If the generator can save annotations natively, do it
    QTemporaryFile modifiedFile;
    bool annotationsSavedNatively = false;
    bool formsSavedNatively = false;
    if (d->canAddAnnotationsNatively() || canSaveChanges(SaveFormsCapability)) {
        if (!modifiedFile.open()) {
            return false;
        }

        const QString modifiedFileName = modifiedFile.fileName();

        modifiedFile.close(); // We're only interested in the file name

        QString errorText;
        if (saveChanges(modifiedFileName, &errorText)) {
            docPath = modifiedFileName; // Save this instead of the original file
            annotationsSavedNatively = d->canAddAnnotationsNatively();
            formsSavedNatively = canSaveChanges(SaveFormsCapability);
        } else {
            qCWarning(OkularCoreDebug) << "saveChanges failed: " << errorText;
            qCDebug(OkularCoreDebug) << "Falling back to saving a copy of the original file";
        }
    }

    PageItems saveWhat = None;
    if (!annotationsSavedNatively) {
        saveWhat |= AnnotationPageItems;
    }
    if (!formsSavedNatively) {
        saveWhat |= FormFieldPageItems;
    }

    QTemporaryFile metadataFile;
    if (!d->savePageDocumentInfo(&metadataFile, saveWhat)) {
        return false;
    }

    const QByteArray contentDocXml = contentDoc.toByteArray();
    const mode_t perm = 0100644;
    okularArchive.writeFile(QStringLiteral("content.xml"), contentDocXml, perm, user.loginName(), userGroup.name());

    okularArchive.addLocalFile(docPath, docFileName);
    okularArchive.addLocalFile(metadataFile.fileName(), QStringLiteral("metadata.xml"));

    if (!okularArchive.close()) {
        return false;
    }

    return true;
}

bool Document::extractArchivedFile(const QString &destFileName)
{
    if (!d->m_archiveData) {
        return false;
    }

    // Remove existing file, if present (QFile::copy doesn't overwrite by itself)
    QFile::remove(destFileName);

    return d->m_archiveData->document.copy(destFileName);
}

QPageLayout::Orientation Document::orientation() const
{
    int landscape, portrait;

    // if some pages are landscape and others are not, the most common wins, as
    // QPrinter does not accept a per-page setting
    landscape = 0;
    portrait = 0;
    for (Page *const current : std::as_const(d->m_pagesVector)) {
        double width = current->width();
        double height = current->height();
        if (current->orientation() == Okular::Rotation90 || current->orientation() == Okular::Rotation270) {
            std::swap(width, height);
        }
        if (width > height) {
            landscape++;
        } else {
            portrait++;
        }
    }
    return (landscape > portrait) ? QPageLayout::Landscape : QPageLayout::Portrait;
}

void Document::setAnnotationEditingEnabled(bool enable)
{
    d->m_annotationEditingEnabled = enable;
    foreachObserver(notifySetup(d->m_pagesVector, 0));
}

void Document::walletDataForFile(const QString &fileName, QString *walletName, QString *walletFolder, QString *walletKey) const
{
    if (d->m_generator) {
        d->m_generator->walletDataForFile(fileName, walletName, walletFolder, walletKey);
    } else if (d->m_walletGenerator) {
        d->m_walletGenerator->walletDataForFile(fileName, walletName, walletFolder, walletKey);
    }
}

bool Document::isDocdataMigrationNeeded() const
{
    return d->m_docdataMigrationNeeded;
}

void Document::docdataMigrationDone()
{
    if (d->m_docdataMigrationNeeded) {
        d->m_docdataMigrationNeeded = false;
        foreachObserver(notifySetup(d->m_pagesVector, 0));
    }
}

QAbstractItemModel *Document::layersModel() const
{
    return d->m_generator ? d->m_generator->layersModel() : nullptr;
}

QString Document::openError() const
{
    return d->m_openError;
}

QByteArray Document::requestSignedRevisionData(const Okular::SignatureInfo &info)
{
    QFile f(d->m_docFileName);
    if (!f.open(QIODevice::ReadOnly)) {
        Q_EMIT error(i18n("Could not open '%1'. File does not exist", d->m_docFileName), -1);
        return {};
    }

    const QList<qint64> byteRange = info.signedRangeBounds();
    f.seek(byteRange.first());
    QByteArray data = f.read(byteRange.last() - byteRange.first());
    f.close();

    return data;
}

void Document::refreshPixmaps(int pageNumber)
{
    d->refreshPixmaps(pageNumber);
}

void DocumentPrivate::executeScript(const QString &function)
{
    if (!m_scripter) {
        m_scripter = new Scripter(this);
    }
    m_scripter->execute(JavaScript, function);
}

void DocumentPrivate::requestDone(PixmapRequest *req)
{
    if (!req) {
        return;
    }

    if (!m_generator || m_closingLoop) {
        m_pixmapRequestsMutex.lock();
        m_executingPixmapRequests.remove(req);
        m_pixmapRequestsMutex.unlock();

        delete req;
        req = nullptr;

        if (m_closingLoop) {
            m_closingLoop->exit();
        }
        return;
    }

#ifndef NDEBUG
    if (!m_generator->canGeneratePixmap()) {
        qCDebug(OkularCoreDebug) << "requestDone with generator not in READY state.";
    }
#endif

    if (!req->shouldAbortRender()) {
        // [MEM] 1.1 find and remove a previous entry for the same page and id
        auto it = std::ranges::find_if(m_allocatedPixmaps, [&](AllocatedPixmap *const p) { //
            return p->page == req->pageNumber() && p->observer == req->observer();
        });
        if (it != m_allocatedPixmaps.end()) {
            AllocatedPixmap *p = *it;
            m_allocatedPixmaps.erase(it);
            m_allocatedPixmapsTotalMemory -= p->memory;
            delete p;
        }

        DocumentObserver *observer = req->observer();
        if (m_observers.contains(observer)) {
            // [MEM] 1.2 append memory allocation descriptor to the FIFO
            qulonglong memoryBytes = 0;
            const TilesManager *tm = req->d->tilesManager();
            if (tm) {
                memoryBytes = tm->totalMemory();
            } else {
                memoryBytes = 4 * qulonglong(req->width()) * req->height();
            }

            AllocatedPixmap *memoryPage = new AllocatedPixmap(req->observer(), req->pageNumber(), memoryBytes);
            m_allocatedPixmaps.push_back(memoryPage);
            m_allocatedPixmapsTotalMemory += memoryBytes;

            // 2. notify an observer that its pixmap changed
            observer->notifyPageChanged(req->pageNumber(), DocumentObserver::Pixmap);
        }
#ifndef NDEBUG
        else {
            qCWarning(OkularCoreDebug) << "Receiving a done request for the defunct observer" << observer;
        }
#endif
    }

    // 3. delete request
    m_pixmapRequestsMutex.lock();
    m_executingPixmapRequests.remove(req);
    m_pixmapRequestsMutex.unlock();

    delete req;
    req = nullptr;

    // 4. start a new generation if some is pending
    m_pixmapRequestsMutex.lock();
    bool hasPixmaps = !m_pixmapRequestsStack.empty();
    m_pixmapRequestsMutex.unlock();
    if (hasPixmaps) {
        sendGeneratorPixmapRequest();
    }
}

void DocumentPrivate::setPageBoundingBox(int page, const NormalizedRect &boundingBox)
{
    Page *kp = m_pagesVector[page];
    if (!m_generator || !kp) {
        return;
    }

    if (kp->boundingBox() == boundingBox) {
        return;
    }
    kp->setBoundingBox(boundingBox);

    // notify observers about the change
    foreachObserverD(notifyPageChanged(page, DocumentObserver::BoundingBox));

    // TODO: For generators that generate the bbox by pixmap scanning, if the first generated pixmap is very small, the bounding box will forever be inaccurate.
    // TODO: Crop computation should also consider annotations, actions, etc. to make sure they're not cropped away.
    // TODO: Help compute bounding box for generators that create a QPixmap without a QImage, like text and plucker.
    // TODO: Don't compute the bounding box if no one needs it (e.g., Trim Borders is off).
}

void DocumentPrivate::calculateMaxTextPages()
{
    int multipliers = qMax(1, qRound(getTotalMemory() / 536870912.0)); // 512 MB
    switch (SettingsCore::memoryLevel()) {
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

void DocumentPrivate::textGenerationDone(Page *page)
{
    if (!m_pageController) {
        return;
    }

    // 1. If we reached the cache limit, delete the first text page from the fifo
    if (m_allocatedTextPagesFifo.size() == m_maxAllocatedTextPages) {
        int pageToKick = m_allocatedTextPagesFifo.takeFirst();
        if (pageToKick != page->number()) // this should never happen but better be safe than sorry
        {
            m_pagesVector.at(pageToKick)->setTextPage(nullptr); // deletes the textpage
        }
    }

    // 2. Add the page to the fifo of generated text pages
    m_allocatedTextPagesFifo.append(page->number());
}

void Document::setRotation(int r)
{
    d->setRotationInternal(r, true);
}

void DocumentPrivate::setRotationInternal(int r, bool notify)
{
    Rotation rotation = (Rotation)r;
    if (!m_generator || (m_rotation == rotation)) {
        return;
    }

    // tell the pages to rotate
    for (Page *const page : std::as_const(m_pagesVector)) {
        page->d->rotateAt(rotation);
    }
    if (notify) {
        // notify the generator that the current rotation has changed
        m_generator->rotationChanged(rotation, m_rotation);
    }
    // set the new rotation
    m_rotation = rotation;

    if (notify) {
        foreachObserverD(notifySetup(m_pagesVector, DocumentObserver::NewLayoutForPages));
        foreachObserverD(notifyContentsCleared(DocumentObserver::Pixmap | DocumentObserver::Highlights | DocumentObserver::Annotations));
    }
    qCDebug(OkularCoreDebug) << "Rotated:" << r;
}

void Document::setPageSize(const PageSize &size)
{
    if (!d->m_generator || !d->m_generator->hasFeature(Generator::PageSizes)) {
        return;
    }

    if (d->m_pageSizes.isEmpty()) {
        d->m_pageSizes = d->m_generator->pageSizes();
    }
    int sizeid = d->m_pageSizes.indexOf(size);
    if (sizeid == -1) {
        return;
    }

    // tell the pages to change size
    for (Page *const page : std::as_const(d->m_pagesVector)) {
        page->d->changeSize(size);
    }
    // clear 'memory allocation' descriptors
    qDeleteAll(d->m_allocatedPixmaps);
    d->m_allocatedPixmaps.clear();
    d->m_allocatedPixmapsTotalMemory = 0;
    // notify the generator that the current page size has changed
    d->m_generator->pageSizeChanged(size, d->m_pageSize);
    // set the new page size
    d->m_pageSize = size;

    foreachObserver(notifySetup(d->m_pagesVector, DocumentObserver::NewLayoutForPages));
    foreachObserver(notifyContentsCleared(DocumentObserver::Pixmap | DocumentObserver::Highlights));
    qCDebug(OkularCoreDebug) << "New PageSize id:" << sizeid;
}

/** DocumentViewport **/

DocumentViewport::DocumentViewport(int n)
    : pageNumber(n)
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

DocumentViewport::DocumentViewport(const QString &xmlDesc)
    : pageNumber(-1)
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
    if (xmlDesc.isEmpty()) {
        return;
    }

    // decode the string
    bool ok;
    int field = 0;
    QString token = xmlDesc.section(QLatin1Char(';'), field, field);
    while (!token.isEmpty()) {
        // decode the current token
        if (field == 0) {
            pageNumber = token.toInt(&ok);
            if (!ok) {
                return;
            }
        } else if (token.startsWith(QLatin1String("C1"))) {
            rePos.enabled = true;
            rePos.normalizedX = token.section(QLatin1Char(':'), 1, 1).toDouble();
            rePos.normalizedY = token.section(QLatin1Char(':'), 2, 2).toDouble();
            rePos.pos = Center;
        } else if (token.startsWith(QLatin1String("C2"))) {
            rePos.enabled = true;
            rePos.normalizedX = token.section(QLatin1Char(':'), 1, 1).toDouble();
            rePos.normalizedY = token.section(QLatin1Char(':'), 2, 2).toDouble();
            if (token.section(QLatin1Char(':'), 3, 3).toInt() == 1) {
                rePos.pos = Center;
            } else {
                rePos.pos = TopLeft;
            }
        } else if (token.startsWith(QLatin1String("AF1"))) {
            autoFit.enabled = true;
            autoFit.width = token.section(QLatin1Char(':'), 1, 1) == QLatin1String("T");
            autoFit.height = token.section(QLatin1Char(':'), 2, 2) == QLatin1String("T");
        }
        // proceed tokenizing string
        field++;
        token = xmlDesc.section(QLatin1Char(';'), field, field);
    }
}

QString DocumentViewport::toString() const
{
    // start string with page number
    QString s = QString::number(pageNumber);
    // if has center coordinates, save them on string
    if (rePos.enabled) {
        s += QStringLiteral(";C2:") + QString::number(rePos.normalizedX) + QLatin1Char(':') + QString::number(rePos.normalizedY) + QLatin1Char(':') + QString::number(rePos.pos);
    }
    // if has autofit enabled, save its state on string
    if (autoFit.enabled) {
        s += QStringLiteral(";AF1:") + (autoFit.width ? QLatin1Char('T') : QLatin1Char('F')) + QLatin1Char(':') + (autoFit.height ? QLatin1Char('T') : QLatin1Char('F'));
    }
    return s;
}

bool DocumentViewport::isValid() const
{
    return pageNumber >= 0;
}

bool DocumentViewport::operator==(const DocumentViewport &other) const
{
    bool equal = (pageNumber == other.pageNumber) && (rePos.enabled == other.rePos.enabled) && (autoFit.enabled == other.autoFit.enabled);
    if (!equal) {
        return false;
    }
    if (rePos.enabled && ((rePos.normalizedX != other.rePos.normalizedX) || (rePos.normalizedY != other.rePos.normalizedY) || rePos.pos != other.rePos.pos)) {
        return false;
    }
    if (autoFit.enabled && ((autoFit.width != other.autoFit.width) || (autoFit.height != other.autoFit.height))) {
        return false;
    }
    return true;
}

bool DocumentViewport::operator<(const DocumentViewport &other) const
{
    // TODO: Check autoFit and Position

    if (pageNumber != other.pageNumber) {
        return pageNumber < other.pageNumber;
    }

    if (!rePos.enabled && other.rePos.enabled) {
        return true;
    }

    if (!other.rePos.enabled) {
        return false;
    }

    if (rePos.normalizedY != other.rePos.normalizedY) {
        return rePos.normalizedY < other.rePos.normalizedY;
    }

    return rePos.normalizedX < other.rePos.normalizedX;
}

/** DocumentInfo **/

DocumentInfo::DocumentInfo()
    : d(new DocumentInfoPrivate())
{
}

DocumentInfo::DocumentInfo(const DocumentInfo &info)
    : d(new DocumentInfoPrivate())
{
    *this = info;
}

DocumentInfo &DocumentInfo::operator=(const DocumentInfo &info)
{
    if (this != &info) {
        d->values = info.d->values;
        d->titles = info.d->titles;
    }
    return *this;
}

DocumentInfo::~DocumentInfo()
{
    delete d;
}

void DocumentInfo::set(const QString &key, const QString &value, const QString &title)
{
    d->values[key] = value;
    d->titles[key] = title;
}

void DocumentInfo::set(Key key, const QString &value)
{
    d->values[getKeyString(key)] = value;
}

QStringList DocumentInfo::keys() const
{
    return d->values.keys();
}

QString DocumentInfo::get(Key key) const
{
    return get(getKeyString(key));
}

QString DocumentInfo::get(const QString &key) const
{
    return d->values[key];
}

QString DocumentInfo::getKeyString(Key key) // const
{
    switch (key) {
    case Title:
        return QStringLiteral("title");
        break;
    case Subject:
        return QStringLiteral("subject");
        break;
    case Description:
        return QStringLiteral("description");
        break;
    case Author:
        return QStringLiteral("author");
        break;
    case Creator:
        return QStringLiteral("creator");
        break;
    case Producer:
        return QStringLiteral("producer");
        break;
    case Copyright:
        return QStringLiteral("copyright");
        break;
    case Pages:
        return QStringLiteral("pages");
        break;
    case CreationDate:
        return QStringLiteral("creationDate");
        break;
    case ModificationDate:
        return QStringLiteral("modificationDate");
        break;
    case MimeType:
        return QStringLiteral("mimeType");
        break;
    case Category:
        return QStringLiteral("category");
        break;
    case Keywords:
        return QStringLiteral("keywords");
        break;
    case FilePath:
        return QStringLiteral("filePath");
        break;
    case DocumentSize:
        return QStringLiteral("documentSize");
        break;
    case PagesSize:
        return QStringLiteral("pageSize");
        break;
    default:
        qCWarning(OkularCoreDebug) << "Unknown" << key;
        return QString();
        break;
    }
}

DocumentInfo::Key DocumentInfo::getKeyFromString(const QString &key) // const
{
    if (key == QLatin1String("title")) {
        return Title;
    } else if (key == QLatin1String("subject")) {
        return Subject;
    } else if (key == QLatin1String("description")) {
        return Description;
    } else if (key == QLatin1String("author")) {
        return Author;
    } else if (key == QLatin1String("creator")) {
        return Creator;
    } else if (key == QLatin1String("producer")) {
        return Producer;
    } else if (key == QLatin1String("copyright")) {
        return Copyright;
    } else if (key == QLatin1String("pages")) {
        return Pages;
    } else if (key == QLatin1String("creationDate")) {
        return CreationDate;
    } else if (key == QLatin1String("modificationDate")) {
        return ModificationDate;
    } else if (key == QLatin1String("mimeType")) {
        return MimeType;
    } else if (key == QLatin1String("category")) {
        return Category;
    } else if (key == QLatin1String("keywords")) {
        return Keywords;
    } else if (key == QLatin1String("filePath")) {
        return FilePath;
    } else if (key == QLatin1String("documentSize")) {
        return DocumentSize;
    } else if (key == QLatin1String("pageSize")) {
        return PagesSize;
    } else {
        return Invalid;
    }
}

QString DocumentInfo::getKeyTitle(Key key) // const
{
    switch (key) {
    case Title:
        return i18n("Title");
        break;
    case Subject:
        return i18n("Subject");
        break;
    case Description:
        return i18n("Description");
        break;
    case Author:
        return i18n("Author");
        break;
    case Creator:
        return i18n("Creator");
        break;
    case Producer:
        return i18n("Producer");
        break;
    case Copyright:
        return i18n("Copyright");
        break;
    case Pages:
        return i18n("Pages");
        break;
    case CreationDate:
        return i18n("Created");
        break;
    case ModificationDate:
        return i18n("Modified");
        break;
    case MimeType:
        return i18n("MIME Type");
        break;
    case Category:
        return i18n("Category");
        break;
    case Keywords:
        return i18n("Keywords");
        break;
    case FilePath:
        return i18n("File Path");
        break;
    case DocumentSize:
        return i18n("File Size");
        break;
    case PagesSize:
        return i18n("Page Size");
        break;
    default:
        return QString();
        break;
    }
}

QString DocumentInfo::getKeyTitle(const QString &key) const
{
    QString title = getKeyTitle(getKeyFromString(key));
    if (title.isEmpty()) {
        title = d->titles[key];
    }
    return title;
}

/** DocumentSynopsis **/

DocumentSynopsis::DocumentSynopsis()
    : QDomDocument(QStringLiteral("DocumentSynopsis"))
{
    // void implementation, only subclassed for naming
}

DocumentSynopsis::DocumentSynopsis(const QDomDocument &document)
    : QDomDocument(document)
{
}

/** EmbeddedFile **/

EmbeddedFile::EmbeddedFile()
{
}

EmbeddedFile::~EmbeddedFile()
{
}

VisiblePageRect::VisiblePageRect(int page, const NormalizedRect &rectangle)
    : pageNumber(page)
    , rect(rectangle)
{
}

/** NewSignatureData **/

struct Okular::NewSignatureDataPrivate {
    NewSignatureDataPrivate() = default;

    QString certNickname;
    QString certSubjectCommonName;
    QString password;
    QString documentPassword;
    QString location;
    QString reason;
    QString backgroundImagePath;
    double fontSize = 10;
    double leftFontSize = 20;
    int page = -1;
    NormalizedRect boundingRectangle;
};

NewSignatureData::NewSignatureData()
    : d(new NewSignatureDataPrivate())
{
}

NewSignatureData::~NewSignatureData()
{
    delete d;
}

QString NewSignatureData::certNickname() const
{
    return d->certNickname;
}

void NewSignatureData::setCertNickname(const QString &certNickname)
{
    d->certNickname = certNickname;
}

QString NewSignatureData::certSubjectCommonName() const
{
    return d->certSubjectCommonName;
}

void NewSignatureData::setCertSubjectCommonName(const QString &certSubjectCommonName)
{
    d->certSubjectCommonName = certSubjectCommonName;
}

QString NewSignatureData::password() const
{
    return d->password;
}

void NewSignatureData::setPassword(const QString &password)
{
    d->password = password;
}

int NewSignatureData::page() const
{
    return d->page;
}

void NewSignatureData::setPage(int page)
{
    d->page = page;
}

NormalizedRect NewSignatureData::boundingRectangle() const
{
    return d->boundingRectangle;
}

void NewSignatureData::setBoundingRectangle(const NormalizedRect &rect)
{
    d->boundingRectangle = rect;
}

QString NewSignatureData::documentPassword() const
{
    return d->documentPassword;
}

void NewSignatureData::setDocumentPassword(const QString &password)
{
    d->documentPassword = password;
}

QString NewSignatureData::location() const
{
    return d->location;
}

void NewSignatureData::setLocation(const QString &location)
{
    d->location = location;
}

QString NewSignatureData::reason() const
{
    return d->reason;
}

void NewSignatureData::setReason(const QString &reason)
{
    d->reason = reason;
}

QString Okular::NewSignatureData::backgroundImagePath() const
{
    return d->backgroundImagePath;
}

void Okular::NewSignatureData::setBackgroundImagePath(const QString &path)
{
    d->backgroundImagePath = path;
}

double Okular::NewSignatureData::fontSize() const
{
    return d->fontSize;
}

void Okular::NewSignatureData::setFontSize(double fontSize)
{
    d->fontSize = fontSize;
}

double Okular::NewSignatureData::leftFontSize() const
{
    return d->leftFontSize;
}

void Okular::NewSignatureData::setLeftFontSize(double fontSize)
{
    d->leftFontSize = fontSize;
}

#undef foreachObserver
#undef foreachObserverD

#include "document.moc"

/* kate: replace-tabs on; indent-width 4; */
