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

/**
 * @short Base class for objects being notified when something changes.
 *
 * Inherit this class and call KPDFDocument->addObserver( yourClass ) to get
 * notified of asyncronous events (new pixmap generated, or changed, etc..).
 */
class KPDFDocumentObserver
{
    public:
        // you must give each observer a unique ID (used for notifications)
        virtual uint observerId() const = 0;

        // commands from the Document to all observers
        virtual void pageSetup( const QValueVector< class KPDFPage * > & /*pages*/, bool /*documentChanged*/ ) {};
        virtual void pageSetCurrent( int /*pageNumber*/, const QRect & /*viewport*/ = QRect() ) {};

        // queries to observers
        virtual bool canUnloadPixmap( int /*pageNum*/ ) { return true; }

        // monitor changes in pixmaps (generation thread complete)
        virtual void notifyPixmapChanged( int /*pageNumber*/ ) {};
        virtual void notifyPixmapsCleared() {};
};

#endif
