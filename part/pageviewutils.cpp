/*
    SPDX-FileCopyrightText: 2004-2005 Enrico Ros <eros.kde@email.it>

    Work sponsored by the LiMux project of the city of Munich:
    SPDX-FileCopyrightText: 2017 Klarälvdalens Datakonsult AB a KDAB Group company <info@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pageviewutils.h"

// qt/kde includes
#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QTimer>

// local includes
#include "core/form.h"
#include "core/page.h"
#include "formwidgets.h"
#include "settings.h"
#include "videowidget.h"

/*********************/
/** PageViewItem     */
/*********************/

PageViewItem::PageViewItem(const Okular::Page *page)
    : m_page(page)
    , m_zoomFactor(1.0)
    , m_visible(true)
    , m_formsVisible(false)
    , m_crop(0., 0., 1., 1.)
{
}

PageViewItem::~PageViewItem()
{
    qDeleteAll(m_formWidgets);
    qDeleteAll(m_videoWidgets);
}

const Okular::Page *PageViewItem::page() const
{
    return m_page;
}

int PageViewItem::pageNumber() const
{
    return m_page->number();
}

const QRect &PageViewItem::croppedGeometry() const
{
    return m_croppedGeometry;
}

int PageViewItem::croppedWidth() const
{
    return m_croppedGeometry.width();
}

int PageViewItem::croppedHeight() const
{
    return m_croppedGeometry.height();
}

const QRect &PageViewItem::uncroppedGeometry() const
{
    return m_uncroppedGeometry;
}

int PageViewItem::uncroppedWidth() const
{
    return m_uncroppedGeometry.width();
}

int PageViewItem::uncroppedHeight() const
{
    return m_uncroppedGeometry.height();
}

const Okular::NormalizedRect &PageViewItem::crop() const
{
    return m_crop;
}

double PageViewItem::zoomFactor() const
{
    return m_zoomFactor;
}

double PageViewItem::absToPageX(double absX) const
{
    return (absX - m_uncroppedGeometry.left()) / m_uncroppedGeometry.width();
}

double PageViewItem::absToPageY(double absY) const
{
    return (absY - m_uncroppedGeometry.top()) / m_uncroppedGeometry.height();
}

bool PageViewItem::isVisible() const
{
    return m_visible;
}

QSet<FormWidgetIface *> &PageViewItem::formWidgets()
{
    return m_formWidgets;
}

QHash<Okular::Movie *, VideoWidget *> &PageViewItem::videoWidgets()
{
    return m_videoWidgets;
}

void PageViewItem::setWHZC(int w, int h, double z, const Okular::NormalizedRect &c)
{
    m_croppedGeometry.setWidth(w);
    m_croppedGeometry.setHeight(h);
    m_zoomFactor = z;
    m_crop = c;
    m_uncroppedGeometry.setWidth(qRound(w / (c.right - c.left)));
    m_uncroppedGeometry.setHeight(qRound(h / (c.bottom - c.top)));
    for (FormWidgetIface *fwi : qAsConst(m_formWidgets)) {
        Okular::NormalizedRect r = fwi->rect();
        fwi->setWidthHeight(qRound(fabs(r.right - r.left) * m_uncroppedGeometry.width()), qRound(fabs(r.bottom - r.top) * m_uncroppedGeometry.height()));
    }
    for (VideoWidget *vw : qAsConst(m_videoWidgets)) {
        const Okular::NormalizedRect r = vw->normGeometry();
        vw->resize(qRound(fabs(r.right - r.left) * m_uncroppedGeometry.width()), qRound(fabs(r.bottom - r.top) * m_uncroppedGeometry.height()));
    }
}

void PageViewItem::moveTo(int x, int y)
// Assumes setWHZC() has already been called
{
    m_croppedGeometry.moveLeft(x);
    m_croppedGeometry.moveTop(y);
    m_uncroppedGeometry.moveLeft(qRound(x - m_crop.left * m_uncroppedGeometry.width()));
    m_uncroppedGeometry.moveTop(qRound(y - m_crop.top * m_uncroppedGeometry.height()));
    QSet<FormWidgetIface *>::iterator it = m_formWidgets.begin(), itEnd = m_formWidgets.end();
    for (; it != itEnd; ++it) {
        Okular::NormalizedRect r = (*it)->rect();
        (*it)->moveTo(qRound(x + m_uncroppedGeometry.width() * r.left) + 1, qRound(y + m_uncroppedGeometry.height() * r.top) + 1);
    }
    for (VideoWidget *vw : qAsConst(m_videoWidgets)) {
        const Okular::NormalizedRect r = vw->normGeometry();
        vw->move(qRound(x + m_uncroppedGeometry.width() * r.left) + 1, qRound(y + m_uncroppedGeometry.height() * r.top) + 1);
    }
}

