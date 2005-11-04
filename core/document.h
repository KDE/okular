/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
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
#include <qdict.h>
#include <kmimetype.h>

class KPDFPage;
class KPDFLink;
class DocumentObserver;
class DocumentViewport;
class DocumentInfo;
class DocumentSynopsis;
class DocumentFonts;
class Generator;
class PixmapRequest;
class Annotation;
class KPrinter;
class KURL;
class KActionCollection;
class QToolBox;
class NotifyRequest;

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
        KPDFDocument( QDict<Generator> * genList );
        ~KPDFDocument();

        // communication with the part

        // document handling
        bool openDocument( const QString & docFile, const KURL & url, const KMimeType::Ptr &mime );
        void closeDocument();

        // observer stuff
        void addObserver( DocumentObserver * pObserver );
        void removeObserver( DocumentObserver * pObserver );
        void notifyObservers (NotifyRequest * request);
        void reparseConfig();

        // enum definitions
        enum Permission { AllowModify = 1, AllowCopy = 2, AllowPrint = 4, AllowNotes = 8 };

        // query methods (const ones)
        bool isOpened() const;
        const DocumentInfo * documentInfo() const;
        const DocumentSynopsis * documentSynopsis() const;
        const DocumentFonts * documentFonts() const;
        const KPDFPage * page( uint page ) const;
        const DocumentViewport & viewport() const;
        uint currentPage() const;
        uint pages() const;
        KURL currentDocument() const;
        bool isAllowed( int /*Document::Permisison(s)*/ ) const;
        bool supportsSearching() const;
        bool supportsRotation()  const;
// might be useful later
//	bool hasFonts() const;
        bool historyAtBegin() const;
        bool historyAtEnd() const;
        QString getMetaData( const QString & key, const QString & option = QString() ) const;

        // gui altering stuff
        QString getXMLFile();
        void setupGUI(KActionCollection  * ac , QToolBox * tBox );

        // perform actions on document / pages
        void setViewportPage( int page, int excludeId = -1, bool smoothMove = false );
        void setViewport( const DocumentViewport & viewport, int excludeId = -1, bool smoothMove = false );
        void setPrevViewport();
        void setNextViewport();
        void requestPixmaps( const QValueList< PixmapRequest * > & requests );
        void requestTextPage( uint page );
        void addPageAnnotation( int page, Annotation * annotation );

        enum SearchType { NextMatch, PrevMatch, AllDoc, GoogleAll, GoogleAny };
        bool searchText( int searchID, const QString & text, bool fromStart, bool caseSensitive,
                         SearchType type, bool moveViewport, const QColor & color, bool noDialogs = false );
        bool continueSearch( int searchID );
        void resetSearch( int searchID );

        void toggleBookmark( int page );
        void processLink( const KPDFLink * link );
        bool canConfigurePrinter() const;
        bool print( KPrinter &printer );
        bool handleEvent (QEvent * event);
        // notifications sent by generator
        void requestDone( PixmapRequest * request );
//         inline pagesVector() { return pages_vector; };

    public slots:
        void slotOrientation( int orientation );

    signals:
        void close();
        void quit();
        void linkFind();
        void linkGoToPage();
        void linkPresentation();
        void linkEndPresentation();
        void openURL(const KURL &url);
        void error(QString & string, int duration);
        void warning(QString & string, int duration);
        void notice(QString & string, int duration);

    private:
        // memory management related functions
        void cleanupPixmapMemory( int bytesOffset = 0 );
        int getTotalMemory();
        int getFreeMemory();
        // more private functions
        void loadDocumentInfo();
        QString giveAbsolutePath( const QString & fileName );
        bool openRelativeFile( const QString & fileName );
        QDict<Generator>* m_loadedGenerators ;
        Generator * generator;
        bool m_usingCachedGenerator;
        QValueVector< KPDFPage * > pages_vector;
        class KPDFDocumentPrivate * d;

    private slots:
        void saveDocumentInfo() const;
        void slotTimedMemoryCheck();
        void sendGeneratorRequest();
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
 * @short A DOM tree containing informations about the document.
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
 * @short A DOM tree that describes the Table of Contents.
 *
 * The Synopsis (TOC or Table Of Contents for friends) is represented via
 * a dom tree where each node has an internal name (displayed in the listview)
 * and one or more attributes.
 *
 * In the tree the tag name is the 'screen' name of the entry. A tag can have
 * attributes. Here follows the list of tag attributes with meaning:
 * - Icon: An icon to be set in the Lisview for the node
 * - Viewport: A string description of the referred viewport
 * - ViewportName: A 'named reference' to the viewport that must be converted
 *      using getMetaData( "NamedViewport", *viewport_name* )
 */
class DocumentSynopsis : public QDomDocument
{
    public:
        DocumentSynopsis();
};

/**
 * @short A DOM thee describing fonts used in document.
 *
 * Root's childrend (if any) are font nodes with the following attributes:
 * - Name
 * - Type
 * - Embedded (if font is shipped inside the document)
 * - File (system's file that provides this font
 */
class DocumentFonts : public QDomDocument
{
    public:
        DocumentFonts();
};

#endif
