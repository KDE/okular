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
#define PAGEVIEW_PRELOAD_PRIO 3
#define THUMBNAILS_PRIO 2
#define THUMBNAILS_PRELOAD_PRIO 4
#define PRESENTATION_PRIO 0

class Page;
class VisiblePageRect;

/**
 * @short Base class for objects being notified when something changes.
 *
 * Inherit this class and call Document->addObserver( yourClass ) to get
 * notified of asynchronous events (new pixmap generated, or changed, etc..).
 */
class DocumentObserver
{
    public:
        virtual ~DocumentObserver() {};

        // you must give each observer a unique ID (used for notifications)
        virtual uint observerId() const = 0;

        // commands from the Document to all observers
        enum ChangedFlags { Pixmap = 1, Bookmark = 2, Highlights = 4, TextSelection = 8, Annotations = 16 };
        enum NotifyType { Setup = 1, Viewport = 2, Page = 4, Contents = 8, VisibleAreas = 16 };
        virtual void notifySetup( const QVector< Okular::Page * > & /*pages*/, bool /*documentChanged*/ ) {};
        virtual void notifyViewportChanged( bool /*smoothMove*/ ) {};
        virtual void notifyPageChanged( int /*pageNumber*/, int /*changedFlags*/ ) {};
        virtual void notifyContentsCleared( int /*changedFlags*/ ) {};
        virtual void notifyVisibleRectsChanged() {};

        // queries to observers
        virtual bool canUnloadPixmap( int /*pageNum*/ ) { return true; }
};

struct NotifyRequest
{
    DocumentObserver::NotifyType type;
    bool toggle;
    int page;
    int flags;
    QVector<VisiblePageRect*> rects;
    NotifyRequest (DocumentObserver::NotifyType t, bool to=false)
        : type(t), toggle(to), page(-1), flags(0) { ; };
    NotifyRequest (DocumentObserver::NotifyType t, int p, int f)
        : type(t), toggle(false), page(p), flags (f) { ; };
    NotifyRequest (DocumentObserver::NotifyType t, int p)
        : type(t), toggle(false), page(p), flags(0) { ; };
};

}

#endif
