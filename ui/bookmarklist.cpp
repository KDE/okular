/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "bookmarklist.h"

// qt/kde includes
#include <qaction.h>
#include <qcursor.h>
#include <qheaderview.h>
#include <qlayout.h>
#include <qtoolbar.h>
#include <qtreewidget.h>
#include <QMenu>
#include <QtCore/QDebug>
#include <QIcon>

#include <KLocalizedString>
#include <KTreeWidgetSearchLine>

#include "pageitemdelegate.h"
#include "core/action.h"
#include "core/bookmarkmanager.h"
#include "core/document.h"

static const int BookmarkItemType = QTreeWidgetItem::UserType + 1;
static const int FileItemType = QTreeWidgetItem::UserType + 2;
static const int UrlRole = Qt::UserRole + 1;

class BookmarkItem : public QTreeWidgetItem
{
    public:
        BookmarkItem( const KBookmark& bm )
            : QTreeWidgetItem( BookmarkItemType ), m_bookmark( bm )
        {
            setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
            m_url = m_bookmark.url();
            m_viewport = Okular::DocumentViewport( m_url.fragment(QUrl::FullyDecoded) );
            m_url.setFragment( QString() );
            setText( 0, m_bookmark.fullText() );
            if ( m_viewport.isValid() )
                setData( 0, PageItemDelegate::PageRole, QString::number( m_viewport.pageNumber + 1 ) );
        }

        QVariant data( int column, int role ) const override
        {
            switch ( role )
            {
                case Qt::ToolTipRole:
                    return m_bookmark.fullText();
            }
            return QTreeWidgetItem::data( column, role );
        }

        bool operator<( const QTreeWidgetItem& other ) const override
        {
            if ( other.type() == BookmarkItemType )
            {
                const BookmarkItem *cmp = static_cast< const BookmarkItem* >( &other );
                return m_viewport < cmp->m_viewport;
            }
            return QTreeWidgetItem::operator<( other );
        }

        KBookmark& bookmark()
        {
            return m_bookmark;
        }

        const Okular::DocumentViewport& viewport() const
        {
            return m_viewport;
        }

        QUrl url() const
        {
            return m_url;
        }

    private:
        KBookmark m_bookmark;
        QUrl m_url;
        Okular::DocumentViewport m_viewport;
};


class FileItem : public QTreeWidgetItem
{
    public:
        FileItem( const QUrl & url, QTreeWidget *tree, Okular::Document *document  )
            : QTreeWidgetItem( tree, FileItemType )
        {
            setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable );
            const QString fileString = document->bookmarkManager()->titleForUrl( url );
            setText( 0, fileString );
            setData( 0, UrlRole, qVariantFromValue( url ) );
        }

        QVariant data( int column, int role ) const override
        {
            switch ( role )
            {
                case Qt::ToolTipRole:
                    return i18ncp( "%1 is the file name",
                                   "%1\n\nOne bookmark", "%1\n\n%2 bookmarks",
                                   text( 0 ), childCount() );
            }
            return QTreeWidgetItem::data( column, role );
        }
};


