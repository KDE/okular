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

// the biggest id, useful for ignoring wrong id request
#define MAX_OBSERVER_ID 10

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
class DocumentObserver
{
    public:
        /**
         * Destroys the document observer.
         */
        virtual ~DocumentObserver() {};

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

        virtual void notifySetup( const QVector< Okular::Page * > &/*pages*/, bool /*documentChanged*/ ) {};
        virtual void notifyViewportChanged( bool /*smoothMove*/ ) {};
        virtual void notifyPageChanged( int /*pageNumber*/, int /*changedFlags*/ ) {};
        virtual void notifyContentsCleared( int /*changedFlags*/ ) {};
        virtual void notifyVisibleRectsChanged() {};

        // queries to observers
        virtual bool canUnloadPixmap( int /*pageNum*/ ) const { return true; }
};

}

#endif
