/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "documentitem.h"

#include <QMimeDatabase>
#include <QtQml> // krazy:exclude=includes

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <QtAndroid>
#endif

#include <core/bookmarkmanager.h>
#include <core/document_p.h>
#include <core/page.h>

#include "part/tocmodel.h"

DocumentItem::DocumentItem(QObject *parent)
    : QObject(parent)
    , m_thumbnailObserver(nullptr)
    , m_pageviewObserver(nullptr)
    , m_searchInProgress(false)
{
    qmlRegisterUncreatableType<TOCModel>("org.kde.okular.private", 1, 0, "TOCModel", QStringLiteral("Do not create objects of this type."));
    Okular::Settings::instance(QStringLiteral("okularproviderrc"));
    m_document = new Okular::Document(nullptr);
    m_tocModel = new TOCModel(m_document, this);

    connect(m_document, &Okular::Document::searchFinished, this, &DocumentItem::searchFinished);
    connect(m_document->bookmarkManager(), &Okular::BookmarkManager::bookmarksChanged, this, &DocumentItem::bookmarkedPagesChanged);
    connect(m_document->bookmarkManager(), &Okular::BookmarkManager::bookmarksChanged, this, &DocumentItem::bookmarksChanged);
}

DocumentItem::~DocumentItem()
{
    delete m_document;
}

void DocumentItem::setUrl(const QUrl &url)
{
    m_document->closeDocument();
    // TODO: password
    QMimeDatabase db;

    QUrl realUrl = url; // NOLINT(performance-unnecessary-copy-initialization) because of the ifdef below this can't be const &

#ifdef Q_OS_ANDROID
    realUrl = QUrl(QtAndroid::androidActivity().callObjectMethod("contentUrlToFd", "(Ljava/lang/String;)Ljava/lang/String;", QAndroidJniObject::fromString(url.toString()).object<jstring>()).toString());
#endif

    const QString path = realUrl.isLocalFile() ? realUrl.toLocalFile() : QStringLiteral("-");

    m_document->openDocument(path, realUrl, db.mimeTypeForUrl(realUrl));

    m_tocModel->clear();
    m_tocModel->fill(m_document->documentSynopsis());
    m_tocModel->setCurrentViewport(m_document->viewport());

    m_matchingPages.clear();
    for (uint i = 0; i < m_document->pages(); ++i) {
        m_matchingPages << (int)i;
    }
    emit matchingPagesChanged();
    emit urlChanged();
    emit pageCountChanged();
    emit openedChanged();
    emit supportsSearchingChanged();
    emit windowTitleForDocumentChanged();
    emit bookmarkedPagesChanged();
}

QString DocumentItem::windowTitleForDocument() const
{
    // If 'DocumentTitle' should be used, check if the document has one. If
    // either case is false, use the file name.
    QString title = Okular::Settings::displayDocumentNameOrPath() == Okular::Settings::EnumDisplayDocumentNameOrPath::Path ? m_document->currentDocument().toDisplayString(QUrl::PreferLocalFile) : m_document->currentDocument().fileName();

    if (Okular::Settings::displayDocumentTitle()) {
        const QString docTitle = m_document->metaData(QStringLiteral("DocumentTitle")).toString();

        if (!docTitle.isEmpty() && !docTitle.trimmed().isEmpty()) {
            title = docTitle;
        }
    }

    return title;
}

QUrl DocumentItem::url() const
{
    return m_document->currentDocument();
}

void DocumentItem::setCurrentPage(int page)
{
    m_document->setViewportPage(page);
    m_tocModel->setCurrentViewport(m_document->viewport());
    emit currentPageChanged();
}

int DocumentItem::currentPage() const
{
    return m_document->currentPage();
}

bool DocumentItem::isOpened() const
{
    return m_document->isOpened();
}

int DocumentItem::pageCount() const
{
    return m_document->pages();
}

QVariantList DocumentItem::matchingPages() const
{
    return m_matchingPages;
}

TOCModel *DocumentItem::tableOfContents() const
{
    return m_tocModel;
}

QVariantList DocumentItem::bookmarkedPages() const
{
    QList<int> list;
    QSet<int> pages;
    const KBookmark::List bMarks = m_document->bookmarkManager()->bookmarks();
    for (const KBookmark &bookmark : bMarks) {
        Okular::DocumentViewport viewport(bookmark.url().fragment());
        pages << viewport.pageNumber;
    }
    list = pages.values();
    std::sort(list.begin(), list.end());

    QVariantList variantList;
    for (const int page : qAsConst(list)) {
        variantList << page;
    }
    return variantList;
}

QStringList DocumentItem::bookmarks() const
{
    QStringList list;
    const KBookmark::List bMarks = m_document->bookmarkManager()->bookmarks();
    for (const KBookmark &bookmark : bMarks) {
        list << bookmark.url().toString();
    }
    return list;
}

bool DocumentItem::supportsSearching() const
{
    return m_document->supportsSearching();
}

bool DocumentItem::isSearchInProgress() const
{
    return m_searchInProgress;
}

void DocumentItem::searchText(const QString &text)
{
    if (text.isEmpty()) {
        resetSearch();
        return;
    }
    m_document->cancelSearch();
    m_document->resetSearch(PAGEVIEW_SEARCH_ID);
    m_document->searchText(PAGEVIEW_SEARCH_ID, text, true, Qt::CaseInsensitive, Okular::Document::AllDocument, true, QColor(100, 100, 200, 40));

    if (!m_searchInProgress) {
        m_searchInProgress = true;
        emit searchInProgressChanged();
    }
}

void DocumentItem::resetSearch()
{
    m_document->resetSearch(PAGEVIEW_SEARCH_ID);
    m_matchingPages.clear();
    for (uint i = 0; i < m_document->pages(); ++i) {
        m_matchingPages << (int)i;
    }
    if (m_searchInProgress) {
        m_searchInProgress = false;
        emit searchInProgressChanged();
    }

    emit matchingPagesChanged();
}

Okular::Document *DocumentItem::document()
{
    return m_document;
}

Observer *DocumentItem::thumbnailObserver()
{
    if (!m_thumbnailObserver)
        m_thumbnailObserver = new Observer(this);

    return m_thumbnailObserver;
}

Observer *DocumentItem::pageviewObserver()
{
    if (!m_pageviewObserver) {
        m_pageviewObserver = new Observer(this);
    }

    return m_pageviewObserver;
}

void DocumentItem::searchFinished(int id, Okular::Document::SearchStatus endStatus)
{
    Q_UNUSED(endStatus)

    if (id != PAGEVIEW_SEARCH_ID) {
        return;
    }

    m_matchingPages.clear();
    for (uint i = 0; i < m_document->pages(); ++i) {
        if (m_document->page(i)->hasHighlights(id)) {
            m_matchingPages << (int)i;
        }
    }

    if (m_searchInProgress) {
        m_searchInProgress = false;
        emit searchInProgressChanged();
    }
    emit matchingPagesChanged();
}

// Observer

Observer::Observer(DocumentItem *parent)
    : QObject(parent)
    , m_document(parent)
{
    parent->document()->addObserver(this);
}

Observer::~Observer()
{
}

void Observer::notifyPageChanged(int page, int flags)
{
    emit pageChanged(page, flags);
}
