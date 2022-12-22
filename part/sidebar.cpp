/*
    SPDX-FileCopyrightText: 2007 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2009 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sidebar.h"

#include <qaction.h>
#include <qapplication.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlist.h>
#include <qmimedata.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <qsplitter.h>
#include <qtabwidget.h>

#include <KLocalizedString>
#include <kcolorscheme.h>
#include <kiconloader.h>
#include <kurlmimedata.h>
#include <qmenu.h>

#include "settings.h"

/* Private storage. */
class Sidebar::Private
{
public:
    Private()
        : sideWidget(nullptr)
        , bottomWidget(nullptr)
        , splitterSizesSet(false)
    {
    }

    QSplitter *splitter;
    QTabWidget *viewChooserTabs;
    QWidget *sideContainer;
    QVBoxLayout *vlay;
    QWidget *sideWidget;
    QWidget *bottomWidget;
    bool splitterSizesSet;
};

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
    , d(new Private)
{
    QHBoxLayout *mainlay = new QHBoxLayout(this);
    mainlay->setContentsMargins(0, 0, 0, 0);
    mainlay->setSpacing(0);

    setAutoFillBackground(true);
    setAcceptDrops(true);

    d->splitter = new QSplitter(this);
    mainlay->addWidget(d->splitter);
    d->splitter->setOpaqueResize(true);
    d->splitter->setChildrenCollapsible(false);

    // d->sideContainer holds all the actual content
    d->sideContainer = new QWidget(d->splitter);
    d->sideContainer->setMinimumWidth(90);
    d->sideContainer->setMaximumWidth(600);
    d->vlay = new QVBoxLayout(d->sideContainer);
    d->vlay->setContentsMargins(0, 0, 0, 0);

    d->viewChooserTabs = new QTabWidget(d->sideContainer);
    d->viewChooserTabs->setDocumentMode(true);
    d->vlay->addWidget(d->viewChooserTabs);

    connect(d->splitter, &QSplitter::splitterMoved, this, &Sidebar::splitterMoved);
}

Sidebar::~Sidebar()
{
    delete d;
}

int Sidebar::addItem(QWidget *widget, const QIcon &icon, const QString &text)
{
    if (!widget) {
        return -1;
    }

    widget->setParent(d->viewChooserTabs);
    d->viewChooserTabs->addTab(widget, icon, text);
    const int thisTabIndex = d->viewChooserTabs->count() - 1;
    // Hide all text and use large icons
    d->viewChooserTabs->setTabText(thisTabIndex, QString());
    d->viewChooserTabs->setIconSize(QSize(22, 22));
    d->viewChooserTabs->setTabToolTip(thisTabIndex, text);
    return thisTabIndex;
}

void Sidebar::setMainWidget(QWidget *widget)
{
    delete d->sideWidget;
    d->sideWidget = widget;
    if (d->sideWidget) {
        // setting the splitter as parent for the widget automatically plugs it
        // into the splitter, neat!
        d->sideWidget->setParent(d->splitter);
        setFocusProxy(widget);

        if (!d->splitterSizesSet) {
            QList<int> splitterSizes = Okular::Settings::splitterSizes();
            if (splitterSizes.isEmpty()) {
                // the first time use 1/10 for the panel and 9/10 for the pageView
                splitterSizes.push_back(50);
                splitterSizes.push_back(500);
            }
            d->splitter->setSizes(splitterSizes);
            d->splitterSizesSet = true;
        }
    }
}

void Sidebar::setBottomWidget(QWidget *widget)
{
    delete d->bottomWidget;
    d->bottomWidget = widget;
    if (d->bottomWidget) {
        d->bottomWidget->setParent(this);
        d->vlay->addWidget(d->bottomWidget);
    }
}

void Sidebar::setCurrentItem(QWidget *widget)
{
    d->viewChooserTabs->setCurrentWidget(widget);
}

QWidget *Sidebar::currentItem() const
{
    if (d->viewChooserTabs->currentIndex() == -1) {
        return nullptr;
    }

    return d->viewChooserTabs->currentWidget();
}

void Sidebar::setSidebarVisibility(bool visible)
{
    d->sideContainer->setHidden(!visible);
}

bool Sidebar::isSidebarVisible() const
{
    return !d->sideContainer->isHidden();
}

void Sidebar::moveSplitter(int sideWidgetSize)
{
    QList<int> splitterSizeList = d->splitter->sizes();
    const int total = splitterSizeList.at(0) + splitterSizeList.at(1);
    splitterSizeList.replace(0, total - sideWidgetSize);
    splitterSizeList.replace(1, sideWidgetSize);
    d->splitter->setSizes(splitterSizeList);
}

QWidget *Sidebar::getSideContainer() const
{
    return d->sideContainer;
}

void Sidebar::splitterMoved(int /*pos*/, int index)
{
    // if the side panel has been resized, save splitter sizes
    if (index == 1) {
        saveSplitterSize();
    }
}

void Sidebar::saveSplitterSize() const
{
    Okular::Settings::setSplitterSizes(d->splitter->sizes());
    Okular::Settings::self()->save();
}

void Sidebar::dragEnterEvent(QDragEnterEvent *event)
{
    event->setAccepted(event->mimeData()->hasUrls());
}

void Sidebar::dropEvent(QDropEvent *event)
{
    const QList<QUrl> list = KUrlMimeData::urlsFromMimeData(event->mimeData());
    Q_EMIT urlsDropped(list);
}
