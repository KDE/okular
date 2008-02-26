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
    QMap< int, DocumentObserver * >::const_iterator it = d->document->m_observers.begin(), end = d->document->m_observers.end();\
    for ( ; it != end ; ++ it ) { (*it)-> cmd ; } }

class OkularBookmarkAction : public KBookmarkAction
{
    public:
        OkularBookmarkAction( const Okular::DocumentViewport& vp, const KBookmark& bk, KBookmarkOwner* owner, QObject *parent )
            : KBookmarkAction( bk, owner, parent )
        {
            if ( vp.isValid() )
                setText( QString::number( vp.pageNumber + 1 ) + " - " + text() );
        }
};

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

        QHash<KUrl, KBookmarkGroup>::iterator bookmarkFind( const KUrl& url, bool doCreate );

        BookmarkManager * q;
        KUrl url;
        QSet<int> urlBookmarks;
        DocumentPrivate * document;
        QString file;
        KBookmarkManager * manager;
        QHash<KUrl, KBookmarkGroup> knownFiles;
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
}

BookmarkManager::~BookmarkManager()
{
    save();
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
    d->manager->save( false );
    emit const_cast<BookmarkManager*>( this )->saved();
}

QHash<KUrl, KBookmarkGroup>::iterator BookmarkManager::Private::bookmarkFind( const KUrl& url, bool doCreate )
{
    QHash<KUrl, KBookmarkGroup>::iterator it = knownFiles.find( url );
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
                it = knownFiles.insert( url, bm.toGroup() );
                found = true;
                break;
            }
        }
        if ( !found && doCreate )
        {
            // folder not found :(
            // then, in a single step create a new folder and add it in our cache :)
            QString purl = url.isLocalFile() ? url.path() : url.prettyUrl();
            it = knownFiles.insert( url, root.createNewFolder( purl ) );
        }
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

    QHash<KUrl, KBookmarkGroup>::iterator it = d->bookmarkFind( referurl, true );
    Q_ASSERT( it != d->knownFiles.end() );

    QString newtitle;
    if ( title.isEmpty() )
    {
        // if we have no title specified for the new bookmark, then give it the
        // name '#n' where n is the index of this bookmark among the ones of
        // its file
        int count = 0;
        for ( KBookmark bm = it.value().first(); !bm.isNull(); bm = it.value().next( bm ) )
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
    it.value().addBookmark( newtitle, newurl, QString() );
    foreachObserver( notifyPageChanged( vp.pageNumber, DocumentObserver::Bookmark ) );
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

    QHash<KUrl, KBookmarkGroup>::iterator it = d->bookmarkFind( referurl, false );
    if ( it == d->knownFiles.end() )
        return -1;

    it.value().deleteBookmark( bm );

    foreachObserver( notifyPageChanged( vp.pageNumber, DocumentObserver::Bookmark ) );

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
    return ret;
}

void BookmarkManager::setUrl( const KUrl& url )
{
    d->url = url;
    d->urlBookmarks.clear();
    QHash<KUrl, KBookmarkGroup>::iterator it = d->bookmarkFind( url, false );
    if ( it != d->knownFiles.end() )
    {
        for ( KBookmark bm = it.value().first(); !bm.isNull(); bm = it.value().next( bm ) )
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
    QHash<KUrl, KBookmarkGroup>::iterator it = d->bookmarkFind( d->url, true );
    Q_ASSERT( it != d->knownFiles.end() );

    bool found = false;
    bool added = false;
    for ( KBookmark bm = it.value().first(); !found && !bm.isNull(); bm = it.value().next( bm ) )
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
        it.value().addBookmark( QString::fromLatin1( "#" ) + QString::number( vp.pageNumber + 1 ), newurl, QString() );
        added = true;
    }
    return added;
}

bool BookmarkManager::removePageBookmark( int page )
{
    QHash<KUrl, KBookmarkGroup>::iterator it = d->bookmarkFind( d->url, false );
    if ( it == d->knownFiles.end() )
        return false;

    bool found = false;
    for ( KBookmark bm = it.value().first(); !found && !bm.isNull(); bm = it.value().next( bm ) )
    {
        if ( bm.isSeparator() || bm.isGroup() )
            continue;

        DocumentViewport vp( bm.url().htmlRef() );
        if ( vp.isValid() && vp.pageNumber == page )
        {
            found = true;
            it.value().deleteBookmark( bm );
            d->urlBookmarks.remove( page );
        }
    }
    return found;
}

bool BookmarkManager::isBookmarked( int page ) const
{
    return d->urlBookmarks.contains( page );
}

#include "bookmarkmanager.moc"
