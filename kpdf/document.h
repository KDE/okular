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
 * Inherit this class and call KPDFDocument->addObserver( obsClass ) to get
 * notified of asyncronous events (a new thumbnail has arrived, a pixmap has
 * changed, and other events).
 */
class KPDFDocumentObserver
{
public:
    // monitor changes in pixmaps (generation thread complete)
    virtual void notifyThumbnailChanged( int /*pageNumber*/ ) {};
    virtual void notifyPixmapChanged( int /*pageNumber*/ ) {};

    // commands from the Document to observers
    virtual void pageSetup( const QValueList<int> & /*pages*/ ) {};
    virtual void pageSetCurrent( int /*pageNumber*/, float /*position*/ ) {};
    virtual void pageSetHilight( int /*x*/, int /*y*/, int /*width*/, int /*height*/ ) {};
};


/**
 * @short The information container. Actions (like find) take place here.
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

    // observers related methods
    void addObserver( KPDFDocumentObserver * pObserver );

    // document handling
    bool openFile( const QString & docFile );
    void close();

    // document control
    const KPDFPage * page( uint page ) const;
    uint pages() const;
    uint currentPage() const { return 0; }
    void setCurrentPage( uint page, float position = 0.0 );
    void find( bool nextMatch, const QString & text = "" );
    void goToLink( /* QString anchor */ );
    bool atBegin() {return false;}
    bool atEnd() {return false;}

    void setZoom( float zoom ) {};
    void zoom( float offset ) {};

signals:
    void pageChanged();

private:
    void sendFilteredPageList();
    void deletePages();

    class KPDFDocumentPrivate * d;
};


#endif
