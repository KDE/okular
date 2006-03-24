/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <q3header.h>
#include <qlayout.h>
#include <qtimer.h>
#include <ktoolbar.h>
#include <klistview.h>
#include <klistviewsearchline.h>
#include <kaction.h>
#include <kactionclasses.h>
#include <klocale.h>
#include <kapplication.h>
#include <kiconloader.h>

// local includes
#include "core/annotations.h"
#include "core/document.h"
#include "core/page.h"
#include "conf/settings.h"
#include "side_reviews.h"


Reviews::Reviews( QWidget * parent, KPDFDocument * document )
    : QWidget( parent ), m_document( document ), m_delayTimer( 0 ), m_currentPage( -1 )
{
    // create widgets and layout them vertically
    QVBoxLayout * vLayout = new QVBoxLayout( this, 0, 4 );
    m_toolBar1 = new KToolBar( this, "reviewSearchBar" );
    vLayout->addWidget( m_toolBar1 );
    m_listView = new KListView( this );
    vLayout->addWidget( m_listView );
    m_toolBar2 = new KToolBar( this, "reviewOptsBar" );
    vLayout->addWidget( m_toolBar2 );

    // setup 1-UPPER toolbar and searchLine
    m_searchLine = new KListViewSearchLine( m_toolBar1, m_listView );
    m_toolBar1->setIconDimensions( 16 );
    m_toolBar1->setMovable( false );
    // - add Clear button
    QString clearIconName = KApplication::reverseLayout() ? "clear_left" : "locationbar_erase";
#warning lots of KToolBar code to port
/*    m_toolBar1->insertButton( clearIconName, 1, SIGNAL( clicked() ),
        m_searchLine, SLOT( clear() ), true, i18n( "Clear Filter" ) );
    // - add Search line
    m_toolBar1->insertWidget( 2, 0, m_searchLine );
    m_toolBar1->setItemAutoSized( 2 );
    m_toolBar1->getWidget( 2 )->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );

    // setup 2-LOWER toolbar
    m_toolBar2->setIconDimensions( 16 );
    m_toolBar2->setMovable( false );
    // - add Page button
    m_toolBar2->insertButton( "txt", 1, SIGNAL( toggled( bool ) ),
        this, SLOT( slotPageEnabled( bool ) ), true, i18n( "Group by Page" ) );
    m_toolBar2->setToggle( 1 );
    m_toolBar2->setButton( 1, KpdfSettings::groupByPage() );
    // - add Author button
    m_toolBar2->insertButton( "personal", 2, SIGNAL( toggled( bool ) ),
        this, SLOT( slotAuthorEnabled( bool ) ), true, i18n( "Group by Author" ) );
    m_toolBar2->setToggle( 2 );
    m_toolBar2->setButton( 2, KpdfSettings::groupByAuthor() );
    // - add separator
    m_toolBar2->insertLineSeparator();
    // - add Current Page Only button
    m_toolBar2->insertButton( "1downarrow", 3, SIGNAL( toggled( bool ) ),
        this, SLOT( slotCurrentPageOnly( bool ) ), true, i18n( "Show reviews for current page only" ) );
    m_toolBar2->setToggle( 3 );
    m_toolBar2->setButton( 3, KpdfSettings::currentPageOnly() );*/

    // customize listview appearance
    m_listView->addColumn( i18n("Annotation") );
    m_listView->header()->hide();
    m_listView->setTreeStepSize( 16 );
    m_listView->setResizeMode( KListView::AllColumns );
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


class ReviewItem : public Q3ListViewItem
{
    public:
        ReviewItem( Q3ListView * parent, const QString & text )
            : Q3ListViewItem( parent, text ) {}

        void paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align )
        {
            QColorGroup myCg = cg;
            myCg.setColor( QColorGroup::Text, Qt::red );
            Q3ListViewItem::paintCell( p, myCg, column, width, align );
        }
        void paintFocus( QPainter *, const QColorGroup &, const QRect & )
        {
        }
};

void Reviews::slotUpdateListView()
{
    // reset listview to default
    m_listView->clear();
    m_listView->setRootIsDecorated( true );
    m_listView->setSelectionMode( Q3ListView::Single );

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
    if ( !m_listView->firstChild() )
    {
        m_listView->setRootIsDecorated( false );
        m_listView->setSelectionMode( Q3ListView::NoSelection );
        new ReviewItem( m_listView, i18n("<No Items>") );
    }
}

void Reviews::addContents( const KPDFPage * page )
{
    // if page-grouping -> create Page subnode
    Q3ListViewItem * pageItem = 0;
    if ( KpdfSettings::groupByPage() )
    {
        QString pageText = i18n( "page %1" ).arg( page->number() + 1 );
        pageItem = new Q3ListViewItem( m_listView, pageText );
        pageItem->setPixmap( 0, SmallIcon( "txt" ) );
        pageItem->setOpen( KpdfSettings::groupByAuthor() );
    }

    // iterate over all annotations in this page
    const QLinkedList< Annotation * > & annots = page->getAnnotations();
    QLinkedList< Annotation * >::const_iterator aIt = annots.begin(), aEnd = annots.end();
    for ( ; aIt != aEnd; ++aIt )
    {
        // get annotation
        Annotation * annotation = *aIt;

        // if page-grouping -> create Author subnode
        Q3ListViewItem * authorItem = pageItem;
        if ( KpdfSettings::groupByAuthor() )
        {
            // get author's name
            QString author = annotation->author;
            if ( author.isEmpty() )
                author = i18n( "Unknown" );

            // find out a previous entry by author
            authorItem = pageItem ? pageItem->firstChild() : m_listView->firstChild();
            while ( authorItem && authorItem->text(0) != author )
                authorItem = authorItem->nextSibling();

            // if item not found, create one
            if ( !authorItem )
            {
                if ( pageItem )
                    authorItem = new Q3ListViewItem( pageItem, author );
                else
                    authorItem = new Q3ListViewItem( m_listView, author );
                QString icon = author != i18n( "Unknown" ) ? "personal" : "presence_away";
                authorItem->setPixmap( 0, SmallIcon( icon ) );
            }
        }

        // create Annotation subnode
        Q3ListViewItem * singleItem = authorItem ?
            new Q3ListViewItem( authorItem, annotation->contents ) :
            new Q3ListViewItem( m_listView, annotation->contents );
        singleItem->setPixmap( 0, SmallIcon( "kpdf" ) );
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
        connect( m_delayTimer, SIGNAL( timeout() ), this, SLOT( slotUpdateListView() ) );
    }

    // start timer if not already running
    if ( !m_delayTimer->isActive() )
        m_delayTimer->start( delayms, true );
}

#include "side_reviews.moc"
