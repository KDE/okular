/*
 *   Copyright 2012 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef QPAGEITEM_H
#define QPAGEITEM_H

#include <QImage>
#include <QPointer>
#include <QQuickItem>

#include <core/document.h>
#include <core/view.h>

class QTimer;

class DocumentItem;

namespace Okular
{
class Document;
class Page;
}

class PageItem : public QQuickItem, public Okular::View
{
    Q_OBJECT

    /**
     * If this page is in a Flickable, assign it in this property, to make goToBookmark work
     */
    Q_PROPERTY(QQuickItem *flickable READ flickable WRITE setFlickable NOTIFY flickableChanged)

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
     * True if the page contains at least a bookmark.
     * Writing true to tis property idds a bookmark at the beginning of the page (if needed).
     * Writing false, all bookmarks for this page will be removed
     */
    Q_PROPERTY(bool bookmarked READ isBookmarked WRITE setBookmarked NOTIFY bookmarkedChanged)

    /**
     * list of bookmarks urls valid on this page
     */
    Q_PROPERTY(QStringList bookmarks READ bookmarks NOTIFY bookmarksChanged)

public:
    explicit PageItem(QQuickItem *parent = nullptr);
    ~PageItem() override;

    void setFlickable(QQuickItem *flickable);
    QQuickItem *flickable() const;

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

    QStringList bookmarks() const;
    void requestPixmap();

    /**
     * loads a page bookmark and tries to ensure the bookmarked position is visible
     * @param bookmark Url for the bookmark
     */
    Q_INVOKABLE void goToBookmark(const QString &bookmark);

    /**
     * Returns the position in the page for a bookmark
     * QPointF(-1,-1) if doesn't belong to this page
     *
     * @param bookmark Url for the bookmark
     */
    Q_INVOKABLE QPointF bookmarkPosition(const QString &bookmark) const;

    /**
     * Add a new bookmark ar a given position of the current page
     */
    Q_INVOKABLE void setBookmarkAtPos(qreal x, qreal y);

    /**
     * Remove a bookmark ar a given position of the current page (if present)
     */
    Q_INVOKABLE void removeBookmarkAtPos(qreal x, qreal y);

    /**
     * Remove a bookmark at a given position, if any
     */
    Q_INVOKABLE void removeBookmark(const QString &bookmark);

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QSGNode *updatePaintNode(QSGNode *, QQuickItem::UpdatePaintNodeData *) override;

Q_SIGNALS:
    void flickableChanged();
    void implicitWidthChanged();
    void implicitHeightChanged();
    void documentChanged();
    void pageNumberChanged();
    void bookmarkedChanged();
    void bookmarksChanged();

protected:
    void setIsThumbnail(bool thumbnail);

private Q_SLOTS:
    void pageHasChanged(int page, int flags);
    void checkBookmarksChanged();
    void contentXChanged();
    void contentYChanged();

private:
    void paint();
    void refreshPage();

    const Okular::Page *m_page;
    bool m_smooth;
    bool m_bookmarked;
    bool m_isThumbnail;
    QPointer<DocumentItem> m_documentItem;
    QTimer *m_redrawTimer;
    QPointer<QQuickItem> m_flickable;
    Okular::DocumentViewport m_viewPort;
    QImage m_buffer;
};

#endif
