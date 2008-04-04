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

#include "okular_export.h"

class QAction;
class KUrl;

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
class OKULAR_EXPORT BookmarkManager : public QObject
{
    Q_OBJECT

    public:
        virtual ~BookmarkManager();

        /**
         * Returns the list of documents with bookmarks.
         */
        KUrl::List files() const;
        /**
         * Returns the list of bookmarks for the specified @p url.
         */
        KBookmark::List bookmarks( const KUrl& url ) const;

        /**
         * Forces to save the list of bookmarks.
         */
        void save() const;

        /**
         * Adds a bookmark for the given @p page.
         */
        void addBookmark( int page );

        /**
         * Adds a new bookmark for the @p referurl at the specified viewport @p vp,
         * with an optional @p title.
         *
         * If no @p title is specified, then \em #n will be used.
         */
        bool addBookmark( const KUrl& referurl, const Okular::DocumentViewport& vp, const QString& title = QString() );

        /**
         * Remove a bookmark for the given @p page.
         */
        void removeBookmark( int page );

        /**
         * Removes the bookmark @p bm for the @p referurl specified.
         */
        int removeBookmark( const KUrl& referurl, const KBookmark& bm );

        /**
         * Returns whether the given @p page is bookmarked.
         */
        bool isBookmarked( int page ) const;

        /**
         * Returns a list of actions for the bookmarks of the specified @p url.
         *
         * @note the actions will have no parents, so you have to delete them
         * yourself
         */
        QList< QAction* > actionsForUrl( const KUrl& url ) const;

    Q_SIGNALS:
        /**
         * The bookmark manager is requesting to open the specified @p url.
         */
        void openUrl( const KUrl& url );

        /**
         * This signal is emitted whenever bookmarks have been saved.
         */
        void saved();

        /**
         * The bookmarks for specified @p url were changed.
         *
         * @since 0.7 (KDE 4.1)
         */
        void bookmarksChanged( const KUrl& url );

    private:
        class Private;
        Private * const d;
        friend class Private;

        // private interface used by the Document
        friend class Document;
        friend class DocumentPrivate;

        BookmarkManager( DocumentPrivate * document );

        void setUrl( const KUrl& url );
        bool setPageBookmark( int page );
        bool removePageBookmark( int page );

        Q_DISABLE_COPY( BookmarkManager )
};

}

#endif
