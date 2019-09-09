/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2009 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "sidebar.h"

#include <qaction.h>
#include <qapplication.h>
#include <qcombobox.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlist.h>
#include <qlistwidget.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <qsplitter.h>
#include <qstackedwidget.h>
#include <qmimedata.h>

#include <kiconloader.h>
#include <KLocalizedString>
#include <qmenu.h>
#include <kcolorscheme.h>
#include <kurlmimedata.h>

#include "settings.h"

static const int SidebarItemType = QListWidgetItem::UserType + 1;

/* List item representing a sidebar entry. */
class SidebarItem : public QListWidgetItem
{
    public:
        SidebarItem( QWidget* w, const QIcon &icon, const QString &text )
            : QListWidgetItem( nullptr, SidebarItemType ),
              m_widget( w )
        {
            setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
            setIcon( icon );
            setText( text );
            setToolTip( text );
        }

        QWidget* widget() const
        {
            return m_widget;
        }

    private:
        QWidget *m_widget;
};


/* Private storage. */
class Sidebar::Private
{
public:
    Private()
        : sideWidget( nullptr ), bottomWidget( nullptr ), splitterSizesSet( false ),
          itemsHeight( 0 )
    {
    }

    int indexOf(QWidget *w) const
    {
        for (int i = 0; i < pages.count(); ++i) {
            if (pages[i]->widget() == w) return i;
        }
        return -1;
    }

    QSplitter *splitter;
    QStackedWidget *stack;
    QWidget *sideContainer;
    QComboBox *viewChooserCombobox;
    QVBoxLayout *vlay;
    QWidget *sideWidget;
    QWidget *bottomWidget;
    QList< SidebarItem* > pages;
    int currentIndex;
    bool splitterSizesSet;
    int itemsHeight;
};


Sidebar::Sidebar( QWidget *parent )
    : QWidget( parent ), d( new Private )
{
    QHBoxLayout *mainlay = new QHBoxLayout( this );
    mainlay->setMargin( 0 );
    mainlay->setSpacing( 0 );

    setAutoFillBackground( true );
    setAcceptDrops( true );

    d->splitter = new QSplitter( this );
    mainlay->addWidget( d->splitter );
    d->splitter->setOpaqueResize( true );
    d->splitter->setChildrenCollapsible( false );

    // d->sideContainer holds all the actual content
    d->sideContainer = new QWidget( d->splitter );
    d->sideContainer->setMinimumWidth( 90 );
    d->sideContainer->setMaximumWidth( 600 );
    d->vlay = new QVBoxLayout( d->sideContainer );

    // Create the combobox view chooser/title
    d->viewChooserCombobox = new QComboBox( d->sideContainer );
    connect(d->viewChooserCombobox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Sidebar::setCurrentIndex);
    d->vlay->addWidget( d->viewChooserCombobox );

    d->stack = new QStackedWidget( d->sideContainer );
    d->vlay->addWidget( d->stack );

    connect(d->splitter, &QSplitter::splitterMoved, this, &Sidebar::splitterMoved);
}

Sidebar::~Sidebar()
{
    delete d;
}

int Sidebar::addItem( QWidget *widget, const QIcon &icon, const QString &text )
{
    if ( !widget )
        return -1;

    SidebarItem *newitem = new SidebarItem( widget, icon, text );
    d->pages.append( newitem );
    d->viewChooserCombobox->addItem(icon, text);
    widget->setParent( d->stack );
    d->stack->addWidget( widget );
    return d->pages.count() - 1;
}

void Sidebar::setMainWidget( QWidget *widget )
{
    delete d->sideWidget;
    d->sideWidget = widget;
    if ( d->sideWidget )
    {
        // setting the splitter as parent for the widget automatically plugs it
        // into the splitter, neat!
        d->sideWidget->setParent( d->splitter );

        if ( !d->splitterSizesSet )
        {
            QList<int> splitterSizes = Okular::Settings::splitterSizes();
            if ( !splitterSizes.count() )
            {
                // the first time use 1/10 for the panel and 9/10 for the pageView
                splitterSizes.push_back( 50 );
                splitterSizes.push_back( 500 );
            }
            d->splitter->setSizes( splitterSizes );
            d->splitterSizesSet = true;
        }
    }
}

void Sidebar::setBottomWidget( QWidget *widget )
{
    delete d->bottomWidget;
    d->bottomWidget = widget;
    if ( d->bottomWidget )
    {
        d->bottomWidget->setParent( this );
        d->vlay->addWidget( d->bottomWidget );
    }
}

void Sidebar::setCurrentItem( QWidget *widget )
{
    const int index = d->indexOf( widget );
    setCurrentIndex( index );
}

void Sidebar::setCurrentIndex( int index )
{
    if ( index < 0 )
        return;

    QListWidgetItem *item = d->pages.at( index );

    if ( !item )
        return;

    SidebarItem* sbItem = dynamic_cast< SidebarItem* >( item );
    if ( !sbItem )
        return;

    if ( sbItem->widget() == d->stack->currentWidget() )
        return;

    d->stack->setCurrentWidget( sbItem->widget() );
    d->viewChooserCombobox->setCurrentIndex( index );
    d->currentIndex = index;
}

QWidget *Sidebar::currentItem() const
{
    if (d->currentIndex < 0 || d->currentIndex >= d->pages.count())
        return nullptr;

    return d->pages[d->currentIndex]->widget();
}

void Sidebar::setSidebarVisibility( bool visible )
{
    if ( visible != d->sideContainer->isHidden() )
        return;

    d->sideContainer->setHidden( !visible );
}

bool Sidebar::isSidebarVisible() const
{
    return !d->sideContainer->isHidden();
}

void Sidebar::moveSplitter(int sideWidgetSize)
{
    QList<int> splitterSizeList = d->splitter->sizes();
    const int total = splitterSizeList.at( 0 ) + splitterSizeList.at( 1 );
    splitterSizeList.replace( 0, total - sideWidgetSize );
    splitterSizeList.replace( 1, sideWidgetSize );
    d->splitter->setSizes( splitterSizeList );
}

void Sidebar::splitterMoved( int /*pos*/, int index )
{
    // if the side panel has been resized, save splitter sizes
    if ( index == 1 )
        saveSplitterSize();
}

void Sidebar::saveSplitterSize() const
{
    Okular::Settings::setSplitterSizes( d->splitter->sizes() );
    Okular::Settings::self()->save();
}

void Sidebar::dragEnterEvent( QDragEnterEvent* event )
{
    event->setAccepted( event->mimeData()->hasUrls() );
}

void Sidebar::dropEvent( QDropEvent* event )
{
    const QList<QUrl> list = KUrlMimeData::urlsFromMimeData( event->mimeData() );
    emit urlsDropped( list );
}

#include "sidebar.moc"
