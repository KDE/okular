/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_GENERATOR_H_
#define _KPDF_GENERATOR_H_

#include <qobject.h>
#include <qvaluevector.h>
#include <qstring.h>
#include "core/document.h"
class KListView;
class KPrinter;
class KPDFPage;
class KPDFLink;
class PixmapRequest;

/* Note: on contents generation and asyncronous queries.
 * Many observers may want to request data syncronously or asyncronously.
 * - Sync requests. These should be done in-place.
 * - Async request must be done in real background. That usually means a
 *   thread, such as QThread derived classes.
 * Once contents are available, they must be immediately stored in the
 * KPDFPage they refer to, and a signal is emitted as soon as storing
 * (even for sync or async queries) has been done.
 */

/**
 * @short [Abstract Class] The information generator.
 *
 * Most of class members are pure virtuals and they must be implemented to
 * provide a class that builds contents (graphics and text).
 *
 * Generation/query is requested by the 'KPDFDocument' class only, and that
 * class stores the resulting data into 'KPDFPage's. The data will then be
 * displayed by the GUI components (pageView, thumbnailList, etc..).
 */
class Generator : public QObject
{
    public:
        /** virtual methods to reimplement **/
        // load a document and fill up the pagesVector
        virtual bool loadDocument( const QString & fileName, QValueVector< KPDFPage * > & pagesVector ) = 0;

        // Document description and Table of contents
        virtual const DocumentInfo * generateDocumentInfo() { return 0L; }
        virtual const DocumentSynopsis * generateDocumentSynopsis() { return 0L; }

        // DRM handling
        virtual bool isAllowed( int /*Document::Permisison(s)*/ ) { return true; }

        // page contents generation
        virtual bool canGeneratePixmap() = 0;
        virtual void generatePixmap( PixmapRequest * request ) = 0;
        virtual void generateSyncTextPage( KPDFPage * page ) = 0;

        // capability querying
        virtual bool supportsSearching() const = 0;
        virtual bool hasFonts() const = 0;

        // font related
        virtual void putFontInfo(KListView *list) = 0;

        // print document using already configured kprinter
        virtual bool print( KPrinter& /*printer*/ ) { return false; }
        // access meta data of the generator
        virtual QString getMetaData( const QString &/*key*/, const QString &/*option*/ ) { return QString(); }
        // tell generator to re-parse configuration and return true if something changed
        virtual bool reparseConfig() { return false; }

        /** 'signals' to send events the KPDFDocument **/
        // tell the document that the job has been completed
        void signalRequestDone( PixmapRequest * request ) { m_document->requestDone( request ); }

        /** constructor: takes the Document as a parameter **/
        Generator( KPDFDocument * doc ) : m_document( doc ) {};

    protected:
        KPDFDocument * m_document;

    private:
        Generator();
};

/**
 * @short Describes a pixmap type request.
 */
struct PixmapRequest
{
    PixmapRequest( int rId, int n, int w, int h, int p, bool a = false )
        : id( rId ), pageNumber( n ), width( w ), height( h ),
        priority( p ), async( a ), page( 0 )  {};

    // observer id
    int id;
    // page number and size
    int pageNumber;
    int width;
    int height;
    // asyncronous request priority (less is better, 0 is max)
    int priority;
    // generate the pixmap in a thread and notify observer when done
    bool async;

    // this field is set by the Docuemnt prior passing the
    // request to the generator
    KPDFPage * page;
};

#endif
