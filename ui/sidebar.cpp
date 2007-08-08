/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "sidebar.h"

#include <qabstractitemdelegate.h>
#include <qevent.h>
#include <qfont.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlist.h>
#include <qlistwidget.h>
#include <qpainter.h>
#include <qscrollbar.h>
#include <qsplitter.h>
#include <qstackedwidget.h>

#include "settings.h"

static const int SidebarItemType = QListWidgetItem::UserType + 1;

/* List item representing a sidebar entry. */
class SidebarItem : public QListWidgetItem
{
    public:
        SidebarItem( QWidget* w, const QIcon &icon, const QString &text )
            : QListWidgetItem( 0, SidebarItemType ),
              m_widget( w )
        {
            setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
            setIcon( icon );
            setText( QString() );
            setToolTip( text );
        }

        QWidget* widget() const
        {
            return m_widget;
        }

    private:
        QWidget *m_widget;
};


/* A simple delegate to paint the icon of each item */
class SidebarDelegate : public QAbstractItemDelegate
{
    public:
        SidebarDelegate( QObject *parent = 0 );
        ~SidebarDelegate();

        // from QAbstractItemDelegate
        void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
        QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const;
};

SidebarDelegate::SidebarDelegate( QObject *parent )
    : QAbstractItemDelegate( parent )
{
}

SidebarDelegate::~SidebarDelegate()
{
}

void SidebarDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    QBrush backBrush;
    bool disabled = false;
    if ( !( option.state & QStyle::State_Enabled ) )
    {
        backBrush = option.palette.brush( QPalette::Disabled, QPalette::Base );
        disabled = true;
    }
    else if ( option.state & ( QStyle::State_HasFocus | QStyle::State_Selected ) )
    {
        backBrush = option.palette.brush( QPalette::Highlight );
    }
    else if ( option.state & QStyle::State_MouseOver )
    {
        backBrush = option.palette.color( QPalette::Highlight ).light( 115 );
    }
    else /*if ( option.state & QStyle::State_Enabled )*/
    {
        backBrush = option.palette.brush( QPalette::Base );
    }
    painter->fillRect( option.rect, backBrush );
    QIcon icon = index.data( Qt::DecorationRole ).value< QIcon >();
    if ( !icon.isNull() )
    {
        QPoint iconpos(
            ( option.rect.width() - option.decorationSize.width() ) / 2,
            ( option.rect.height() - option.decorationSize.height() ) / 2
        );
        iconpos += option.rect.topLeft();
        QIcon::Mode iconmode = disabled ? QIcon::Disabled : QIcon::Normal;
        painter->drawPixmap( iconpos, icon.pixmap( option.decorationSize, iconmode ) );
    }
}

QSize SidebarDelegate::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    Q_UNUSED( index )
    return option.decorationSize + QSize( 10, 10 );
}


/* A custom list widget that ignores the events for disabled items */
class SidebarListWidget : public QListWidget
{
    public:
        SidebarListWidget( QWidget *parent = 0 );
        ~SidebarListWidget();

    protected:
        // from QListWidget
        void mouseDoubleClickEvent( QMouseEvent *event );
        void mouseMoveEvent( QMouseEvent *event );
        void mousePressEvent( QMouseEvent *event );
        void mouseReleaseEvent( QMouseEvent *event );

        QModelIndex moveCursor( QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers );
};

SidebarListWidget::SidebarListWidget( QWidget *parent )
    : QListWidget( parent )
{
}

SidebarListWidget::~SidebarListWidget()
{
}

void SidebarListWidget::mouseDoubleClickEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if ( index.isValid() && !( index.flags() & Qt::ItemIsSelectable ) )
        return;

    QListWidget::mouseDoubleClickEvent( event );
}

void SidebarListWidget::mouseMoveEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if ( index.isValid() && !( index.flags() & Qt::ItemIsSelectable ) )
        return;

    QListWidget::mouseMoveEvent( event );
}

void SidebarListWidget::mousePressEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if ( index.isValid() && !( index.flags() & Qt::ItemIsSelectable ) )
        return;

    QListWidget::mousePressEvent( event );
}

void SidebarListWidget::mouseReleaseEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if ( index.isValid() && !( index.flags() & Qt::ItemIsSelectable ) )
        return;

    QListWidget::mouseReleaseEvent( event );
}

QModelIndex SidebarListWidget::moveCursor( QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers )
{
    QModelIndex oldindex = currentIndex();
    QModelIndex newindex = QListWidget::moveCursor( cursorAction, modifiers );
    // dirty hack to change item when the key cursor changes item
    if ( oldindex != newindex )
    {
        emit itemClicked( itemFromIndex( newindex ) );
    }
    return newindex;
}


/* Private storage. */
class Sidebar::Private
{
public:
    Private()
        : sideWidget( 0 ), bottomWidget( 0 ), splitterSizesSet( false )
    {
    }

    QListWidget *list;
    QSplitter *splitter;
    QStackedWidget *stack;
    QWidget *sideContainer;
    QLabel *sideTitle;
    QVBoxLayout *vlay;
    QWidget *sideWidget;
    QWidget *bottomWidget;
    QList< SidebarItem* > pages;
    bool splitterSizesSet;
};


