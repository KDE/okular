/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qheaderview.h>
#include <qlayout.h>
#include <qsizepolicy.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qtoolbar.h>
#include <qtreewidget.h>
#include <kaction.h>
#include <klocale.h>
#include <qapplication.h>
#include <kiconloader.h>
#include <ktreewidgetsearchline.h>

// local includes
#include "core/annotations.h"
#include "core/document.h"
#include "core/page.h"
#include "settings.h"
#include "side_reviews.h"


Reviews::Reviews( QWidget * parent, KPDFDocument * document )
    : QWidget( parent ), m_document( document ), m_delayTimer( 0 ), m_currentPage( -1 )
{
    // create widgets and layout them vertically
    QVBoxLayout * vLayout = new QVBoxLayout( this );
    vLayout->setMargin( 0 );
    vLayout->setSpacing( 4 );
    m_toolBar1 = new QToolBar( this );
    m_toolBar1->setObjectName( "reviewSearchBar" );
    vLayout->addWidget( m_toolBar1 );
    QSizePolicy sp = m_toolBar1->sizePolicy();
    sp.setVerticalPolicy( QSizePolicy::Minimum );
    m_toolBar1->setSizePolicy( sp );
    m_listView = new QTreeWidget( this );
    vLayout->addWidget( m_listView );
    m_toolBar2 = new QToolBar( this );
    m_toolBar2->setObjectName( "reviewOptsBar" );
    vLayout->addWidget( m_toolBar2 );
    sp = m_toolBar2->sizePolicy();
    sp.setVerticalPolicy( QSizePolicy::Minimum );
    m_toolBar2->setSizePolicy( sp );

    // setup 1-UPPER toolbar and searchLine
    m_searchLine = new KTreeWidgetSearchLine( m_toolBar1, m_listView );
    m_toolBar1->setIconSize( QSize( 16, 16 ) );
    m_toolBar1->setMovable( false );
    // - add Clear button
    QString clearIconName = QApplication::layoutDirection() ==
 Qt::RightToLeft ? "clear_left" : "locationbar_erase";
    QAction * clearAction = m_toolBar1->addAction(
        KIcon( clearIconName ), i18n( "Clear Filter" ), m_searchLine, SLOT( clear() ) );
    // - add Search line
    m_toolBar1->addWidget(m_searchLine);

    // setup 2-LOWER toolbar
    m_toolBar2->setIconSize( QSize( 16, 16 ) );
    m_toolBar2->setMovable( false );
    // - add Page button
    QAction * groupByPageAction = m_toolBar2->addAction( KIcon( "txt" ), i18n( "Group by Page" ) );
    groupByPageAction->setCheckable( true );
    connect( groupByPageAction, SIGNAL( toggled( bool ) ), this, SLOT( slotPageEnabled( bool ) ) );
    groupByPageAction->setChecked( KpdfSettings::groupByPage() );
    // - add Author button
    QAction * groupByAuthorAction = m_toolBar2->addAction( KIcon( "personal" ), i18n( "Group by Author" ) );
    groupByAuthorAction->setCheckable( true );
    connect( groupByAuthorAction, SIGNAL( toggled( bool ) ), this, SLOT( slotAuthorEnabled( bool ) ) );
    groupByAuthorAction->setChecked( KpdfSettings::groupByAuthor() );
    // - add separator
    m_toolBar2->addSeparator();
    // - add Current Page Only button
    QAction * curPageOnlyAction = m_toolBar2->addAction( KIcon( "1downarrow" ), i18n( "Show reviews for current page only" ) );
    curPageOnlyAction->setCheckable( true );
    connect( curPageOnlyAction, SIGNAL( toggled( bool ) ), this, SLOT( slotCurrentPageOnly( bool ) ) );
    curPageOnlyAction->setChecked( KpdfSettings::currentPageOnly() );

    // customize listview appearance
    QStringList cols;
    cols.append( i18n("Annotation") );
    m_listView->setHeaderLabels( cols );
    m_listView->header()->setResizeMode( QHeaderView::Stretch );
    m_listView->header()->hide();
    m_listView->setIndentation( 16 );
}

//BEGIN DocumentObserver Notifies -> requestListViewUpdate
void Reviews::notifySetup( const QVector< KPDFPage * > & pages, bool documentChanged )
{
    // grab the page array when document changes
    if ( documentChanged )
    {
        m_currentPage = -1;
        m_pages = pages;

        // request an update to the listview
        m_listView->clear();
        requestListViewUpdate( 1000 );
    }
}

void Reviews::notifyViewportChanged( bool /*smoothMove*/ )
{
    // sync current page and update listview on change if filtering-on-page
    int page = m_document->currentPage();
    if ( page != m_currentPage )
    {
        m_currentPage = page;
        if ( KpdfSettings::currentPageOnly() )
            requestListViewUpdate();
    }
}

void Reviews::notifyPageChanged( int pageNumber, int changedFlags )
{
    // only check if there are changes to annotations
    if ( changedFlags & DocumentObserver::Annotations )
    {
        // if filtering-on-page and the page is not the active one, return
        if ( KpdfSettings::currentPageOnly() && pageNumber != m_currentPage )
            return;
        // request an update to the listview
        // TODO selective update on modified page only
        // nice trick: listview is cleared but screen not updated. zero flicker.
        m_listView->setUpdatesEnabled( false );
        m_listView->clear();
        m_listView->setUpdatesEnabled( true );
        requestListViewUpdate();
    }
}
//END DocumentObserver Notifies 

