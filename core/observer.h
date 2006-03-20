/***************************************************************************
 *   Copyright (C) 2005 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2005 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_DOCUMENTOBSERVER_H_
#define _KPDF_DOCUMENTOBSERVER_H_

#include <qvector.h>
#include <qrect.h>

/** IDs for observers. Globally defined here. **/
#define PRESENTATION_ID 1
#define PART_ID 2
#define PAGEVIEW_ID 3
#define THUMBNAILS_ID 4
#define TOC_ID 5
#define MINIBAR_ID 6
#define REVIEWS_ID 7

// the biggest ide, useful for ignoring wrong id request
#define MAX_OBSERVER_ID 8
/** PRIORITIES for requests. Globally defined here. **/
#define PAGEVIEW_PRIO 1
#define PAGEVIEW_PRELOAD_PRIO 3
#define THUMBNAILS_PRIO 2
#define THUMBNAILS_PRELOAD_PRIO 4
#define PRESENTATION_PRIO 0

class KPDFPage;

/**
 * @short Base class for objects being notified when something changes.
 *
 * Inherit this class and call KPDFDocument->addObserver( yourClass ) to get
 * notified of asyncronous events (new pixmap generated, or changed, etc..).
 */
class DocumentObserver
{
    public:
        virtual ~DocumentObserver() {};

        // you must give each observer a unique ID (used for notifications)
        virtual uint observerId() const = 0;

        // commands from the Document to all observers
        enum ChangedFlags { Pixmap = 1, Bookmark = 2, Highlights = 4, Annotations = 8 };
        enum NotifyType { Setup = 1, Viewport = 2, Page = 4, Contents = 8 };
        virtual void notifySetup( const QVector< KPDFPage * > & /*pages*/, bool /*documentChanged*/ ) {};
        virtual void notifyViewportChanged( bool /*smoothMove*/ ) {};
        virtual void notifyPageChanged( int /*pageNumber*/, int /*changedFlags*/ ) {};
        virtual void notifyContentsCleared( int /*changedFlags*/ ) {};

        // queries to observers
        virtual bool canUnloadPixmap( int /*pageNum*/ ) { return true; }
};

struct NotifyRequest
{
    DocumentObserver::NotifyType type;
    bool toggle;
    int page;
    int flags;
    NotifyRequest (DocumentObserver::NotifyType t, bool to=false)
        : type(t), toggle(to) { ; };
    NotifyRequest (DocumentObserver::NotifyType t, int p, int f)
        : type(t), page(p), flags (f) { ; };
    NotifyRequest (DocumentObserver::NotifyType t, int p)
        : type(t), page(p) { ; };
};

#endif
