/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qaction.h>
#include <qcursor.h>
#include <qheaderview.h>
#include <qlayout.h>
#include <qtoolbar.h>
#include <qtreewidget.h>

#include <kdebug.h>
#include <kicon.h>
#include <klocale.h>
#include <kmenu.h>
#include <ktreewidgetsearchline.h>

#include "bookmarklist.h"
#include "pageitemdelegate.h"
#include "core/bookmarkmanager.h"
#include "core/document.h"

class BookmarkItem : public QTreeWidgetItem
{
    public:
        BookmarkItem( const KBookmark& bm )
            : QTreeWidgetItem(), m_bookmark( bm )
        {
            setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
            m_url = m_bookmark.url();
            m_viewport = Okular::DocumentViewport( m_url.ref() );
            m_url.setRef( QString() );
            setText( 0, m_bookmark.fullText() );
            if ( m_viewport.isValid() )
                setData( 0, PageItemDelegate::PageRole, QString::number( m_viewport.pageNumber + 1 ) );
        }

        KBookmark bookmark() const
        {
            return m_bookmark;
        }

        const Okular::DocumentViewport& viewport() const
        {
            return m_viewport;
        }

        KUrl url() const
        {
            return m_url;
        }

    private:
        KBookmark m_bookmark;
        KUrl m_url;
        Okular::DocumentViewport m_viewport;
};


BookmarkList::BookmarkList( Okular::Document *document, QWidget *parent )
    : QWidget( parent ), m_document( document )
{
    QVBoxLayout *mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );

    QHBoxLayout *searchlay = new QHBoxLayout();
    searchlay->setMargin( 2 );
    searchlay->setSpacing( 2 );
    mainlay->addLayout( searchlay );

    m_searchLine = new KTreeWidgetSearchLine( this );
    searchlay->addWidget( m_searchLine );

    m_tree = new QTreeWidget( this );
    mainlay->addWidget( m_tree );
    QStringList cols;
    cols.append( "Bookmarks" );
    m_tree->setContextMenuPolicy(  Qt::CustomContextMenu );
    m_tree->setHeaderLabels( cols );
    m_tree->setSortingEnabled( false );
    m_tree->setRootIsDecorated( true );
    m_tree->setAlternatingRowColors( true );
    m_tree->setItemDelegate( new PageItemDelegate( m_tree ) );
    m_tree->header()->hide();
    m_tree->setSelectionBehavior( QAbstractItemView::SelectRows );
    connect( m_tree, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ), this, SLOT( slotExecuted( QTreeWidgetItem * ) ) );
    connect( m_tree, SIGNAL( itemActivated( QTreeWidgetItem *, int ) ), this, SLOT( slotExecuted( QTreeWidgetItem * ) ) );
    connect( m_tree, SIGNAL( itemChanged( QTreeWidgetItem *, int ) ), this, SLOT( slotChanged( QTreeWidgetItem * ) ) );
    connect( m_tree, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( slotContextMenu( const QPoint& ) ) );
    m_searchLine->addTreeWidget( m_tree );

    QToolBar * bookmarkController = new QToolBar( this );
    mainlay->addWidget( bookmarkController );
    bookmarkController->setObjectName( "BookmarkControlBar" );
    // change toolbar appearance
    bookmarkController->setIconSize( QSize( 16, 16 ) );
    bookmarkController->setMovable( false );
    QSizePolicy sp = bookmarkController->sizePolicy();
    sp.setVerticalPolicy( QSizePolicy::Minimum );
    bookmarkController->setSizePolicy( sp );
    // insert a togglebutton [show only bookmarks in the current document]
    m_showBoomarkOnlyAction = bookmarkController->addAction( KIcon( "bookmark" ), i18n( "Current document only" ) );
    m_showBoomarkOnlyAction->setCheckable( true );
    connect( m_showBoomarkOnlyAction, SIGNAL( toggled( bool ) ), this, SLOT( slotFilterBookmarks( bool ) ) );

}

BookmarkList::~BookmarkList()
{
}

uint BookmarkList::observerId() const
{
    return BOOKMARKLIST_ID;
}

void BookmarkList::notifySetup( const QVector< Okular::Page * > & pages, bool documentChanged )
{
    if ( !documentChanged )
        return;

    // clear contents
    m_tree->clear();
    m_searchLine->clear();

    if ( pages.isEmpty() )
        return;

    rebuildTree( m_showBoomarkOnlyAction->isChecked() );
}

