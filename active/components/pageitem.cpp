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

#include "pageitem.h"
#include "documentitem.h"

#include <QPainter>
#include <QTimer>
#include <QStyleOptionGraphicsItem>

#include <core/bookmarkmanager.h>
#include <core/document.h>
#include <core/generator.h>
#include <core/page.h>

#include "ui/pagepainter.h"
#include "ui/priorities.h"
#include "settings.h"

#define REDRAW_TIMEOUT 250

PageItem::PageItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent),
      Okular::View( QString::fromLatin1( "PageView" ) ),
      m_page(0),
      m_smooth(false),
      m_intentionalDraw(false),
      m_bookmarked(false),
      m_isThumbnail(false)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);

    m_viewPort.rePos.enabled = true;

    m_redrawTimer = new QTimer(this);
    m_redrawTimer->setInterval(REDRAW_TIMEOUT);
    m_redrawTimer->setSingleShot(true);
    connect(m_redrawTimer, SIGNAL(timeout()), this, SLOT(delayedRedraw()));
}


PageItem::~PageItem()
{
}

void PageItem::setFlickable(QDeclarativeItem *flickable)
{
    if (m_flickable.data() == flickable) {
        return;
    }

    //check the object can act as a flickable
    if (!flickable->property("contentX").isValid() ||
        !flickable->property("contentY").isValid()) {
        return;
    }

    if (m_flickable) {
        disconnect(m_flickable.data(), 0, this, 0);
    }

    //check the object can act as a flickable
    if (!flickable->property("contentX").isValid() ||
        !flickable->property("contentY").isValid()) {
        m_flickable.clear();
        return;
    }

    m_flickable = flickable;

    if (flickable) {
        connect(flickable, SIGNAL(contentXChanged()), this, SLOT(contentXChanged()));
        connect(flickable, SIGNAL(contentYChanged()), this, SLOT(contentYChanged()));
    }

    emit flickableChanged();
}

QDeclarativeItem *PageItem::flickable() const
{
    return m_flickable.data();
}

DocumentItem *PageItem::document() const
{
    return m_documentItem.data();
}

void PageItem::setDocument(DocumentItem *doc)
{
    if (doc == m_documentItem.data() || !doc) {
        return;
    }

    m_page = 0;
    disconnect(doc, 0, this, 0);
    m_documentItem = doc;
    Observer *observer = m_isThumbnail ? m_documentItem.data()->thumbnailObserver() : m_documentItem.data()->pageviewObserver();
    connect(observer, SIGNAL(pageChanged(int, int)), this, SLOT(pageHasChanged(int, int)));
    connect(doc->document()->bookmarkManager(), SIGNAL(bookmarksChanged(KUrl)),
            this, SLOT(checkBookmarksChanged()));
    setPageNumber(0);
    emit documentChanged();
    m_redrawTimer->start();

    connect(doc, SIGNAL(pathChanged()), this, SLOT(documentPathChanged()));
}

int PageItem::pageNumber() const
{
    return m_viewPort.pageNumber;
}

void PageItem::setPageNumber(int number)
{
    if ((m_page && m_viewPort.pageNumber == number) ||
        !m_documentItem ||
        !m_documentItem.data()->isOpened() ||
        number < 0 ||
        (uint)number >= m_documentItem.data()->document()->pages()) {
        return;
    }

    m_viewPort.pageNumber = number;
    m_page = m_documentItem.data()->document()->page(number);

    emit pageNumberChanged();
    emit implicitWidthChanged();
    emit implicitHeightChanged();
    checkBookmarksChanged();
    m_redrawTimer->start();
}

int PageItem::implicitWidth() const
{
    if (m_page) {
        return m_page->width();
    }
    return 0;
}

int PageItem::implicitHeight() const
{
    if (m_page) {
        return m_page->height();
    }
    return 0;
}

void PageItem::setSmooth(const bool smooth)
{
    if (smooth == m_smooth) {
        return;
    }
    m_smooth = smooth;
    update();
}

bool PageItem::smooth() const
{
    return m_smooth;
}

bool PageItem::isBookmarked()
{
    return m_bookmarked;
}

void PageItem::setBookmarked(bool bookmarked)
{
    if (!m_documentItem) {
        return;
    }

    if (bookmarked == m_bookmarked) {
        return;
    }

    if (bookmarked) {
        m_documentItem.data()->document()->bookmarkManager()->addBookmark(m_viewPort);
    } else {
        m_documentItem.data()->document()->bookmarkManager()->removeBookmark(m_viewPort.pageNumber);
    }
    m_bookmarked = bookmarked;
    emit bookmarkedChanged();
}

QStringList PageItem::bookmarks() const
{
    QStringList list;
    foreach(const KBookmark &bookmark, m_documentItem.data()->document()->bookmarkManager()->bookmarks(m_viewPort.pageNumber)) {
        list << bookmark.url().prettyUrl();
    }
    return list;
}

void PageItem::goToBookmark(const QString &bookmark)
{
    Okular::DocumentViewport viewPort(KUrl(bookmark).htmlRef());
    setPageNumber(viewPort.pageNumber);

    //Are we in a flickable?
    if (m_flickable) {
        //normalizedX is a proportion, so contentX will be the difference between document and viewport times normalizedX
        m_flickable.data()->setProperty("contentX", qMax((qreal)0, width() - m_flickable.data()->width()) * viewPort.rePos.normalizedX);

        m_flickable.data()->setProperty("contentY", qMax((qreal)0, height() - m_flickable.data()->height()) * viewPort.rePos.normalizedY);
    }
}

