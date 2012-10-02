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
#include "settings.h"

#define REDRAW_TIMEOUT 250

PageItem::PageItem(QDeclarativeItem *parent)
    : QDeclarativeItem(parent),
      Okular::View( QString::fromLatin1( "PageView" ) ),
      m_page(0),
      m_pageNumber(0),
      m_smooth(false),
      m_intentionalDraw(true),
      m_bookmarked(false)
{
    m_observerId = PAGEVIEW_ID;
    setFlag(QGraphicsItem::ItemHasNoContents, false);

    m_redrawTimer = new QTimer(this);
    m_redrawTimer->setSingleShot(true);
    connect(m_redrawTimer, SIGNAL(timeout()), this, SLOT(delayedRedraw()));
}


PageItem::~PageItem()
{
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
    Observer *observer = m_documentItem.data()->observerFor(m_observerId);
    connect(observer, SIGNAL(pageChanged(int, int)), this, SLOT(pageHasChanged(int, int)));
    connect(doc->document()->bookmarkManager(), SIGNAL(bookmarksChanged(KUrl)),
            this, SLOT(checkBookmarksChanged()));
    setPageNumber(0);
    emit documentChanged();
    m_redrawTimer->start(REDRAW_TIMEOUT);
}

int PageItem::pageNumber() const
{
    return m_pageNumber;
}

void PageItem::setPageNumber(int number)
{
    if ((m_page && m_pageNumber == number) ||
        !m_documentItem ||
        !m_documentItem.data()->isOpened() ||
        number < 0 ||
        (uint)number > m_documentItem.data()->document()->pages()) {
        return;
    }

    m_pageNumber = number;
    m_page = m_documentItem.data()->document()->page(number);

    emit pageNumberChanged();
    emit implicitWidthChanged();
    emit implicitHeightChanged();
    checkBookmarksChanged();
    m_redrawTimer->start(REDRAW_TIMEOUT);
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
        m_documentItem.data()->document()->bookmarkManager()->addBookmark(m_pageNumber);
    } else {
        m_documentItem.data()->document()->bookmarkManager()->removeBookmark(m_pageNumber);
    }
    m_bookmarked = bookmarked;
    emit bookmarkedChanged();
}

//Reimplemented
void PageItem::geometryChanged(const QRectF &newGeometry,
                               const QRectF &oldGeometry)
{
    if (newGeometry.size().isEmpty()) {
        return;
    }

    if (newGeometry.size() != oldGeometry.size()) {
        m_redrawTimer->start(REDRAW_TIMEOUT);
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

    const int priority = m_observerId == PAGEVIEW_ID ? PAGEVIEW_PRELOAD_PRIO : THUMBNAILS_PRELOAD_PRIO;

    if (m_intentionalDraw) {
        QLinkedList< Okular::PixmapRequest * > requestedPixmaps;
        requestedPixmaps.push_back(new Okular::PixmapRequest(m_observerId, m_pageNumber, width(), height(), priority, true));
        m_documentItem.data()->document()->requestPixmaps( requestedPixmaps );
    }
    m_intentionalDraw = false;
    const int flags = PagePainter::Accessibility | PagePainter::Highlights | PagePainter::Annotations;
    PagePainter::paintPageOnPainter(painter, m_page, m_observerId, flags, width(), height(), option->exposedRect.toRect());

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

void PageItem::pageHasChanged( int page, int flags )
{
    Q_UNUSED(flags)
    if (m_pageNumber == page) {
        m_redrawTimer->start(REDRAW_TIMEOUT);
    }
}

void PageItem::checkBookmarksChanged()
{
    if (!m_documentItem) {
        return;
    }

    bool newBookmarked = m_documentItem.data()->document()->bookmarkManager()->isBookmarked(m_pageNumber);
    if (m_bookmarked != newBookmarked) {
        m_bookmarked = newBookmarked;
        emit bookmarkedChanged();
    }
}

#include "pageitem.moc"