void BookmarkList::notifyPageChanged( int pageNumber, int changedFlags )
{
    // only check if there are changes to bookmarks
    if ( !( changedFlags & Okular::DocumentObserver::Bookmark ) )
        return;

    (void)pageNumber;

    m_tree->clear();

    rebuildTree( m_showBoomarkOnlyAction->isChecked() );
}

void BookmarkList::slotFilterBookmarks( bool on )
{
    m_tree->clear();

    rebuildTree( on );
}

void BookmarkList::slotExecuted( QTreeWidgetItem * item )
{
    BookmarkItem* bmItem = dynamic_cast<BookmarkItem*>( item );
    if ( !bmItem || !bmItem->viewport().isValid() )
        return;

    goTo( bmItem );
}

void BookmarkList::slotChanged( QTreeWidgetItem * item )
{
    BookmarkItem* bmItem = dynamic_cast<BookmarkItem*>( item );
    if ( !bmItem || !bmItem->viewport().isValid() )
        return;

    QDomElement el = bmItem->bookmark().internalElement();
    QDomElement titleel = el.firstChild().toElement();
    if ( titleel.isNull() )
    {
        titleel = el.ownerDocument().createElement( "title" );
        el.appendChild( titleel );
    }
    QDomText textel = titleel.firstChild().toText();
    if ( textel.isNull() )
    {
        textel = el.ownerDocument().createTextNode( "" );
        titleel.appendChild( textel );
    }
    textel.setData( bmItem->text( 0 ) );
    m_document->bookmarkManager()->save();
}

void BookmarkList::slotContextMenu( const QPoint& p )
{
    QTreeWidgetItem * item = m_tree->itemAt( p );
    BookmarkItem* bmItem = item ? dynamic_cast<BookmarkItem*>( item ) : 0;
    if ( !bmItem || !bmItem->viewport().isValid() )
        return;

    KMenu menu( this );
    QAction * gotobm = menu.addAction( i18n( "Go to This Bookmark" ) );
    QAction * editbm = menu.addAction( KIcon( "edit" ), i18n( "Rename Bookmark" ) );
    QAction * removebm = menu.addAction( KIcon( "remove" ), i18n( "Remove Bookmark" ) );
    QAction * res = menu.exec( QCursor::pos() );
    if ( !res )
        return;

    if ( res == gotobm )
        goTo( bmItem );
    else if ( res == editbm )
        m_tree->editItem( item, 0 );
    else if ( res == removebm )
        m_document->removeBookmark( bmItem->url(), bmItem->bookmark() );
}

QList<QTreeWidgetItem*> createItems( const KUrl& baseurl, const KBookmark::List& bmlist )
{
    (void)baseurl;
    QList<QTreeWidgetItem*> ret;
    foreach ( const KBookmark& bm, bmlist )
    {
//        kDebug() << "checking '" << tmp << "'" << endl;
//        kDebug() << "      vs '" << baseurl << "'" << endl;
        // TODO check that bm and baseurl are the same (#ref excluded)
        QTreeWidgetItem * item = new BookmarkItem( bm );
        ret.append( item );
    }
    return ret;
}

void BookmarkList::rebuildTree( bool filter )
{
    KUrl::List urls = m_document->bookmarkManager()->files();
    if ( filter )
    {
        foreach ( const KUrl& url, urls )
        {
            if ( url == m_document->currentDocument() )
            {
                m_tree->addTopLevelItems( createItems( url, m_document->bookmarkManager()->bookmarks( url ) ) );
                break;
            }
        }
    }
    else
    {
        QTreeWidgetItem * currenturlitem = 0;
        foreach ( const KUrl& url, urls )
        {
            QTreeWidgetItem * item = new QTreeWidgetItem( m_tree );
            item->setText( 0, url.isLocalFile() ? url.path() : url.prettyUrl() );
            item->addChildren( createItems( url, m_document->bookmarkManager()->bookmarks( url ) ) );
            if ( !currenturlitem && url == m_document->currentDocument() )
            {
                currenturlitem = item;
            }
        }
        if ( currenturlitem )
        {
            currenturlitem->setExpanded( true );
            currenturlitem->setIcon( 0, KIcon( "bookmark" ) );
            m_tree->scrollToItem( currenturlitem, QAbstractItemView::PositionAtTop );
        }
    }
}

void BookmarkList::goTo( BookmarkItem * item )
{
    if ( item->url() == m_document->currentDocument() )
    {
        m_document->setViewport( item->viewport() );
    }
    else
    {
        m_document->setNextDocumentViewport( item->viewport() );
        emit openUrl( item->url() );
    }
}


#include "bookmarklist.moc"
