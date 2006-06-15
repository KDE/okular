/***************************************************************************
 *   Copyright (C) 1997-2005 the KGhostView authors. See file GV_AUTHORS.  *
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   Many portions of this file are based on kghostview's code             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_GSINTERNALDOC_H_
#define _KPDF_GSINTERNALDOC_H_

#include <errno.h>

#include <qlist.h>
#include <qmap.h>
#include <qprinter.h>

#include "interpreter.h"
#include "dscparse_adapter.h"
#include "dscparse.h"
#include "core/generator.h"

class KDSC;
class QString;
class QStringList;

typedef QList<int> PageList;

class GSInternalDocument
{
    public:
        typedef enum Format{ PS, PDF };
        GSInternalDocument(const QString &fname, GSInternalDocument::Format f);
        static QString pageSizeToString( QPrinter::PageSize pSize );
        static QStringList paperSizes();
        void scanDSC();
        const CDSCMEDIA* findMediaByName( const QString& mediaName ) const;

        QString getPaperSize( const QString& mediaName ) const;
        QString pageMedia( int pagenumber ) const;
        QString pageMedia() const;
        void setMedia(const QString & m) { m_overrideMedia=m; };

        CDSC_ORIENTATION_ENUM orientation() const;
        CDSC_ORIENTATION_ENUM orientation( int pagenumber ) const;

        void setOrientation(CDSC_ORIENTATION_ENUM ori) { m_overrideOrientation=ori; };
        void insertPageData (int n, GSInterpreterLib::Position p) { pagesInternalData.insert(n,p); };

        FILE * file () { return m_internalFile; };
        GSInterpreterLib::Position pagePos (int i) { return pagesInternalData[i]; };
        const QString & fileName () { return m_fileName ; };

        const KDSC* dsc () { return m_dsc; };

        QSize computePageSize( const QString& mediaName ) const;
        KDSCBBOX boundingBox( int pageNo ) const;
        KDSCBBOX boundingBox() const;

        void setProlog( GSInterpreterLib::Position p )   { m_prolog=p; };
        GSInterpreterLib::Position prolog() { return m_prolog ; }
        void setSetup( GSInterpreterLib::Position p) { m_setup=p; };
        GSInterpreterLib::Position setup() { return m_setup; }

        Format format() { return m_format; };
        const DocumentInfo * generateDocumentInfo();
        bool psCopyDoc( const QString& inputFile,
            const QString& outputFile, const PageList& pageList );
        bool savePages( const QString& saveFileName, const PageList& pageList );
   private:
        bool m_error;
        QString m_errorString;

        QString m_fallbackMedia;
        CDSC_ORIENTATION_ENUM m_overrideOrientation;
        QString m_overrideMedia;

        // document stuff
        QString m_fileName;
        QStringList m_mediaNames;

        DocumentInfo docInfo;
        FILE* m_internalFile;
        KDSC* m_dsc;
        Format m_format;
        GSInterpreterLib::Position m_prolog;
        GSInterpreterLib::Position m_setup;
        QMap<int, GSInterpreterLib::Position > pagesInternalData;
};
#endif
