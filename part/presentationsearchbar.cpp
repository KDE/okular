/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "presentationsearchbar.h"

#include <qevent.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qstylepainter.h>
#include <qtoolbutton.h>

#include <KLocalizedString>
#include <QIcon>

#include "searchlineedit.h"

#define SNAP_DELTA 15

class HandleDrag : public QWidget
{
    Q_OBJECT

public:
    explicit HandleDrag(QWidget *parent = Q_NULLPTR)
        : QWidget(parent)
    {
        setCursor(Qt::SizeAllCursor);
        setFixedWidth(style()->pixelMetric(QStyle::PM_ToolBarHandleExtent));
    }

    void paintEvent(QPaintEvent *) override
    {
        QStyleOption opt;
        opt.initFrom(this);
        opt.state |= QStyle::State_Horizontal;
        QStylePainter p(this);
        p.drawPrimitive(QStyle::PE_IndicatorToolBarHandle, opt);
    }
};

PresentationSearchBar::PresentationSearchBar(Okular::Document *document, QWidget *anchor, QWidget *parent)
    : QWidget(parent)
    , m_anchor(anchor)
    , m_snapped(true)
{
    setAutoFillBackground(true);

    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_handle = new HandleDrag(this);
    m_handle->installEventFilter(this);
    lay->addWidget(m_handle);

    QToolButton *closeBtn = new QToolButton(this);
    closeBtn->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    closeBtn->setIconSize(QSize(24, 24));
    closeBtn->setToolTip(i18n("Close"));
    closeBtn->setAutoRaise(true);
    lay->addWidget(closeBtn);

    m_search = new SearchLineEdit(this, document);
    m_search->setClearButtonEnabled(true);
    m_search->setSearchCaseSensitivity(Qt::CaseInsensitive);
    m_search->setSearchMinimumLength(0);
    m_search->setSearchType(Okular::Document::NextMatch);
    m_search->setSearchId(PRESENTATION_SEARCH_ID);
    m_search->setSearchColor(qRgb(255, 255, 64));
    m_search->setSearchMoveViewport(true);
    lay->addWidget(m_search);

    QPushButton *findNextBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("go-down-search")), i18n("Find Next"), this);
    lay->addWidget(findNextBtn);

    m_anchor->installEventFilter(this);

    connect(closeBtn, &QAbstractButton::clicked, this, &QWidget::close);
    connect(findNextBtn, &QPushButton::clicked, m_search, &SearchLineEdit::findNext);
}

PresentationSearchBar::~PresentationSearchBar()
{
}

void PresentationSearchBar::forceSnap()
{
    m_point = QPoint(m_anchor->width() / 2, m_anchor->height());
    m_snapped = true;
    move(m_point.x() - width() / 2, m_point.y() - height());
}

void PresentationSearchBar::focusOnSearchEdit()
{
    m_search->setFocus();
}

void PresentationSearchBar::resizeEvent(QResizeEvent *)
{
    // if in snap mode, then force the snap and place ourselves correctly again
    if (m_snapped) {
        forceSnap();
    }
}

bool PresentationSearchBar::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_handle && (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease || e->type() == QEvent::MouseMove)) {
        QMouseEvent *me = (QMouseEvent *)e;
        if (e->type() == QEvent::MouseButtonPress) {
            m_drag = m_handle->mapTo(this, me->pos());
        } else if (e->type() == QEvent::MouseButtonRelease) {
            m_drag = QPoint();
        } else if (e->type() == QEvent::MouseMove) {
            QPoint snapdelta(width() / 2, height());
            QPoint delta = m_handle->mapTo(this, me->pos()) - m_drag;
            QPoint newpostemp = pos() + delta;
            QPoint tmp = newpostemp + snapdelta - m_point;
            QPoint newpos = abs(tmp.x()) < SNAP_DELTA && abs(tmp.y()) < SNAP_DELTA ? m_point - snapdelta : newpostemp;
            m_snapped = newpos == (m_point - snapdelta);
            move(newpos);
        }
        return true;
    }
    if (obj == m_anchor && e->type() == QEvent::Resize) {
        m_point = QPoint(m_anchor->width() / 2, m_anchor->height());

        if (m_snapped) {
            move(m_point.x() - width() / 2, m_point.y() - height());
        }
    }

    return false;
}

#include "presentationsearchbar.moc"
