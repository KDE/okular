/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_DOCUMENT_H_
#define _KPDF_DOCUMENT_H_

#include <qobject.h>
#include <qvaluelist.h>

class KPDFPage;

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
    virtual void pageSetup( const QValueList<int> & /*pages*/ ) {};
    virtual void pageSetCurrent( int /*pageNumber*/, float /*position*/ ) {};
    //virtual void pageSetHilight( int /*x*/, int /*y*/, int /*width*/, int /*height*/ ) {};
};

#define PAGEWIDGET_ID 1
#define THUMBNAILS_ID 2

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
    bool openFile( const QString & docFile );
    void close();

    // query methods
    uint currentPage() const;
    uint pages() const;
    bool atBegin() const;
    bool atEnd() const;
    const KPDFPage * page( uint page ) const;

    // observers related methods
    void addObserver( KPDFDocumentObserver * pObserver );
    void requestPixmap( int id, uint page, int width, int height, bool syncronous = false );

public slots:
    // document commands via slots
    void slotSetCurrentPage( int page );
    void slotSetCurrentPagePosition( int page, float position );
    void slotFind( bool nextMatch, const QString & text = "" );
    void slotGoToLink( /* QString anchor */ );

signals:
    // notify changes via signals
    void pageChanged();

private:
    void sendFilteredPageList( bool forceEmpty = false );
    void deletePages();

    class KPDFDocumentPrivate * d;
};


#endif
