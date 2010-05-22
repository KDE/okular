/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "bookmarkmanager.h"

// qt/kde includes
#include <qhash.h>
#include <qset.h>
#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>

// local includes
#include "document_p.h"
#include "observer.h"

using namespace Okular;

#define foreachObserver( cmd ) {\
    QMap< int, DocumentObserver * >::const_iterator it = d->document->m_observers.constBegin(), end = d->document->m_observers.constEnd();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

#define foreachObserverD( cmd ) {\
    QMap< int, DocumentObserver * >::const_iterator it = document->m_observers.constBegin(), end = document->m_observers.constEnd();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

class OkularBookmarkAction : public KBookmarkAction
{
    public:
        OkularBookmarkAction( const Okular::DocumentViewport& vp, const KBookmark& bk, KBookmarkOwner* owner, QObject *parent )
            : KBookmarkAction( bk, owner, parent )
            , m_pageNumber( vp.pageNumber + 1 )
        {
            if ( vp.isValid() )
                setText( QString::number( vp.pageNumber + 1 ) + " - " + text() );
        }

        inline int pageNumber() const
        {
            return m_pageNumber;
        }

    private:
        const int m_pageNumber;
};

inline bool okularBookmarkActionLessThan( QAction * a1, QAction * a2 )
{
    return static_cast< OkularBookmarkAction * >( a1 )->pageNumber()
           < static_cast< OkularBookmarkAction * >( a2 )->pageNumber();
}

class BookmarkManager::Private : public KBookmarkOwner
{
    public:
        Private( BookmarkManager * qq )
            : KBookmarkOwner(), q( qq ), document( 0 ), manager( 0 )
        {
        }

        ~Private()
        {
            knownFiles.clear();
            // no need to delete the manager, it's automatically done by KBookmarkManager
            // delete manager;
        }

        virtual QString currentUrl() const;
        virtual QString currentTitle() const;
        virtual bool enableOption(BookmarkOption option) const;
        virtual void openBookmark( const KBookmark & bm, Qt::MouseButtons, Qt::KeyboardModifiers );

        QHash<KUrl, QString>::iterator bookmarkFind( const KUrl& url, bool doCreate, KBookmarkGroup *result  = 0);

        // slots
        void _o_changed( const QString & groupAddress, const QString & caller );

        BookmarkManager * q;
        KUrl url;
        QSet<int> urlBookmarks;
        DocumentPrivate * document;
        QString file;
        KBookmarkManager * manager;
        QHash<KUrl, QString> knownFiles;
};

BookmarkManager::BookmarkManager( DocumentPrivate * document )
    : QObject( document->m_parent ), d( new Private( this ) )
{
    setObjectName( "Okular::BookmarkManager" );

    d->document = document;

    d->file = KStandardDirs::locateLocal( "data", "okular/bookmarks.xml" );

    d->manager = KBookmarkManager::managerForFile( d->file, "okular" );
    d->manager->setEditorOptions( KGlobal::caption(), false );
    d->manager->setUpdate( true );
    connect( d->manager, SIGNAL( changed( const QString &, const QString & ) ),
             this, SLOT( _o_changed( const QString &, const QString & ) ) );
}

BookmarkManager::~BookmarkManager()
{
    delete d;
}

//BEGIN Reimplementations from KBookmarkOwner
QString BookmarkManager::Private::currentUrl() const
{
    return url.prettyUrl();
}

QString BookmarkManager::Private::currentTitle() const
{
    return url.isLocalFile() ? url.path() : url.prettyUrl();
}

bool BookmarkManager::Private::enableOption(BookmarkOption option) const
{
    Q_UNUSED( option )
    return false;
}

void BookmarkManager::Private::openBookmark( const KBookmark & bm, Qt::MouseButtons, Qt::KeyboardModifiers )
{
    emit q->openUrl( bm.url() );
}
//END Reimplementations from KBookmarkOwner

void BookmarkManager::Private::_o_changed( const QString & groupAddress, const QString & caller )
{
    Q_UNUSED( caller );
    if ( groupAddress.isEmpty() )
        return;

    KUrl referurl;
    // first, try to find the bookmark group whom change notification was just received
    QHash<KUrl, QString>::iterator it = knownFiles.begin(), itEnd = knownFiles.end();
    for ( ; it != itEnd; ++it )
    {
        if ( it.value() == groupAddress )
        {
            referurl = it.key();
            knownFiles.erase( it );
            break;
        }
    }
    if ( !referurl.isValid() )
    {
        const KBookmark bm = manager->findByAddress( groupAddress );
        // better be safe than sorry
        if ( bm.isNull() )
            return;
        Q_ASSERT( bm.isGroup() );
        referurl = KUrl( bm.fullText() );
    }
    Q_ASSERT( referurl.isValid() );
    emit q->bookmarksChanged( referurl );
    // case for the url representing the current document
    // (this might happen if the same document is open in another place;
    // in such case, make really sure to be in sync)
    if ( referurl == url )
    {
        // save the old bookmarks for the current url
        const QSet<int> oldUrlBookmarks = urlBookmarks;
        // set the same url again, so we reload the information we have about it
        q->setUrl( referurl );
        // then notify the observers about the changes in the bookmarks
        foreach ( int p, ( oldUrlBookmarks + urlBookmarks ) - ( oldUrlBookmarks & urlBookmarks ) )
        {
            foreachObserverD( notifyPageChanged( p, DocumentObserver::Bookmark ) );
        }
    }
    emit q->saved();
}