BookmarkList::BookmarkList( Okular::Document *document, QWidget *parent )
    : QWidget( parent ), m_document( document ), m_currentDocumentItem( nullptr )
{
    QVBoxLayout *mainlay = new QVBoxLayout( this );
    mainlay->setMargin( 0 );
    mainlay->setSpacing( 6 );

    m_searchLine = new KTreeWidgetSearchLine( this );
    mainlay->addWidget( m_searchLine );

    m_tree = new QTreeWidget( this );
    mainlay->addWidget( m_tree );
    QStringList cols;
    cols.append( QStringLiteral("Bookmarks") );
    m_tree->setContextMenuPolicy(  Qt::CustomContextMenu );
    m_tree->setHeaderLabels( cols );
    m_tree->setSortingEnabled( false );
    m_tree->setRootIsDecorated( true );
    m_tree->setAlternatingRowColors( true );
    m_tree->setItemDelegate( new PageItemDelegate( m_tree ) );
    m_tree->header()->hide();
    m_tree->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_tree->setEditTriggers( QAbstractItemView::EditKeyPressed );
    connect(m_tree, &QTreeWidget::itemActivated, this, &BookmarkList::slotExecuted);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &BookmarkList::slotContextMenu);
    m_searchLine->addTreeWidget( m_tree );

    QToolBar * bookmarkController = new QToolBar( this );
    mainlay->addWidget( bookmarkController );
    bookmarkController->setObjectName( QStringLiteral( "BookmarkControlBar" ) );
    // change toolbar appearance
    bookmarkController->setIconSize( QSize( 16, 16 ) );
    bookmarkController->setMovable( false );
    QSizePolicy sp = bookmarkController->sizePolicy();
    sp.setVerticalPolicy( QSizePolicy::Minimum );
    bookmarkController->setSizePolicy( sp );
    // insert a togglebutton [show only bookmarks in the current document]
    m_showBoomarkOnlyAction = bookmarkController->addAction( QIcon::fromTheme( QStringLiteral("bookmarks") ), i18n( "Current document only" ) );
    m_showBoomarkOnlyAction->setCheckable( true );
    connect(m_showBoomarkOnlyAction, &QAction::toggled, this, &BookmarkList::slotFilterBookmarks);

    connect( m_document->bookmarkManager(), &Okular::BookmarkManager::bookmarksChanged, this, &BookmarkList::slotBookmarksChanged );

    rebuildTree( m_showBoomarkOnlyAction->isChecked() );
}

BookmarkList::~BookmarkList()
{
    m_document->removeObserver( this );
}

void BookmarkList::notifySetup( const QVector< Okular::Page * > & pages, int setupFlags )
{
    Q_UNUSED( pages );
    if ( !( setupFlags & Okular::DocumentObserver::UrlChanged ) )
        return;

    // clear contents
    m_searchLine->clear();

    if ( m_showBoomarkOnlyAction->isChecked() )
    {
        rebuildTree( m_showBoomarkOnlyAction->isChecked() );
    }
    else
    {
        disconnect(m_tree, &QTreeWidget::itemChanged, this, &BookmarkList::slotChanged);
        if ( m_currentDocumentItem && m_currentDocumentItem != m_tree->invisibleRootItem()  )
        {
            m_currentDocumentItem->setIcon( 0, QIcon() );
        }
        m_currentDocumentItem = itemForUrl( m_document->currentDocument() );
        if ( m_currentDocumentItem && m_currentDocumentItem != m_tree->invisibleRootItem()  )
        {
            m_currentDocumentItem->setIcon( 0, QIcon::fromTheme( QStringLiteral("bookmarks") ) );
            m_currentDocumentItem->setExpanded( true );
        }
        connect(m_tree, &QTreeWidget::itemChanged, this, &BookmarkList::slotChanged);
    }
}

void BookmarkList::slotFilterBookmarks( bool on )
{
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
    if ( bmItem && bmItem->viewport().isValid() )
    {
        bmItem->bookmark().setFullText( bmItem->text( 0 ) );
        m_document->bookmarkManager()->save();
    }

    FileItem* fItem = dynamic_cast<FileItem*>( item );
    if ( fItem )
    {
        const QUrl url = fItem->data( 0, UrlRole ).value< QUrl >();
        m_document->bookmarkManager()->renameBookmark( url, fItem->text( 0 ) );
        m_document->bookmarkManager()->save();
    }
}

void BookmarkList::slotContextMenu( const QPoint& p )
{
    QTreeWidgetItem * item = m_tree->itemAt( p );
    BookmarkItem* bmItem = item ? dynamic_cast<BookmarkItem*>( item ) : nullptr;
    if ( bmItem )
        contextMenuForBookmarkItem( p, bmItem );
    else if ( FileItem* fItem = dynamic_cast< FileItem * >( item ) )
        contextMenuForFileItem( p, fItem );
}

