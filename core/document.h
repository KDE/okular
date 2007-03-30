/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004-2005 by Albert Astals Cid <tsdgeos@terra.es>       *
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
#include <qstring.h>
#include <qdom.h>

#include <kmimetype.h>

class KPDFPage;
class KPDFLink;
class DocumentObserver;
class DocumentViewport;
class DocumentInfo;
class DocumentSynopsis;
class Generator;
class PixmapRequest;
class KListView;
class KPrinter;
class KURL;

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
 * @see DocumentObserver, KPDFPage
 */
class KPDFDocument : public QObject
{
    Q_OBJECT
    public:
        KPDFDocument( QWidget *widget );
        ~KPDFDocument();

        // document handling
        bool openDocument( const QString & docFile, const KURL & url, const KMimeType::Ptr &mime );
        void closeDocument();

        // misc methods
        void addObserver( DocumentObserver * pObserver );
        void removeObserver( DocumentObserver * pObserver );
        void reparseConfig();

        // enum definitions
        enum Permission { AllowModify = 1, AllowCopy = 2, AllowPrint = 4, AllowNotes = 8 };

        // returns the widget where the document is shown
        QWidget *widget() const;

        // query methods (const ones)
        bool isOpened() const;
        const DocumentInfo * documentInfo() const;
        const DocumentSynopsis * documentSynopsis() const;
        const KPDFPage * page( uint page ) const;
        const DocumentViewport & viewport() const;
        uint currentPage() const;
        uint pages() const;
        KURL currentDocument() const;
        bool isAllowed( int /*Document::Permisison(s)*/ ) const;
        bool historyAtBegin() const;
        bool historyAtEnd() const;
        QString getMetaData( const QString & key, const QString & option = QString() ) const;
        bool supportsSearching() const;
        bool hasFonts() const;
        void putFontInfo(KListView *list);

        // perform actions on document / pages
        void setViewportPage( int page, int excludeId = -1, bool smoothMove = false );
        void setViewport( const DocumentViewport & viewport, int excludeId = -1, bool smoothMove = false );
        void setPrevViewport();
        void setNextViewport();
        void setNextDocumentViewport( const DocumentViewport & viewport );
        void requestPixmaps( const QValueList< PixmapRequest * > & requests );
        void requestTextPage( uint page );

        enum SearchType { NextMatch, PrevMatch, AllDoc, GoogleAll, GoogleAny };
        bool searchText( int searchID, const QString & text, bool fromStart, bool caseSensitive,
                         SearchType type, bool moveViewport, const QColor & color, bool noDialogs = false );
        bool continueSearch( int searchID );
        void resetSearch( int searchID );
        bool continueLastSearch();
        void toggleBookmark( int page );
        void processLink( const KPDFLink * link );
        bool print( KPrinter &printer );

        // notifications sent by generator
        void requestDone( PixmapRequest * request );

    signals:
        void close();
        void quit();
        void linkFind();
        void linkGoToPage();
        void openURL(const KURL &url);
        void linkPresentation();
        void linkEndPresentation();

    private:
        void sendGeneratorRequest();
        // memory management related functions
        void cleanupPixmapMemory( int bytesOffset = 0 );
        int getTotalMemory();
        int getFreeMemory();
        // more private functions
        void loadDocumentInfo();
        QString giveAbsolutePath( const QString & fileName );
        bool openRelativeFile( const QString & fileName );

        Generator * generator;
        QValueVector< KPDFPage * > pages_vector;
        class KPDFDocumentPrivate * d;

    private slots:
        void saveDocumentInfo() const;
        void slotTimedMemoryCheck();
};


/**
 * @short A view on the document.
 *
 * The Viewport structure is the 'current view' over the document. Contained
 * data is broadcasted between observers to syncronize their viewports to get
 * the 'I scroll one view and others scroll too' views.
 */
class DocumentViewport
{
    public:
        /** data fields **/
        // the page nearest the center of the viewport
        int pageNumber;

        // enum definitions
        enum Position { Center = 1, TopLeft = 2};

        // if reCenter.enabled, this contains the viewport center
        struct {
            bool enabled;
            double normalizedX;
            double normalizedY;
            Position pos;
        } rePos;

        // if autoFit.enabled, page must be autofitted in the viewport
        struct {
            bool enabled;
            bool width;
            bool height;
        } autoFit;

        /** class methods **/
        // allowed constructors, don't use others
        DocumentViewport( int pageNumber = -1 );
        DocumentViewport( const QString & xmlDesc );
        QString toString() const;
        bool operator==( const DocumentViewport & vp ) const;
};

/**
 * @short A dom tree containing informations about the document.
 *
 * The Info structure can be filled in by generators to display metadata
 * about the currently opened file.
 */
class DocumentInfo : public QDomDocument
{
    public:
        DocumentInfo();

        /**
         * Sets a value for a special key. The title should be an i18n'ed
         * string, since it's used in the document information dialog.
         */
        void set( const QString &key, const QString &value,
                  const QString &title = QString() );

        /**
         * Returns the value for a given key or an empty string when the
         * key doesn't exist.
         */
        QString get( const QString &key ) const;
};

/**
 * @short A Dom tree that describes the Table of Contents.
 *
 * The Synopsis (TOC or Table Of Contents for friends) is represented via
 * a dom tree where each nod has an internal name (displayed in the listview)
 * and one or more attributes.
 *
 * In the tree the tag name is the 'screen' name of the entry. A tag can have
 * attributes. Here follows the list of tag attributes with meaning:
 * - Viewport: A string description of the referred viewport
 * - ViewportName: A 'named reference' to the viewport that must be converted
 *      using getMetaData( "NamedViewport", *viewport_name* )
 */
class DocumentSynopsis : public QDomDocument
{
    public:
        DocumentSynopsis();
};

#endif