void PageViewItem::setVisible(bool visible)
{
    setFormWidgetsVisible(visible && m_formsVisible);
    m_visible = visible;
}

void PageViewItem::invalidate()
{
    m_croppedGeometry.setRect(0, 0, 0, 0);
    m_uncroppedGeometry.setRect(0, 0, 0, 0);
}

bool PageViewItem::setFormWidgetsVisible(bool visible)
{
    m_formsVisible = visible;

    if (!m_visible)
        return false;

    bool somehadfocus = false;
    QSet<FormWidgetIface *>::iterator it = m_formWidgets.begin(), itEnd = m_formWidgets.end();
    for (; it != itEnd; ++it) {
        bool hadfocus = (*it)->setVisibility(visible && (*it)->formField()->isVisible() && FormWidgetsController::shouldFormWidgetBeShown((*it)->formField()));
        somehadfocus = somehadfocus || hadfocus;
    }
    return somehadfocus;
}

void PageViewItem::reloadFormWidgetsState()
{
    for (FormWidgetIface *fwi : qAsConst(m_formWidgets)) {
        fwi->setVisibility(fwi->formField()->isVisible() && FormWidgetsController::shouldFormWidgetBeShown(fwi->formField()));
    }
}

/*********************/
/** PageViewMessage  */
/*********************/

PageViewMessage::PageViewMessage(QWidget *parent)
    : QWidget(parent)
    , m_timer(nullptr)
    , m_lineSpacing(0)
{
    setObjectName(QStringLiteral("pageViewMessage"));
    setFocusPolicy(Qt::NoFocus);
    QPalette pal = palette();
    pal.setColor(QPalette::Active, QPalette::Window, QApplication::palette().color(QPalette::Active, QPalette::Window));
    setPalette(pal);
    // if the layout is LtR, we can safely place it in the right position
    if (layoutDirection() == Qt::LeftToRight)
        move(10, 10);
    resize(0, 0);
    hide();
}

void PageViewMessage::display(const QString &message, const QString &details, Icon icon, int durationMs)
// give Caesar what belongs to Caesar: code taken from Amarok's osd.h/.cpp
// "redde (reddite, pl.) cesari quae sunt cesaris", just btw.  :)
// The code has been heavily modified since then.
{
    if (!Okular::Settings::showOSD()) {
        hide();
        return;
    }

    // set text
    m_message = message;
    m_details = details;
    // reset vars
    m_lineSpacing = 0;

    // load icon (if set)
    m_symbol = QPixmap();
    const auto symbolSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
    if (icon != None) {
        switch (icon) {
        case Annotation:
            m_symbol = QIcon::fromTheme(QStringLiteral("draw-freehand")).pixmap(symbolSize);
            break;
        case Find:
            m_symbol = QIcon::fromTheme(QStringLiteral("zoom-original")).pixmap(symbolSize);
            break;
        case Error:
            m_symbol = QIcon::fromTheme(QStringLiteral("dialog-error")).pixmap(symbolSize);
            break;
        case Warning:
            m_symbol = QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(symbolSize);
            break;
        default:
            m_symbol = QIcon::fromTheme(QStringLiteral("dialog-information")).pixmap(symbolSize);
            break;
        }
    }

    computeSizeAndResize();
    // show widget and schedule a repaint
    show();
    update();

    // close the message window after given mS
    if (durationMs > 0) {
        if (!m_timer) {
            m_timer = new QTimer(this);
            m_timer->setSingleShot(true);
            connect(m_timer, &QTimer::timeout, this, &PageViewMessage::hide);
        }
        m_timer->start(durationMs);
    } else if (m_timer)
        m_timer->stop();

    qobject_cast<QAbstractScrollArea *>(parentWidget())->viewport()->installEventFilter(this);
}