Sidebar::Sidebar( QWidget *parent )
    : QWidget( parent ), d( new Private )
{
    QHBoxLayout *mainlay = new QHBoxLayout( this );
    mainlay->setMargin( 0 );

    d->list = new SidebarListWidget( this );
    mainlay->addWidget( d->list );
    d->list->setMouseTracking( true );
    d->list->viewport()->setAttribute( Qt::WA_Hover );
    d->list->setItemDelegate( new SidebarDelegate( d->list ) );
    d->list->setUniformItemSizes( true );
    d->list->setSelectionMode( QAbstractItemView::SingleSelection );
    d->list->setIconSize( QSize( 48, 48 ) );
    d->list->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    d->list->setFixedWidth(
        d->list->iconSize().width() +
        d->list->verticalScrollBar()->sizeHint().width() +
        d->list->frameWidth() * 2
    );
    QPalette pal = d->list->palette();
    pal.setBrush( QPalette::Base, pal.brush( QPalette::Window ) );
    d->list->setPalette( pal );

    d->splitter = new QSplitter( this );
    mainlay->addWidget( d->splitter );
    d->splitter->setOpaqueResize( true );
    d->splitter->setChildrenCollapsible( false );

    d->sideContainer = new QWidget( d->splitter );
    d->sideContainer->setMinimumWidth( 90 );
    d->sideContainer->setMaximumWidth( 300 );
    d->vlay = new QVBoxLayout( d->sideContainer );
    d->vlay->setMargin( 0 );

    d->sideTitle = new QLabel( d->sideContainer );
    d->vlay->addWidget( d->sideTitle );
    QFont tf = d->sideTitle->font();
    tf.setBold( true );
    d->sideTitle->setFont( tf );
    d->sideTitle->setMargin( 3 );
    d->sideTitle->setIndent( 3 );

    d->stack = new QStackedWidget( d->sideContainer );
    d->vlay->addWidget( d->stack );
    d->sideContainer->hide();

    connect( d->list, SIGNAL( itemClicked( QListWidgetItem* ) ), this, SLOT( itemClicked( QListWidgetItem* ) ) );
    connect( d->splitter, SIGNAL( splitterMoved( int, int ) ), this, SLOT( splitterMoved( int, int ) ) );
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
    d->list->addItem( newitem );
    d->pages.append( newitem );
    widget->setParent( d->stack );
    d->stack->addWidget( widget );
    // updating the minimum height of the icon view, so all are visible with no scrolling
    d->list->setMinimumHeight(
        d->list->visualRect( d->list->model()->index( d->pages.count() - 1, 0 ) ).bottom() +
        d->list->frameWidth() * 2
    );
    return d->pages.count() - 1;
}

void Sidebar::setMainWidget( QWidget *widget )
{
    delete d->sideWidget;
    d->sideWidget = widget;
    if ( widget )
    {
        // setting the splitter as parent for the widget automatically plugs it
        // into the splitter, neat!
        widget->setParent( d->splitter );

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
    if ( widget )
    {
        widget->setParent( this );
        d->vlay->addWidget( d->bottomWidget );
    }
}

void Sidebar::setItemEnabled( int index, bool enabled )
{
    if ( index < 0 || index >= d->pages.count() )
        return;

    Qt::ItemFlags f = d->pages.at( index )->flags();
    if ( enabled )
    {
        f |= Qt::ItemIsEnabled;
        f |= Qt::ItemIsSelectable;
    }
    else
    {
        f &= ~Qt::ItemIsEnabled;
        f &= ~Qt::ItemIsSelectable;
    }
    d->pages.at( index )->setFlags( f );

    if ( !enabled && index == currentIndex() )
        // find an enabled item, and select that one
        for ( int i = 0; i < d->pages.count(); ++i )
            if ( d->pages.at(i)->flags() & Qt::ItemIsEnabled )
            {
                setCurrentIndex( i );
                break;
            }
}

bool Sidebar::isItemEnabled( int index ) const
{
    if ( index < 0 || index >= d->pages.count() )
        return false;

    Qt::ItemFlags f = d->pages.at( index )->flags();
    return ( f & Qt::ItemIsEnabled ) == Qt::ItemIsEnabled;
}

void Sidebar::setCurrentIndex( int index )
{
    if ( index < 0 || index >= d->pages.count() || !isItemEnabled( index ) )
        return;

    itemClicked( d->pages.at( index ) );
    d->list->selectionModel()->clear();
    d->list->setCurrentRow( index );
}

int Sidebar::currentIndex() const
{
    return d->list->currentRow();
}

void Sidebar::setSidebarVisibility( bool visible )
{
    static bool sideWasVisible = d->sideContainer->isVisible();

    d->list->setVisible( visible );
    if ( visible )
    {
        d->sideContainer->setVisible( sideWasVisible );
    }
    else
    {
        sideWasVisible = d->sideContainer->isVisible();
        d->sideContainer->setVisible( false );
    }
}

void Sidebar::itemClicked( QListWidgetItem *item )
{
    if ( !item )
        return;

    SidebarItem* sbItem = dynamic_cast< SidebarItem* >( item );
    if ( !sbItem )
        return;

    if ( !d->sideContainer->isHidden() && sbItem->widget() == d->stack->currentWidget() )
    {
        d->list->selectionModel()->clear();
        d->sideContainer->hide();
    }
    else
    {
        if ( d->sideContainer->isHidden() )
            d->sideContainer->show();
        d->stack->setCurrentWidget( sbItem->widget() );
        d->sideTitle->setText( sbItem->toolTip() );
    }
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
    Okular::Settings::self()->writeConfig();
}


#include "sidebar.moc"