KUrl::List BookmarkManager::files() const
{
    KUrl::List ret;
    KBookmarkGroup group = d->manager->root();
    for ( KBookmark bm = group.first(); !bm.isNull(); bm = group.next( bm ) )
    {
        if ( bm.isSeparator() || !bm.isGroup() )
            continue;

        ret.append( KUrl( bm.fullText() ) );
    }
    return ret;
}

KBookmark::List BookmarkManager::bookmarks( const KUrl& url ) const
{
    KBookmark::List ret;
    KBookmarkGroup group = d->manager->root();
    for ( KBookmark bm = group.first(); !bm.isNull(); bm = group.next( bm ) )
    {
        if ( !bm.isGroup() || KUrl( bm.fullText() ) != url )
            continue;

        KBookmarkGroup group = bm.toGroup();
        for ( KBookmark b = group.first(); !b.isNull(); b = group.next( b ) )
        {
            if ( b.isSeparator() || b.isGroup() )
                continue;

            ret.append( b );
        }
        break;
    }
    return ret;
}

void BookmarkManager::save() const
{
    d->manager->emitChanged();
    emit const_cast<BookmarkManager*>( this )->saved();
}

QHash<KUrl, QString>::iterator BookmarkManager::Private::bookmarkFind( const KUrl& url, bool doCreate, KBookmarkGroup *result )
{
    QHash<KUrl, QString>::iterator it = knownFiles.find( url );
    if ( it == knownFiles.end() )
    {
        // if the url we want to add a new entry for is not in the hash of the
        // known files, then first try to find the file among the top-level
        // "folder" names
        bool found = false;
        KBookmarkGroup root = manager->root();
        for ( KBookmark bm = root.first(); !found && !bm.isNull(); bm = root.next( bm ) )
        {
            if ( bm.isSeparator() || !bm.isGroup() )
                continue;

            KUrl tmpurl( bm.fullText() );
            if ( tmpurl == url )
            {
                // got it! place it the hash of known files
                KBookmarkGroup bg = bm.toGroup();
                it = knownFiles.insert( url, bg.address() );
                found = true;
                if ( result )
                    *result = bg;
                break;
            }
        }
        if ( !found && doCreate )
        {
            // folder not found :(
            // then, in a single step create a new folder and add it in our cache :)
            QString purl = url.isLocalFile() ? url.path() : url.prettyUrl();
            KBookmarkGroup newbg = root.createNewFolder( purl );
            it = knownFiles.insert( url, newbg.address() );
            if ( result )
                *result = newbg;
        }
    }
    else if ( result )
    {
        const KBookmark bm = manager->findByAddress( it.value() );
        Q_ASSERT( bm.isGroup() );
        *result = bm.toGroup();
    }
    return it;
}

void BookmarkManager::addBookmark( int n )
{
    if ( n >= 0 && n < (int)d->document->m_pagesVector.count() )
    {
        if ( setPageBookmark( n ) )
            foreachObserver( notifyPageChanged( n, DocumentObserver::Bookmark ) );
    }
}

bool BookmarkManager::addBookmark( const KUrl& referurl, const Okular::DocumentViewport& vp, const QString& title )
{
    if ( !referurl.isValid() || !vp.isValid() )
        return false;

    KBookmarkGroup thebg;
    QHash<KUrl, QString>::iterator it = d->bookmarkFind( referurl, true, &thebg );
    Q_ASSERT( it != d->knownFiles.end() );

    QString newtitle;
    if ( title.isEmpty() )
    {
        // if we have no title specified for the new bookmark, then give it the
        // name '#n' where n is the index of this bookmark among the ones of
        // its file
        int count = 0;
        for ( KBookmark bm = thebg.first(); !bm.isNull(); bm = thebg.next( bm ) )
        {
            if ( !bm.isSeparator() && !bm.isGroup() )
                ++count;
        }
        newtitle = QString( "#%1" ).arg( count + 1 );
    }
    else
        newtitle = title;
    KUrl newurl = referurl;
    newurl.setHTMLRef( vp.toString() );
    thebg.addBookmark( newtitle, newurl, QString() );
    if ( referurl == d->document->m_url )
    {
        d->urlBookmarks.insert( vp.pageNumber );
        foreachObserver( notifyPageChanged( vp.pageNumber, DocumentObserver::Bookmark ) );
    }
    d->manager->emitChanged( thebg );
    return true;
}

