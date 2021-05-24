/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

*/

#include "magnifierview.h"

#include <QPainter>

#include "core/document.h"
#include "core/generator.h"
#include "pagepainter.h"
#include "priorities.h"

static const int SCALE = 10;

MagnifierView::MagnifierView(Okular::Document *document, QWidget *parent)
    : QWidget(parent)
    , m_document(document)
    , m_page(nullptr)
{
    document->addObserver(this);
}

MagnifierView::~MagnifierView()
{
    m_document->removeObserver(this);
}

void MagnifierView::notifySetup(const QVector<Okular::Page *> &pages, int setupFlags)
{
    if (!(setupFlags & Okular::DocumentObserver::DocumentChanged)) {
        return;
    }

    m_pages = pages;
    m_page = nullptr;
    m_current = -1;
}

void MagnifierView::notifyPageChanged(int page, int flags)
{
    Q_UNUSED(page);
    Q_UNUSED(flags);

    if (isVisible()) {
        update();
    }
}

bool MagnifierView::canUnloadPixmap(int page) const
{
    return (page != m_current);
}

void MagnifierView::notifyCurrentPageChanged(int previous, int current)
{
    Q_UNUSED(previous);

    if (current != m_current) {
        m_current = current;
        m_page = m_pages[current];

        if (isVisible()) {
            requestPixmap();
            update();
        }
    }
}

void MagnifierView::updateView(const Okular::NormalizedPoint &p, const Okular::Page *page)
{
    m_viewpoint = p;

    if (page != m_page) // ok, we are screwed
    {
        m_page = page;
        m_current = page->number();
    }

    if (isVisible()) {
        requestPixmap();
        update();
    }
}

void MagnifierView::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter p(this);

    if (m_page) {
        QRect where = QRect(0, 0, width(), height());
        PagePainter::paintCroppedPageOnPainter(&p, m_page, this, 0, m_page->width() * SCALE, m_page->height() * SCALE, where, normalizedView(), nullptr);
    }

    drawTicks(&p);
}

void MagnifierView::move(int x, int y)
{
    QWidget::move(x, y);
    requestPixmap();
}

void MagnifierView::requestPixmap()
{
    const int full_width = m_page->width() * SCALE;
    const int full_height = m_page->height() * SCALE;

    Okular::NormalizedRect nrect = normalizedView();

    if (m_page && !m_page->hasPixmap(this, full_width, full_height, nrect)) {
        QLinkedList<Okular::PixmapRequest *> requestedPixmaps;

        Okular::PixmapRequest *p = new Okular::PixmapRequest(this, m_current, full_width, full_height, devicePixelRatioF(), PAGEVIEW_PRIO, Okular::PixmapRequest::Asynchronous);

        if (m_page->hasTilesManager(this)) {
            p->setTile(true);
        }

        // request a little bit bigger rectangle then currently viewed, but not the full scale page
        const double rect_width = (nrect.right - nrect.left) * 0.5, rect_height = (nrect.bottom - nrect.top) * 0.5;

        const double top = qMax(nrect.top - rect_height, 0.0);
        const double bottom = qMin(nrect.bottom + rect_height, 1.0);
        const double left = qMax(nrect.left - rect_width, 0.0);
        const double right = qMin(nrect.right + rect_width, 1.0);

        p->setNormalizedRect(Okular::NormalizedRect(left, top, right, bottom));
        requestedPixmaps.push_back(p);

        m_document->requestPixmaps(requestedPixmaps);
    }
}

Okular::NormalizedRect MagnifierView::normalizedView() const
{
    double h = (double)height() / (SCALE * m_page->height() * 2);
    double w = (double)width() / (SCALE * m_page->width() * 2);
    return Okular::NormalizedRect(m_viewpoint.x - w, m_viewpoint.y - h, m_viewpoint.x + w, m_viewpoint.y + h);
}

void MagnifierView::drawTicks(QPainter *p)
{
    p->save();

    p->setPen(QPen(Qt::black, 0));
    // the cross
    p->drawLine(width() / 2, 0, width() / 2, height() - 1);
    p->drawLine(0, height() / 2, width() - 1, height() / 2);

    // the borders
    p->drawLine(0, 0, width() - 1, 0);
    p->drawLine(width() - 1, 0, width() - 1, height() - 1);
    p->drawLine(0, height() - 1, width() - 1, height() - 1);
    p->drawLine(0, height() - 1, 0, 0);

    // ticks
    // TODO possibility to switch units (pt, mm, cc, in, printing dots)
    float ps = (float)SCALE * 5; // how much pixels in widget is one pixel in document * how often
    int tw = 10;                 // tick size in pixels

    for (float x = 0; x < width(); x += ps) {
        p->drawLine(x, 1, x, tw);
        p->drawLine(x, height() - 1, x, height() - tw - 1);
        p->drawLine(1, x, tw, x);
        p->drawLine(width() - 1, x, width() - tw - 1, x);
    }

    p->restore();
}

#include "moc_magnifierview.cpp"