QRect PageViewMessage::computeTextRect(const QString &message, int extra_width) const
// Return the QRect which embeds the text
{
    int charSize = fontMetrics().averageCharWidth();
    /* width of the viewport, minus 20 (~ size removed by further resizing),
       minus the extra size (usually the icon width), minus (a bit empirical)
       twice the mean width of a character to ensure that the bounding box is
       really smaller than the container.
     */
    const int boundingWidth = qobject_cast<QAbstractScrollArea *>(parentWidget())->viewport()->width() - 20 - (extra_width > 0 ? 2 + extra_width : 0) - 2 * charSize;
    QRect textRect = fontMetrics().boundingRect(0, 0, boundingWidth, 0, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, message);
    textRect.translate(-textRect.left(), -textRect.top());
    textRect.adjust(0, 0, 2, 2);

    return textRect;
}

void PageViewMessage::computeSizeAndResize()
{
    // determine text rectangle
    const QRect textRect = computeTextRect(m_message, m_symbol.width());
    int width = textRect.width(), height = textRect.height();

    if (!m_details.isEmpty()) {
        // determine details text rectangle
        const QRect detailsRect = computeTextRect(m_details, m_symbol.width());
        width = qMax(width, detailsRect.width());
        height += detailsRect.height();

        // plus add a ~60% line spacing
        m_lineSpacing = static_cast<int>(fontMetrics().height() * 0.6);
        height += m_lineSpacing;
    }

    // update geometry with icon information
    if (!m_symbol.isNull()) {
        width += 2 + m_symbol.width();
        height = qMax(height, m_symbol.height());
    }

    // resize widget
    resize(QRect(0, 0, width + 10, height + 8).size());

    // if the layout is RtL, we can move it to the right place only after we
    // know how much size it will take
    if (layoutDirection() == Qt::RightToLeft)
        move(parentWidget()->width() - geometry().width() - 10 - 1, 10);
}

bool PageViewMessage::eventFilter(QObject *obj, QEvent *event)
{
    /* if the parent object (scroll area) resizes, the message should
       resize as well */
    if (event->type() == QEvent::Resize) {
        QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
        if (resizeEvent->oldSize() != resizeEvent->size()) {
            computeSizeAndResize();
        }
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
}

void PageViewMessage::paintEvent(QPaintEvent * /* e */)
{
    const QRect textRect = computeTextRect(m_message, m_symbol.width());

    QRect detailsRect;
    if (!m_details.isEmpty()) {
        detailsRect = computeTextRect(m_details, m_symbol.width());
    }

    int textXOffset = 0,
        // add 2 to account for the reduced drawRoundedRect later
        textYOffset = (geometry().height() - textRect.height() - detailsRect.height() - m_lineSpacing + 2) / 2, iconXOffset = 0, iconYOffset = !m_symbol.isNull() ? (geometry().height() - m_symbol.height()) / 2 : 0, shadowOffset = 1;

    if (layoutDirection() == Qt::RightToLeft)
        iconXOffset = 2 + textRect.width();
    else
        textXOffset = 2 + m_symbol.width();

    // draw background
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::black);
    painter.setBrush(palette().color(QPalette::Window));
    painter.translate(0.5, 0.5);
    painter.drawRoundedRect(1, 1, width() - 2, height() - 2, 1600.0 / width(), 1600.0 / height());

    // draw icon if present
    if (!m_symbol.isNull())
        painter.drawPixmap(5 + iconXOffset, iconYOffset, m_symbol, 0, 0, m_symbol.width(), m_symbol.height());

    const int xStartPoint = 5 + textXOffset;
    const int yStartPoint = textYOffset;
    const int textDrawingFlags = Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap;

    // draw shadow and text
    painter.setPen(palette().color(QPalette::Window).darker(115));
    painter.drawText(xStartPoint + shadowOffset, yStartPoint + shadowOffset, textRect.width(), textRect.height(), textDrawingFlags, m_message);
    if (!m_details.isEmpty())
        painter.drawText(xStartPoint + shadowOffset, yStartPoint + textRect.height() + m_lineSpacing + shadowOffset, textRect.width(), detailsRect.height(), textDrawingFlags, m_details);
    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(xStartPoint, yStartPoint, textRect.width(), textRect.height(), textDrawingFlags, m_message);
    if (!m_details.isEmpty())
        painter.drawText(xStartPoint + shadowOffset, yStartPoint + textRect.height() + m_lineSpacing, textRect.width(), detailsRect.height(), textDrawingFlags, m_details);
}

void PageViewMessage::mousePressEvent(QMouseEvent * /*e*/)
{
    if (m_timer)
        m_timer->stop();
    hide();
}
