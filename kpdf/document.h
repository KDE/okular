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

#include <qvaluevector.h>

class KPrinter;
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
        virtual uint observerId() const = 0;

        // monitor changes in pixmaps (generation thread complete)
        virtual void notifyPixmapChanged( int /*pageNumber*/ ) {};
        virtual void notifyPixmapsCleared() {};

        // commands from the Document to all observers
        virtual void pageSetup( const QValueVector<KPDFPage*> & /*pages*/, bool /*documentChanged*/ ) {};
        virtual void pageSetCurrent( int /*pageNumber*/, const QRect & /*viewport*/ = QRect() ) {};
};

#define PART_ID 1
#define PAGEVIEW_ID 2
#define THUMBNAILS_ID 3
#define TOC_ID 4

/**
 * @short The information container. Actions (like open,find) take place here.
 *
 * xxxxxx
 * yyy.
 */
class KPDFDocument
{
    public:
        KPDFDocument();
        ~KPDFDocument();

        // document handling
        bool openDocument( const QString & docFile );
        void closeDocument();

        // misc methods
        void addObserver( KPDFDocumentObserver * pObserver );
        void reparseConfig();

        // query methods (const ones)
        uint currentPage() const;
        uint pages() const;
        QString author() const;
        QString creationDate() const;
        QString creator() const;
        bool encrypted() const;
        QString keywords() const;
        QString modificationDate() const;
        bool optimized() const;
        float PDFversion() const;
        QString producer() const;
        QString subject() const;
        QString title() const;
        bool okToPrint() const;
        Outline * outline() const;
        const KPDFPage * page( uint page ) const;

        // perform actions on document / pages
        void requestPixmap( int id, uint page, int width, int height, bool syncronous = false );
        void requestTextPage( uint page );
        void setCurrentPage( int page, const QRect & viewport = QRect() );
        void findText( const QString & text = "", bool caseSensitive = false );
        void findTextAll( const QString & pattern, bool caseSensitive );
        void toggleBookmark( int page );
        void processLink( const KPDFLink * link );
        bool print( KPrinter &printer );

    private:
        QString giveAbsolutePath( const QString & fileName );
        bool openRelativeFile( const QString & fileName );
        void processPageList( bool documentChanged );
        void unHilightPages();

        class KPDFDocumentPrivate * d;
};

#endif