QPointF PageItem::bookmarkPosition(const QString &bookmark) const
{
    Okular::DocumentViewport viewPort(KUrl(bookmark).htmlRef());

    if (viewPort.pageNumber != m_viewPort.pageNumber) {
        return QPointF(-1, -1);
    }

    return QPointF(qMax((qreal)0, width() - m_flickable.data()->width()) * viewPort.rePos.normalizedX,
                   qMax((qreal)0, height() - m_flickable.data()->height()) * viewPort.rePos.normalizedY);
}

void PageItem::setBookmarkAtPos(qreal x, qreal y)
{
    Okular::DocumentViewport viewPort(m_viewPort);
    viewPort.rePos.normalizedX = x;
    viewPort.rePos.normalizedY = y;

    m_documentItem.data()->document()->bookmarkManager()->addBookmark(viewPort);

    if (!m_bookmarked) {
        m_bookmarked = true;
        emit bookmarkedChanged();
    }

    emit bookmarksChanged();
}

void PageItem::removeBookmarkAtPos(qreal x, qreal y)
{
    Okular::DocumentViewport viewPort(m_viewPort);
    viewPort.rePos.enabled = true;
    viewPort.rePos.normalizedX = x;
    viewPort.rePos.normalizedY = y;

    m_documentItem.data()->document()->bookmarkManager()->addBookmark(viewPort);

    if (m_bookmarked && m_documentItem.data()->document()->bookmarkManager()->bookmarks(m_viewPort.pageNumber).count() == 0) {
        m_bookmarked = false;
        emit bookmarkedChanged();
    }

    emit bookmarksChanged();
}

void PageItem::removeBookmark(const QString &bookmark)
{
    m_documentItem.data()->document()->bookmarkManager()->removeBookmark(bookmark);
    emit bookmarksChanged();
}

//Reimplemented
void PageItem::geometryChanged(const QRectF &newGeometry,
                               const QRectF &oldGeometry)
{
    if (newGeometry.size().isEmpty()) {
        return;
    }

    if (newGeometry.size() != oldGeometry.size()) {
        m_redrawTimer->start();
    }

    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);
    //Why aren't they automatically emuitted?
    emit widthChanged();
    emit heightChanged();
}

void PageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if (!m_documentItem || !m_page) {
        QDeclarativeItem::paint(painter, option, widget);
        return;
    }

    const bool setAA = m_smooth && !(painter->renderHints() & QPainter::Antialiasing);
    if (setAA) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
    }

    Observer *observer = m_isThumbnail ? m_documentItem.data()->thumbnailObserver() : m_documentItem.data()->pageviewObserver();
    const int priority = m_isThumbnail ? THUMBNAILS_PRIO : PAGEVIEW_PRIO;

    if (m_intentionalDraw) {
        QLinkedList<Okular::PixmapRequest *> requestedPixmaps;
        requestedPixmaps.push_back(new Okular::PixmapRequest(observer, m_viewPort.pageNumber, width(), height(), priority, Okular::PixmapRequest::Asynchronous));
        const Okular::Document::PixmapRequestFlag prf = m_isThumbnail ? Okular::Document::NoOption : Okular::Document::RemoveAllPrevious;
        m_documentItem.data()->document()->requestPixmaps(requestedPixmaps, prf);
        m_intentionalDraw = false;
    }
    const int flags = PagePainter::Accessibility | PagePainter::Highlights | PagePainter::Annotations;
    PagePainter::paintPageOnPainter(painter, m_page, observer, flags, width(), height(), option->exposedRect.toRect());

    if (setAA) {
        painter->restore();
    }
}

//Protected slots
void PageItem::delayedRedraw()
{
    if (m_documentItem && m_page) {
        m_intentionalDraw = true;
        update();
    }
}

void PageItem::pageHasChanged(int page, int flags)
{
    if (m_viewPort.pageNumber == page) {
        if (flags == 32) {
            // skip bounding box updates
            //kDebug() << "32" << m_page->boundingBox();
        } else if (flags == Okular::DocumentObserver::Pixmap) {
            // if pixmaps have updated, just repaint .. don't bother updating pixmaps AGAIN
            update();
        } else {
            m_redrawTimer->start();
        }
    }
}

void PageItem::checkBookmarksChanged()
{
    if (!m_documentItem) {
        return;
    }

    bool newBookmarked = m_documentItem.data()->document()->bookmarkManager()->isBookmarked(m_viewPort.pageNumber);
    if (m_bookmarked != newBookmarked) {
        m_bookmarked = newBookmarked;
        emit bookmarkedChanged();
    }

    //TODO: check the page
    emit bookmarksChanged();
}

void PageItem::contentXChanged()
{
    if (!m_flickable || !m_flickable.data()->property("contentX").isValid()) {
        return;
    }

    m_viewPort.rePos.normalizedX = m_flickable.data()->property("contentX").toReal() / (width() - m_flickable.data()->width());
}

void PageItem::contentYChanged()
{
    if (!m_flickable || !m_flickable.data()->property("contentY").isValid()) {
        return;
    }

    m_viewPort.rePos.normalizedY = m_flickable.data()->property("contentY").toReal() / (height() - m_flickable.data()->height());
}

void PageItem::documentPathChanged()
{
    m_page = 0;
    setPageNumber(0);

    m_redrawTimer->start();
}


void PageItem::setIsThumbnail(bool thumbnail)
{
    if (thumbnail == m_isThumbnail) {
        return;
    }

    m_isThumbnail = thumbnail;

    if (thumbnail) {
        m_smooth = false;
    }

    /*
    m_redrawTimer->setInterval(thumbnail ? 0 : REDRAW_TIMEOUT);
    m_redrawTimer->setSingleShot(true);
    */
}

#include "pageitem.moc"
