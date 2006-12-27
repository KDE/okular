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

#include <QtCore/QStringList>

#include <kbookmark.h>
#include <kbookmarkmanager.h>

#include "okular_export.h"

class KUrl;

namespace Okular {

class Document;
class DocumentViewport;

class OKULAR_EXPORT BookmarkManager : public QObject, public KBookmarkOwner
{
    Q_OBJECT

    public:
        BookmarkManager( Document * document );
        virtual ~BookmarkManager();

        virtual QString currentUrl() const;
        virtual QString currentTitle() const;
        virtual bool addBookmarkEntry() const;
        virtual bool editBookmarkEntry() const;
        virtual void openBookmark( const KBookmark & bm, Qt::MouseButtons, Qt::KeyboardModifiers );

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
         * Adds a new bookmark for the @p referurl at the specified viewport @p vp,
         * with an optional @p title.
         *
         * If no @p title is specified, then \em #n will be used.
         */
        bool addBookmark( const KUrl& referurl, const Okular::DocumentViewport& vp, const QString& title = QString() );

        /**
         * Removes the bookmark @p bm for the @p referurl specified.
         */
        int removeBookmark( const KUrl& referurl, const KBookmark& bm );

    Q_SIGNALS:
        /**
         * The bookmark manager is requesting to open the specified @p url.
         */
        void openUrl( const KUrl& url );

    private:
        class Private;
        Private * const d;

        // private interface used by the Document
        friend class Document;
        void setUrl( const KUrl& url );
        bool setPageBookmark( int page );
        bool isPageBookmarked( int page ) const;

        Q_DISABLE_COPY( BookmarkManager )
};

}

#endif
