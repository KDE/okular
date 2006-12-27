/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_DOCUMENTOBSERVER_H_
#define _OKULAR_DOCUMENTOBSERVER_H_

#include <QtCore/QRect>
#include <QtCore/QVector>

#include "okular_export.h"

namespace Okular {

/** IDs for observers. Globally defined here. **/
#define PRESENTATION_ID 1
#define PART_ID 2
#define PAGEVIEW_ID 3
#define THUMBNAILS_ID 4
#define TOC_ID 5
#define MINIBAR_ID 6
#define REVIEWS_ID 7
#define PROGRESSWIDGET_ID 8
#define PAGESIZELABEL_ID 9
#define BOOKMARKLIST_ID 10

// the biggest id, useful for ignoring wrong id request
#define MAX_OBSERVER_ID 11

/** PRIORITIES for requests. Globally defined here. **/
#define PAGEVIEW_PRIO 1
#define PAGEVIEW_PRELOAD_PRIO 4
#define THUMBNAILS_PRIO 2
#define THUMBNAILS_PRELOAD_PRIO 5
#define PRESENTATION_PRIO 0
#define PRESENTATION_PRELOAD_PRIO 3

class Page;

/**
 * @short Base class for objects being notified when something changes.
 *
 * Inherit this class and call Document->addObserver( yourClass ) to get
 * notified of asynchronous events (new pixmap generated, or changed, etc..).
 */
class OKULAR_EXPORT DocumentObserver
{
    public:
        /**
         * Destroys the document observer.
         */
        virtual ~DocumentObserver();

        /**
         * Must return an unique ID for each observer (used for notifications).
         */
        virtual uint observerId() const = 0;

        /**
         * Flags that can be sent from the document to all observers to
         * inform them about the type of object that has been changed.
         */
        enum ChangedFlags {
            Pixmap = 1,           ///< Pixmaps has been changed
            Bookmark = 2,         ///< Bookmarks has been changed
            Highlights = 4,       ///< Highlighting information has been changed
            TextSelection = 8,    ///< Text selection has been changed
            Annotations = 16      ///< Annotations has been changed
        };

        /**
         * Flags that can be sent from the document to all observers to
         * inform them about area that has been changed.
         */
        enum NotifyType {
            Setup = 1,            ///< The page is setup the first time
            Viewport = 2,         ///< The viewport has changed
            Page = 4,             ///< The page format (properties) has been changed
            Contents = 8,         ///< The page content has been changed
            VisibleAreas = 16     ///< The visible are changed
        };

        /**
         * This method is called whenever the document is initialized or reconstructed.
         *
         * @param pages The vector of pages of the document.
         * @param documentChanged If true, the document is reconstructed.
         */
        virtual void notifySetup( const QVector< Okular::Page * > &pages, bool documentChanged );

        /**
         * This method is called whenever the viewport has been changed.
         *
         * @param smoothMove If true, the move shall be animated.
         */
        virtual void notifyViewportChanged( bool smoothMove );

        /**
         * This method is called whenever the content on @p page described by the
         * passed @p flags has been changed.
         */
        virtual void notifyPageChanged( int page, int flags );

        /**
         * This method is called whenever the content described by the passed @p flags
         * has been cleared.
         */
        virtual void notifyContentsCleared( int flags );

        /**
         * This method is called whenever the visible rects have been changed.
         */
        virtual void notifyVisibleRectsChanged();

        /**
         * Returns whether the observer agrees that all pixmaps for the given
         * @p page can be unloaded to improve memory usage.
         *
         * Returns true per default.
         */
        virtual bool canUnloadPixmap( int page ) const;

    private:
        class Private;
        Private* d;
};

}

#endif
