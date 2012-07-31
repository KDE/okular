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

#ifndef QDOCUMENTITEM_H
#define QDOCUMENTITEM_H

#include <QObject>

#include "settings.h"

#include <core/document.h>
#include <core/observer.h>

namespace Okular {
    class Document;
}

class Observer;
class TOCModel;

class DocumentItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(bool opened READ isOpened NOTIFY openedChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(bool supportsSearching READ supportsSearching NOTIFY supportsSearchingChanged)
    Q_PROPERTY(bool searchInProgress READ isSearchInProgress NOTIFY searchInProgressChanged)
    Q_PROPERTY(QList<int> matchingPages READ matchingPages NOTIFY matchingPagesChanged)
    Q_PROPERTY(TOCModel *tableOfContents READ tableOfContents CONSTANT)

public:

    DocumentItem(QObject *parent=0);
    ~DocumentItem();

    void setPath(const QString &path);
    QString path() const;

    void setCurrentPage(int page);
    int currentPage() const;

    bool isOpened() const;

    int pageCount() const;

    bool supportsSearching() const;

    bool isSearchInProgress() const;

    QList<int> matchingPages() const;

    TOCModel *tableOfContents() const;

    //Those could be a property, but maybe we want to have parameter for searchText
    Q_INVOKABLE void searchText(const QString &text);
    Q_INVOKABLE void resetSearch();

    //Internal, not binded to qml
    Okular::Document *document();
    Observer *observerFor(int id);

Q_SIGNALS:
    void pathChanged();
    void pageCountChanged();
    void openedChanged();
    void searchInProgressChanged();
    void matchingPagesChanged();
    void currentPageChanged();
    void supportsSearchingChanged();

private Q_SLOTS:
    void searchFinished(int id, Okular::Document::SearchStatus endStatus);

private:
    Okular::Document *m_document;
    TOCModel *m_tocModel;
    QHash <int, Observer *> m_observers;
    QList<int> m_matchingPages;
    bool m_searchInProgress;
};

class Observer : public QObject, public Okular::DocumentObserver
{
    Q_OBJECT

public:
    Observer(DocumentItem *parent, int observerId);
    ~Observer();

    // inherited from DocumentObserver
    uint observerId() const { return m_observerId; }
    void notifyPageChanged(int page, int flags);

Q_SIGNALS:
    void pageChanged(int page, int flags);

private:
    int m_observerId;
    DocumentItem *m_document;
};

#endif