void BookmarkList::contextMenuForBookmarkItem( const QPoint& p, BookmarkItem* bmItem )
{
    Q_UNUSED( p );
    if ( !bmItem || !bmItem->viewport().isValid() )
        return;

    QMenu menu( this );
    QAction * gotobm = menu.addAction( i18n( "Go to This Bookmark" ) );
    QAction * editbm = menu.addAction( QIcon::fromTheme( QStringLiteral("edit-rename") ), i18n( "Rename Bookmark" ) );
    QAction * removebm = menu.addAction( QIcon::fromTheme( QStringLiteral("list-remove") ), i18n( "Remove Bookmark" ) );
    QAction * res = menu.exec( QCursor::pos() );
    if ( !res )
        return;

    if ( res == gotobm )
        goTo( bmItem );
    else if ( res == editbm )
        m_tree->editItem( bmItem, 0 );
    else if ( res == removebm )
        m_document->bookmarkManager()->removeBookmark( bmItem->url(), bmItem->bookmark() );
}

void BookmarkList::contextMenuForFileItem( const QPoint& p, FileItem* fItem )
{
    Q_UNUSED( p );
    if ( !fItem )
        return;

    const QUrl itemurl = fItem->data( 0, UrlRole ).value< QUrl >();
    const bool thisdoc = itemurl == m_document->currentDocument();

    QMenu menu( this );
    QAction * open = nullptr;
    if ( !thisdoc )
        open = menu.addAction( i18nc( "Opens the selected document", "Open Document" ) );
    QAction * editbm = menu.addAction( QIcon::fromTheme( QStringLiteral("edit-rename") ), i18n( "Rename Bookmark" ) );
    QAction * removebm = menu.addAction( QIcon::fromTheme( QStringLiteral("list-remove") ), i18n( "Remove Bookmarks" ) );
    QAction * res = menu.exec( QCursor::pos() );
    if ( !res )
        return;

    if ( res == open )
    {
        Okular::GotoAction action( itemurl.toDisplayString(QUrl::PreferLocalFile), Okular::DocumentViewport() );
        m_document->processAction( &action );
    }
    else if ( res == editbm )
        m_tree->editItem( fItem, 0 );
    else if ( res == removebm )
    {
        KBookmark::List list;
        for ( int i = 0; i < fItem->childCount(); ++i )
        {
            list.append( static_cast<BookmarkItem*>( fItem->child( i ) )->bookmark() );
        }
        m_document->bookmarkManager()->removeBookmarks( itemurl, list );
    }
}

void BookmarkList::slotBookmarksChanged(const QUrl &url )
{
    // special case here, as m_currentDocumentItem could represent
    // the invisible root item
    if ( url == m_document->currentDocument() )
    {
        selectiveUrlUpdate( m_document->currentDocument(), m_currentDocumentItem );
        return;
    }

    // we are showing the bookmarks for the current document only
    if ( m_showBoomarkOnlyAction->isChecked() )
        return;

    QTreeWidgetItem *item = itemForUrl( url );
    selectiveUrlUpdate( url, item );
}

QList<QTreeWidgetItem*> createItems( const QUrl& baseurl, const KBookmark::List& bmlist )
{
    Q_UNUSED(baseurl)
    QList<QTreeWidgetItem*> ret;
    foreach ( const KBookmark& bm, bmlist )
    {
//        qCDebug(OkularUiDebug).nospace() << "checking '" << tmp << "'";
//        qCDebug(OkularUiDebug).nospace() << "      vs '" << baseurl << "'";
        // TODO check that bm and baseurl are the same (#ref excluded)
        QTreeWidgetItem * item = new BookmarkItem( bm );
        ret.append( item );
    }
    return ret;
}

