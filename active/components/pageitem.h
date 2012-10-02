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

#ifndef QPAGEITEM_H
#define QPAGEITEM_H

#include <QDeclarativeItem>

#include <core/view.h>

class QTimer;

class DocumentItem;

namespace Okular {
    class Document;
    class Page;
}

class PageItem : public QDeclarativeItem, public Okular::View
{
    Q_OBJECT

    /**
     * The document this page belongs to
     */
    Q_PROPERTY(DocumentItem *document READ document WRITE setDocument NOTIFY documentChanged)

    /**
     * The currently displayed page
     */
    Q_PROPERTY(int pageNumber READ pageNumber WRITE setPageNumber NOTIFY pageNumberChanged)

    /**
     * If true, the page will be rendered with antialias
     */
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth)

    /**
     * "Natural" width of the page
     */
    Q_PROPERTY(int implicitWidth READ implicitWidth NOTIFY implicitWidthChanged)

    /**
     * "Natural" height of the page
     */
    Q_PROPERTY(int implicitHeight READ implicitHeight NOTIFY implicitHeightChanged)

    /**
     * True if the page contains a bookmark
     */
    Q_PROPERTY(bool bookmarked READ isBookmarked WRITE setBookmarked NOTIFY bookmarkedChanged)

public:

    PageItem(QDeclarativeItem *parent=0);
    ~PageItem();


    int implicitWidth() const;
    int implicitHeight() const;

    DocumentItem *document() const;
    void setDocument(DocumentItem *doc);

    int pageNumber() const;
    void setPageNumber(int number);

    bool smooth() const;
    void setSmooth(bool smooth);

    bool isBookmarked();
    void setBookmarked(bool bookmarked);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    // inherited from View
    uint viewId() const { return m_observerId; }

    void geometryChanged(const QRectF &newGeometry,
                         const QRectF &oldGeometry);

private Q_SLOTS:
    void delayedRedraw();
    void pageHasChanged(int page, int flags);
    void checkBookmarksChanged();

Q_SIGNALS:
    void implicitWidthChanged();
    void implicitHeightChanged();
    void documentChanged();
    void pageNumberChanged();
    void bookmarkedChanged();

private:
    const Okular::Page *m_page;
    int m_pageNumber;
    bool m_smooth;
    bool m_intentionalDraw;
    bool m_bookmarked;
    QWeakPointer<DocumentItem> m_documentItem;
    QTimer *m_redrawTimer;
    int m_observerId;
    friend class ThumbnailItem;
};

#endif
