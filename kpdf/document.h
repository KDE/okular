/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_DOCUMENT_H_
#define _KPDF_DOCUMENT_H_

#include <qobject.h>
#include <qvaluevector.h>

class Outline;

class KPDFPage;
class KPDFLink;

/**
 * @short Base class for objects being notified when something changes.
 *
 * Inherit this class and call KPDFDocument->addObserver( obsClass ) to get notified
 * of asyncronous events (a new pixmap has arrived, changed, etc... ).
 */
class KPDFDocumentObserver
{
public:
    // you must give each observer a unique ID (used for notifications)
    virtual uint observerId() = 0;

    // monitor changes in pixmaps (generation thread complete)
    virtual void notifyPixmapChanged( int /*pageNumber*/ ) {};

    // commands from the Document to all observers
    virtual void pageSetup( const QValueVector<KPDFPage*> & /*pages*/, bool /*documentChanged*/ ) {};
    virtual void pageSetCurrent( int /*pageNumber*/, const QRect & /*viewport*/ = QRect() ) {};
};

#define PAGEVIEW_ID 1
#define THUMBNAILS_ID 2
#define TOC_ID 3

/**
 * @short The information container. Actions (like open,find) take place here.
 *
 * xxxxxx
 * yyy.
 */
class KPDFDocument : public QObject
{
    Q_OBJECT

public:
    KPDFDocument();
    ~KPDFDocument();

    // document handling
    bool openDocument( const QString & docFile );
    void closeDocument();

    // query methods
    uint currentPage() const;
    uint pages() const;
    bool atBegin() const;
    bool atEnd() const;
    const KPDFPage * page( uint page ) const;
    Outline * outline() const;

    // observers related methods
    void addObserver( KPDFDocumentObserver * pObserver );
    void requestPixmap( int id, uint page, int width, int height, bool syncronous = false );
    void requestTextPage( uint page );

public slots:
    // document commands via slots
    void slotSetCurrentPage( int page );
    void slotSetCurrentPageViewport( int page, const QRect & viewport );
    void slotSetFilter( const QString & pattern, bool caseSensitive );
    void slotBookmarkPage( int page, bool enabled );
    void slotFind( const QString & text = "", bool caseSensitive = false );
    void slotProcessLink( const KPDFLink * link );

signals:
    // notify changes via signals
    void pageChanged();

private:
    bool openRelativeFile( const QString & fileName );
    void processPageList( bool documentChanged );
    void unHilightPages();

    class KPDFDocumentPrivate * d;
};


#endif
