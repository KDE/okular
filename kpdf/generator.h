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

#include <qvaluevector.h>
#include <qstring.h>
class KPrinter;
class KPDFPage;
class KPDFLink;
class KPDFDocument;
class DocumentSynopsis;
class DocumentInfo;

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
class Generator
{
    public:
        // load a document and fill up the pagesVector
        virtual bool loadDocument( const QString & fileName, QValueVector< KPDFPage* > & pagesVector ) = 0;

        // Document description
        virtual const DocumentInfo & documentInfo() = 0;

        // DRM handling
        enum Permissions { Modify = 1, Copy = 2, Print = 4, AddNotes = 8 };
        virtual bool allowed( int /*permisisons*/ ) { return true; }

        // perform actions (/request content generation)
        virtual bool print( KPrinter& printer ) = 0;
        virtual bool requestPixmap( int id, KPDFPage * page, int width, int height, bool syncronous = false ) = 0;
        virtual void requestTextPage( KPDFPage * page ) = 0;
        virtual DocumentSynopsis& synopsis() = 0;

        // check configuration and return if something changed
        virtual bool reparseConfig() { return false; }
};

#endif