//BEGIN GUI Slots -> requestListViewUpdate
void Reviews::slotPageEnabled( bool on )
{
    // store toggle state in Settings and update the listview
    KpdfSettings::setGroupByPage( on );
    requestListViewUpdate();
}

void Reviews::slotAuthorEnabled( bool on )
{
    // store toggle state in Settings and update the listview
    KpdfSettings::setGroupByAuthor( on );
    requestListViewUpdate();
}

void Reviews::slotCurrentPageOnly( bool on )
{
    // store toggle state in Settings and update the listview
    KpdfSettings::setCurrentPageOnly( on );
    requestListViewUpdate();
}
//END GUI Slots


class ReviewItem : public QTreeWidgetItem
{
    public:
        ReviewItem( QTreeWidget * parent, const QString & text )
            : QTreeWidgetItem( parent )
        {
            setText( 0, text );
            setTextColor( 0, Qt::red );
        }
};

void Reviews::slotUpdateListView()
{
    // reset listview to default
    m_listView->clear();
    m_listView->setRootIsDecorated( true );
    m_listView->setSelectionMode( QAbstractItemView::SingleSelection );

    if ( KpdfSettings::currentPageOnly() )
    {
        // handle the 'filter on current page'
        if ( m_currentPage >= 0 && m_currentPage < (int)m_pages.count() )
        {
            const KPDFPage * page = m_pages[ m_currentPage ];
            if ( page->hasAnnotations() )
                addContents( page );
        }
    }
    else
    {
        // grab all annotations from pages
        QVector< KPDFPage * >::iterator it = m_pages.begin(), end = m_pages.end();
        for ( ; it != end; ++it )
        {
            const KPDFPage * page = *it;
            if ( page->hasAnnotations() )
                addContents( page );
        }
    }

    // display an info message if no annotations present
    if ( !m_listView->topLevelItem( 0 ) )
    {
        m_listView->setRootIsDecorated( false );
        m_listView->setSelectionMode( QTreeWidget::NoSelection );
        new ReviewItem( m_listView, i18n("<No Items>") );
    }
}

void Reviews::addContents( const KPDFPage * page )
{
    // if page-grouping -> create Page subnode
    QTreeWidgetItem * pageItem = 0;
    if ( KpdfSettings::groupByPage() )
    {
        QString pageText = i18n( "page %1", page->number() + 1 );
        pageItem = new QTreeWidgetItem( m_listView );
        pageItem->setText( 0, pageText );
        pageItem->setIcon( 0, KIcon( "txt" ) );
    }

    QTreeWidgetItemIterator::IteratorFlags itFlags =
            QTreeWidgetItemIterator::All |
            QTreeWidgetItemIterator::HasChildren;

    // iterate over all annotations in this page
    const QLinkedList< Annotation * > & annots = page->getAnnotations();
    QLinkedList< Annotation * >::const_iterator aIt = annots.begin(), aEnd = annots.end();
    for ( ; aIt != aEnd; ++aIt )
    {
        // get annotation
        Annotation * annotation = *aIt;

        // if page-grouping -> create Author subnode
        QTreeWidgetItem * authorItem = pageItem;
        if ( KpdfSettings::groupByAuthor() )
        {
            // get author's name
            QString author = annotation->author;
            if ( author.isEmpty() )
                author = i18n( "Unknown" );

            // find out a previous entry by author
            QTreeWidgetItemIterator it = pageItem
                ? QTreeWidgetItemIterator( pageItem, itFlags )
                : QTreeWidgetItemIterator( m_listView, itFlags );
            while ( (*it) && ( (*it)->text(0) != author ) ) ++it;
            authorItem = *it;

            // if item not found, create one
            if ( !authorItem )
            {
                if ( pageItem )
                    authorItem = new QTreeWidgetItem( pageItem );
                else
                    authorItem = new QTreeWidgetItem( m_listView );
                QString icon = author != i18n( "Unknown" ) ? "personal" : "presence_away";
                authorItem->setText( 0, author );
                authorItem->setIcon( 0, KIcon( icon ) );
            }
        }

        // create Annotation subnode
        QTreeWidgetItem * singleItem = authorItem ?
            new QTreeWidgetItem( authorItem ) :
            new QTreeWidgetItem( m_listView );
        singleItem->setText( 0, annotation->contents );
        singleItem->setIcon( 0, KIcon( "oKular" ) );
    }
}

void Reviews::requestListViewUpdate( int delayms )
{
    // only schedule an update if have any pages
    if ( m_pages.isEmpty() )
        return;

    // create timer if not present
    if ( !m_delayTimer )
    {
        m_delayTimer = new QTimer( this );
        m_delayTimer->setSingleShot( true );
        connect( m_delayTimer, SIGNAL( timeout() ), this, SLOT( slotUpdateListView() ) );
    }

    // start timer if not already running
    if ( !m_delayTimer->isActive() )
        m_delayTimer->start( delayms );
}

#include "side_reviews.moc"
