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
class KPrinter;
class KPDFPage;
class KPDFLink;
class KPDFDocument;
class DocumentSynopsis;
class DocumentInfo;
class PixmapRequest;

/* Note: on contents generation and asyncronous queries.
 * Many observers may want to request data syncronously or asyncronously.
 * - Sync requests. These should be done in-place. Syncronous events in the
 *   queue have precedence on all the asyncronous ones.
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
    Q_OBJECT
    public:
        // load a document and fill up the pagesVector
        virtual bool loadDocument( const QString & fileName, QValueVector< KPDFPage* > & pagesVector ) = 0;

        // Document description and Table of contents
        virtual const DocumentInfo * documentInfo() { return 0L; }
        virtual const DocumentSynopsis * documentSynopsis() { return 0L; }

        // DRM handling
        enum Permissions { Modify = 1, Copy = 2, Print = 4, AddNotes = 8 };
        virtual bool allowed( int /*permisisons*/ ) { return true; }

        // generator core
        virtual bool print( KPrinter& /*printer*/ ) { return false; }
        virtual void requestPixmap( PixmapRequest * request, bool asyncronous ) = 0;
        virtual void requestTextPage( KPDFPage * page ) = 0;

        // check configuration and return true if something changed
        virtual bool reparseConfig() { return false; }

    signals:
        void contentsChanged( int id, int pageNumber );
};

/**
 * @short Describes a pixmap type request.
 */
struct PixmapRequest
{
    // public data fields
    int id;
    int pageNumber;
    int width;
    int height;
    // this field is set by the document before passing the
    // request to the generator
    KPDFPage * page;

    // public constructor: initialize data
    PixmapRequest( int rId, int n, int w, int h )
        : id( rId ), pageNumber( n ), width( w ), height( h ), page( 0 )  {};
};

#endif