void BookmarkManager::removeBookmark( int n )
{
    if ( n >= 0 && n < (int)d->document->m_pagesVector.count() )
    {
        if ( removePageBookmark( n ) )
            foreachObserver( notifyPageChanged( n, DocumentObserver::Bookmark ) );
    }
}

int BookmarkManager::removeBookmark( const KUrl& referurl, const KBookmark& bm )
{
    if ( !referurl.isValid() || bm.isNull() || bm.isGroup() || bm.isSeparator() )
        return -1;

    DocumentViewport vp( bm.url().htmlRef() );
    if ( !vp.isValid() )
        return -1;

    KBookmarkGroup thebg;
    QHash<KUrl, QString>::iterator it = d->bookmarkFind( referurl, false, &thebg );
    if ( it == d->knownFiles.end() )
        return -1;

    thebg.deleteBookmark( bm );
    if ( referurl == d->document->m_url )
    {
        d->urlBookmarks.remove( vp.pageNumber );
        foreachObserver( notifyPageChanged( vp.pageNumber, DocumentObserver::Bookmark ) );
    }
    d->manager->emitChanged( thebg );

    return vp.pageNumber;
}

QList< QAction * > BookmarkManager::actionsForUrl( const KUrl& url ) const
{
    QList< QAction * > ret;
    KBookmarkGroup group = d->manager->root();
    for ( KBookmark bm = group.first(); !bm.isNull(); bm = group.next( bm ) )
    {
        if ( !bm.isGroup() || KUrl( bm.fullText() ) != url )
            continue;

        KBookmarkGroup group = bm.toGroup();
        for ( KBookmark b = group.first(); !b.isNull(); b = group.next( b ) )
        {
            if ( b.isSeparator() || b.isGroup() )
                continue;

            ret.append( new OkularBookmarkAction( DocumentViewport( b.url().htmlRef() ), b, d, 0 ) );
        }
        break;
    }
    qSort( ret.begin(), ret.end(), okularBookmarkActionLessThan );
    return ret;
}

void BookmarkManager::setUrl( const KUrl& url )
{
    d->url = url;
    d->urlBookmarks.clear();
    KBookmarkGroup thebg;
    QHash<KUrl, QString>::iterator it = d->bookmarkFind( url, false, &thebg );
    if ( it != d->knownFiles.end() )
    {
        for ( KBookmark bm = thebg.first(); !bm.isNull(); bm = thebg.next( bm ) )
        {
            if ( bm.isSeparator() || bm.isGroup() )
                continue;

            DocumentViewport vp( bm.url().htmlRef() );
            if ( !vp.isValid() )
                continue;

            d->urlBookmarks.insert( vp.pageNumber );
        }
    }
}

bool BookmarkManager::setPageBookmark( int page )
{
    KBookmarkGroup thebg;
    QHash<KUrl, QString>::iterator it = d->bookmarkFind( d->url, true, &thebg );
    Q_ASSERT( it != d->knownFiles.end() );

    bool found = false;
    bool added = false;
    for ( KBookmark bm = thebg.first(); !found && !bm.isNull(); bm = thebg.next( bm ) )
    {
        if ( bm.isSeparator() || bm.isGroup() )
            continue;

        DocumentViewport vp( bm.url().htmlRef() );
        if ( vp.isValid() && vp.pageNumber == page )
            found = true;

    }
    if ( !found )
    {
        d->urlBookmarks.insert( page );
        DocumentViewport vp;
        vp.pageNumber = page;
        KUrl newurl = d->url;
        newurl.setHTMLRef( vp.toString() );
        thebg.addBookmark( QString::fromLatin1( "#" ) + QString::number( vp.pageNumber + 1 ), newurl, QString() );
        added = true;
        d->manager->emitChanged( thebg );
    }
    return added;
}

bool BookmarkManager::removePageBookmark( int page )
{
    KBookmarkGroup thebg;
    QHash<KUrl, QString>::iterator it = d->bookmarkFind( d->url, false, &thebg );
    if ( it == d->knownFiles.end() )
        return false;

    bool found = false;
    for ( KBookmark bm = thebg.first(); !found && !bm.isNull(); bm = thebg.next( bm ) )
    {
        if ( bm.isSeparator() || bm.isGroup() )
            continue;

        DocumentViewport vp( bm.url().htmlRef() );
        if ( vp.isValid() && vp.pageNumber == page )
        {
            found = true;
            thebg.deleteBookmark( bm );
            d->urlBookmarks.remove( page );
            d->manager->emitChanged( thebg );
        }
    }
    return found;
}

bool BookmarkManager::isBookmarked( int page ) const
{
    return d->urlBookmarks.contains( page );
}

#undef foreachObserver
#undef foreachObserverD

#include "bookmarkmanager.moc"
