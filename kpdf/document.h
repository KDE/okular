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
#include <qstring.h>
#include <qdom.h>

class KPrinter;
class KPDFPage;
class KPDFLink;
class Generator;
class DocumentInfo;
class DocumentSynopsis;

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
 * @short The Document. Heart of everything. Actions take place here.
 *
 * The Document is the main object in KPDF. All views query the Document to
 * get data/properties or even for accessing pages (in a 'const' way).
 *
 * It is designed to keep it detached from the document type (pdf, ps, you
 * name it..) so whenever you want to get some data, it asks its internals
 * generator to do the job and return results in a format-indepedent way.
 *
 * Apart from the generator (the currently running one) the document stores
 * all the Pages ('KPDFPage' class) of the current document in a vector and
 * notifies all the registered DocumentObservers when some content changes.
 *
 * For a better understanding of hieracies @see README.internals.png
 * @see KPDFDocumentObserver, KPDFPage
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
        const DocumentInfo * documentInfo() const;
        const DocumentSynopsis * documentSynopsis() const;
        const KPDFPage * page( uint page ) const;
        uint currentPage() const;
        uint pages() const;
        bool okToPrint() const;

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

        Generator * generator;
        QString documentFileName;
        QValueVector< KPDFPage * > pages_vector;
        class KPDFDocumentPrivate * d;
};

/**
 * @short Metadata that describes the document.
 *
 * The Info structure can be filled in by generators to display metadata
 * about the currently opened file.
 * FUTURE: use a Dom tree so every generator can have different fields for
 * its metadata and renew the display widget to use the dynamic format.
 */
struct DocumentInfo
{
    QString author,
            creationDate,
            modificationDate,
            creator,
            keywords,
            producer,
            subject,
            title,
            format,
            formatVersion,
            encryption,
            optimization;
};

/**
 * @short A Dom tree that describes the Table of Contents.
 *
 * The Synopsis (TOC or Table Of Contents for friends) is represented via
 * a dom tree where each nod has an internal name (displayed in the listview)
 * and one or more attributes.
 *
 * To fill in a valid synopsis tree just add domElements where the tag name
 * is the screen name of the entry.
 *
 * The following attributes are valid [more may be added in future]:
 * - Page: The page to which the node refers.
 * - Position: The position inside the page, where 0 means top and 100 is
 *   the bottom.
 */
class DocumentSynopsis : public QDomDocument
{
    public:
        // void implementation, only subclassed for naming!
        DocumentSynopsis() : QDomDocument() {};
};

#endif
