/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef QDOCUMENTITEM_H
#define QDOCUMENTITEM_H

#include <QObject>

#include "settings.h"

#include <core/document.h>
#include <core/observer.h>

namespace Okular
{
class Document;
}

class Observer;
class TOCModel;

class DocumentItem : public QObject
{
    Q_OBJECT

    /**
     * Absolute URI to document file to open
     */
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)

    /**
     * Suggested window title if a window represents this document. may be pathname or document title, depending on Okular settings.
     */
    Q_PROPERTY(QString windowTitleForDocument READ windowTitleForDocument NOTIFY windowTitleForDocumentChanged)

    /**
     * Current displaying page for the document
     */
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)

    /**
     * True if this DocumentItem instance has a document file opened
     */
    Q_PROPERTY(bool opened READ isOpened NOTIFY openedChanged)

    /**
     * How many pages there are in the document
     */
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)

    /**
     * True if the document is able to perform full text searches in its contents
     */
    Q_PROPERTY(bool supportsSearching READ supportsSearching NOTIFY supportsSearchingChanged)

    /**
     * True if a search is currently in progress and results didn't arrive yet
     */
    Q_PROPERTY(bool searchInProgress READ isSearchInProgress NOTIFY searchInProgressChanged)

    /**
     * A list of all pages that contain a match for the search terms. If no text has been searched, all pages are returned.
     */
    Q_PROPERTY(QVariantList matchingPages READ matchingPages NOTIFY matchingPagesChanged)

    /**
     * Table of contents for the document, if available
     */
    Q_PROPERTY(TOCModel *tableOfContents READ tableOfContents CONSTANT)

    /**
     * List of pages that contain a bookmark
     */
    Q_PROPERTY(QVariantList bookmarkedPages READ bookmarkedPages NOTIFY bookmarkedPagesChanged)

    /**
     * list of bookmarks urls valid on this page
     */
    Q_PROPERTY(QStringList bookmarks READ bookmarks NOTIFY bookmarksChanged)

public:
    explicit DocumentItem(QObject *parent = nullptr);
    ~DocumentItem() override;

    void setUrl(const QUrl &url);
    QUrl url() const;

    QString windowTitleForDocument() const;

    void setCurrentPage(int page);
    int currentPage() const;

    bool isOpened() const;

    int pageCount() const;

    bool supportsSearching() const;

    bool isSearchInProgress() const;

    QVariantList matchingPages() const;

    TOCModel *tableOfContents() const;

    QVariantList bookmarkedPages() const;

    QStringList bookmarks() const;

    // This could be a property, but maybe we want to have parameter for searchText
    /**
     * Performs a search in the document
     *
     * @param text the string to search in the document
     */
    Q_INVOKABLE void searchText(const QString &text);

    /**
     * Reset the search over the document.
     */
    Q_INVOKABLE void resetSearch();

    // Internal, not binded to qml
    Okular::Document *document();
    Observer *pageviewObserver();
    Observer *thumbnailObserver();

Q_SIGNALS:
    void urlChanged();
    void pageCountChanged();
    void openedChanged();
    void searchInProgressChanged();
    void matchingPagesChanged();
    void currentPageChanged();
    void supportsSearchingChanged();
    void bookmarkedPagesChanged();
    void bookmarksChanged();
    void windowTitleForDocumentChanged();

private Q_SLOTS:
    void searchFinished(int id, Okular::Document::SearchStatus endStatus);

private:
    Okular::Document *m_document;
    TOCModel *m_tocModel;
    Observer *m_thumbnailObserver;
    Observer *m_pageviewObserver;
    QVariantList m_matchingPages;
    bool m_searchInProgress;
};

class Observer : public QObject, public Okular::DocumentObserver
{
    Q_OBJECT

public:
    explicit Observer(DocumentItem *parent);
    ~Observer() override;

    // inherited from DocumentObserver
    void notifyPageChanged(int page, int flags) override;

Q_SIGNALS:
    void pageChanged(int page, int flags);

private:
    DocumentItem *m_document;
};

#endif
