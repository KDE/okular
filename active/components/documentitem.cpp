/*
 *   Copyright 2012 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "documentitem.h"

#include <okular/core/page.h>


DocumentItem::DocumentItem(QObject *parent)
    : QObject(parent),
      m_searchInProgress(false)
{
    Okular::Settings::instance("okularproviderrc");
    m_document = new Okular::Document(0);
    connect(m_document, SIGNAL(searchFinished(int,Okular::Document::SearchStatus)),
            this, SLOT(searchFinished(int,Okular::Document::SearchStatus)));
}


DocumentItem::~DocumentItem()
{
    delete m_document;
}

void DocumentItem::setPath(const QString &path)
{
    //TODO: remote urls
    m_document->openDocument(path, KUrl(path), KMimeType::findByUrl(KUrl(path)));

    m_matchingPages.clear();
    for (uint i = 0; i < m_document->pages(); ++i) {
         m_matchingPages << (int)i;
    }
    emit matchingPagesChanged();
    emit pathChanged();
    emit pageCountChanged();
    emit openedChanged();
    emit supportsSearchingChanged();
}

QString DocumentItem::path() const
{
    return m_document->currentDocument().prettyUrl();
}

void DocumentItem::setCurrentPage(int page)
{
    m_document->setViewportPage(page);
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

QList<int> DocumentItem::matchingPages() const
{
    return m_matchingPages;
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
    m_document->searchText(PAGEVIEW_SEARCH_ID, text, 1, Qt::CaseInsensitive,
                           Okular::Document::AllDocument, true, QColor(100,100,200,40), true);

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

Observer *DocumentItem::observerFor(int id)
{
    if (!m_observers.contains(id)) {
        m_observers[id] = new Observer(this, id);
    }

    return m_observers.value(id);
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


//Observer

Observer::Observer(DocumentItem *parent, int id)
    : QObject(parent),
      m_observerId(id),
      m_document(parent)
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

#include "documentitem.moc"