void BookmarkList::rebuildTree( bool filter )
{
    // disconnect and reconnect later, otherwise we'll get many itemChanged()
    // signals for all the current items
    disconnect(m_tree, &QTreeWidget::itemChanged, this, &BookmarkList::slotChanged);

    m_currentDocumentItem = nullptr;
    m_tree->clear();

    QList<QUrl> urls = m_document->bookmarkManager()->files();
    if ( filter )
    {
        if ( m_document->isOpened() )
        {
        foreach ( const QUrl& url, urls )
        {
            if ( url == m_document->currentDocument() )
            {
                m_tree->addTopLevelItems( createItems( url, m_document->bookmarkManager()->bookmarks( url ) ) );
                m_currentDocumentItem = m_tree->invisibleRootItem();
                break;
            }
        }
        }
    }
    else
    {
        QTreeWidgetItem * currenturlitem = nullptr;
        foreach ( const QUrl& url, urls )
        {
            QList<QTreeWidgetItem*> subitems = createItems( url, m_document->bookmarkManager()->bookmarks( url ) );
            if ( !subitems.isEmpty() )
            {
                FileItem * item = new FileItem( url, m_tree, m_document );
                item->addChildren( subitems );
                if ( !currenturlitem && url == m_document->currentDocument() )
                {
                    currenturlitem = item;
                }
            }
        }
        if ( currenturlitem )
        {
            currenturlitem->setExpanded( true );
            currenturlitem->setIcon( 0, QIcon::fromTheme( QStringLiteral("bookmarks") ) );
            m_tree->scrollToItem( currenturlitem, QAbstractItemView::PositionAtTop );
            m_currentDocumentItem = currenturlitem;
        }
    }

    m_tree->sortItems( 0, Qt::AscendingOrder );

    connect(m_tree, &QTreeWidget::itemChanged, this, &BookmarkList::slotChanged);
}

void BookmarkList::goTo( BookmarkItem * item )
{
    if ( item->url() == m_document->currentDocument() )
    {
        m_document->setViewport( item->viewport() );
    }
    else
    {
        Okular::GotoAction action( item->url().toDisplayString(QUrl::PreferLocalFile), item->viewport() );
        m_document->processAction( &action );
    }
}

void BookmarkList::selectiveUrlUpdate( const QUrl& url, QTreeWidgetItem*& item )
{
    disconnect(m_tree, &QTreeWidget::itemChanged, this, &BookmarkList::slotChanged);

    const KBookmark::List urlbookmarks = m_document->bookmarkManager()->bookmarks( url );
    if ( urlbookmarks.isEmpty() )
    {
        if ( item != m_tree->invisibleRootItem() )
        {
            m_tree->invisibleRootItem()->removeChild( item );
            item = nullptr;
        }
        else if ( item )
        {
            for ( int i = item->childCount(); i >= 0; --i )
            {
                item->removeChild( item->child( i ) );
            }
        }
    }
    else
    {
        bool fileitem_created = false;

        if ( item )
        {
            for ( int i = item->childCount() - 1; i >= 0; --i )
            {
                item->removeChild( item->child( i ) );
            }
        }
        else
        {
            item = new FileItem( url, m_tree, m_document );
            fileitem_created = true;
        }
        if ( m_document->isOpened() && url == m_document->currentDocument() )
        {
            item->setIcon( 0, QIcon::fromTheme( QStringLiteral("bookmarks") ) );
            item->setExpanded( true );
        }
        item->addChildren( createItems( url, urlbookmarks ) );

        if ( fileitem_created )
        {
            // we need to sort also the parent of the new file item,
            // so it can be properly shown in the correct place
            m_tree->invisibleRootItem()->sortChildren( 0, Qt::AscendingOrder );
        }
        item->sortChildren( 0, Qt::AscendingOrder );
    }

    connect(m_tree, &QTreeWidget::itemChanged, this, &BookmarkList::slotChanged);
}

QTreeWidgetItem* BookmarkList::itemForUrl( const QUrl& url ) const
{
    const int count = m_tree->topLevelItemCount();
    for ( int i = 0; i < count; ++i )
    {
        QTreeWidgetItem *item = m_tree->topLevelItem( i );
        const QUrl itemurl = item->data( 0, UrlRole ).value< QUrl >();
        if ( itemurl.isValid() && itemurl == url )
        {
            return item;
        }
    }
    return nullptr;
}

#include "moc_bookmarklist.cpp"
