/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_BOOKMARK_MANAGER_H_
#define _OKULAR_BOOKMARK_MANAGER_H_

#include <kbookmark.h>

#include "okularcore_export.h"
#include <QObject>
#include <QUrl>

class QAction;

namespace Okular {

class Document;
class DocumentPrivate;
class DocumentViewport;

/**
 * @brief Bookmarks manager utility.
 *
 * This class is responsible for loading and saving the bookmarks using the
 * proper format, and for working with them (eg querying, adding, removing).
 */
class OKULARCORE_EXPORT BookmarkManager : public QObject
{
    Q_OBJECT

    public:
        virtual ~BookmarkManager();

        /**
         * Returns the list of documents with bookmarks.
         */
        QList<QUrl> files() const;

        /**
         * Returns the list of bookmarks for the specified @p url.
         */
        KBookmark::List bookmarks( const QUrl& url ) const;

        /**
         * Returns the list of bookmarks for document
         * @since 0.14 (KDE 4.8)
         */
        KBookmark::List bookmarks() const;

        /**
         * Returns the list of bookmarks for the given page of the document
         * @since 0.15 (KDE 4.9)
         */
        KBookmark::List bookmarks( int page ) const;

        /**
         * Returns the bookmark for the given page of the document
         * @since 0.14 (KDE 4.8)
         */
        KBookmark bookmark( int page ) const;

        /**
         * Returns the bookmark for the given @p viewport of the document
         * @since 0.15 (KDE 4.9)
         */
        KBookmark bookmark( const DocumentViewport &viewport ) const;

        /**
         * Forces to save the list of bookmarks.
         */
        void save() const;

        /**
         * Adds a bookmark for the given @p page.
         */
        void addBookmark( int page );

        /**
         * Adds a bookmark for the given viewport @p vp
         * @since 0.15 (KDE 4.9)
         */
        void addBookmark( const DocumentViewport &vp );

        /**
         * Adds a new bookmark for the @p referurl at the specified viewport @p vp,
         * with an optional @p title.
         *
         * If no @p title is specified, then \em \#n will be used.
         */
        bool addBookmark( const QUrl& referurl, const Okular::DocumentViewport& vp, const QString& title = QString() );

        /**
         * Remove a bookmark for the given @p page.
         */
        void removeBookmark( int page );

        /**
         * Remove a bookmark for the given viewport @p vp
         * @since 0.15 (KDE 4.9)
         */
        void removeBookmark( const DocumentViewport &vp );

        /**
         * Removes the bookmark @p bm for the @p referurl specified.
         */
        int removeBookmark( const QUrl& referurl, const KBookmark& bm );

        /**
         * Removes the bookmarks in @p list for the @p referurl specified.
         *
         * @note it will remove only the bookmarks which belong to @p referurl
         *
         * @since 0.11 (KDE 4.5)
         */
        void removeBookmarks( const QUrl& referurl, const KBookmark::List& list );

        /**
         * Returns the bookmark given bookmark of the document
         * @since 0.14 (KDE 4.8)
         */
        void renameBookmark( KBookmark* bm, const QString& newName );

        /**
         * Renames the top-level bookmark for the @p referurl specified with
         * the @p newName specified.
         * @since 0.15 (KDE 4.9)
         */
        void renameBookmark( const QUrl& referurl, const QString& newName );

        /**
         * Returns title for the @p referurl
         * @since 0.15 (KDE 4.9)
         */
        QString titleForUrl( const QUrl& referurl ) const;

        /**
         * Returns whether the given @p page is bookmarked.
         */
        bool isBookmarked( int page ) const;

        /**
         * Return whether the given @p viewport is bookmarked.
         * @since 0.15 (KDE 4.9)
         */
        bool isBookmarked( const DocumentViewport &viewport ) const;

        /**
         * Given a @p viewport, returns the next bookmark
         * @since 0.15 (KDE 4.9)
         */
        KBookmark nextBookmark( const DocumentViewport &viewport ) const;

        /**
         * Given a @p viewport, returns the previous bookmark
         * @since 0.15 (KDE 4.9)
         */
        KBookmark previousBookmark( const DocumentViewport &viewport ) const;

        /**
         * Returns a list of actions for the bookmarks of the specified @p url.
         *
         * @note the actions will have no parents, so you have to delete them
         * yourself
         */
        QList< QAction* > actionsForUrl( const QUrl& url ) const;

    Q_SIGNALS:
        /**
         * The bookmark manager is requesting to open the specified @p url.
         */
        void openUrl( const QUrl& url );

        /**
         * This signal is emitted whenever bookmarks have been saved.
         */
        void saved();

        /**
         * The bookmarks for specified @p url were changed.
         *
         * @since 0.7 (KDE 4.1)
         */
        void bookmarksChanged( const QUrl& url );

    private:
        class Private;
        Private * const d;
        friend class Private;

        // private interface used by the Document
        friend class Document;
        friend class DocumentPrivate;

        BookmarkManager( DocumentPrivate * document );

        void setUrl( const QUrl& url );
        bool setPageBookmark( int page );
        bool removePageBookmark( int page );

        Q_DISABLE_COPY( BookmarkManager )

        Q_PRIVATE_SLOT( d, void _o_changed( const QString &, const QString & ) )
};

}

#endif
