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

#include <qvaluevector.h>
#include <qrect.h>

/** IDs for observers. Globally defined here. **/
#define PRESENTATION_ID 1
#define PART_ID 2
#define PAGEVIEW_ID 3
#define THUMBNAILS_ID 4
#define TOC_ID 5
#define MINIBAR_ID 6

/** PRIORITIES for requests. Globally defined here. **/
#define PAGEVIEW_PRIO 1
#define PAGEVIEW_PRELOAD_PRIO 4
#define THUMBNAILS_PRIO 2
#define THUMBNAILS_PRELOAD_PRIO 5
#define PRESENTATION_PRIO 0
#define PRESENTATION_PRELOAD_PRIO 3

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
        // you must give each observer a unique ID (used for notifications)
        virtual uint observerId() const = 0;

        // commands from the Document to all observers
        enum ChangedFlags { Pixmap = 1, Bookmark = 2, Highlights = 4 };
        virtual void notifySetup( const QValueVector< KPDFPage * > & /*pages*/, bool /*documentChanged*/ ) {};
        virtual void notifyViewportChanged( bool /*smoothMove*/ ) {};
        virtual void notifyPageChanged( int /*pageNumber*/, int /*changedFlags*/ ) {};
        virtual void notifyContentsCleared( int /*changedFlags*/ ) {};

        // queries to observers
        virtual bool canUnloadPixmap( int /*pageNum*/ ) { return true; }
};

#endif
