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

#ifndef _OKULAR_GSINTERNALDOC_H_
#define _OKULAR_GSINTERNALDOC_H_

#include <QMap>
#include <QPair>
#include <QPrinter>
#include <QStringList>

#include "interpreter.h"
#include "dscparse_adapter.h"
#include "dscparse.h"

namespace Okular {
    class DocumentInfo;
}

typedef QList<int> PageList;

//! @typedef a QPair which first element is the beginning of a block in the file while the second is the end
typedef QPair<unsigned long , unsigned long > PsPosition;

class GSInternalDocument
{
    public:
        enum Format{ PS, PDF };
        GSInternalDocument(const QString &fname, GSInternalDocument::Format f);
        ~GSInternalDocument();
        static QString pageSizeToString( QPrinter::PageSize pSize );
        static QStringList paperSizes();
        void scanDSC();
        const CDSCMEDIA* findMediaByName( const QString& mediaName ) const;

        QString getPaperSize( const QString& mediaName ) const;
        QString pageMedia( int pagenumber ) const;
        QString pageMedia() const;
        void setMedia(const QString & m) { m_overrideMedia=m; }

        CDSC_ORIENTATION_ENUM orientation() const;
        CDSC_ORIENTATION_ENUM orientation( int pagenumber ) const;

        void setOrientation(CDSC_ORIENTATION_ENUM ori) { m_overrideOrientation=ori; }
        void insertPageData (int n, PsPosition p) { pagesInternalData.insert(n,p); }

        FILE * file () { return m_internalFile; }
        PsPosition pagePos (int i) const { return pagesInternalData[i]; }
        const QString & fileName () const { return m_fileName ; }

        const KDSC* dsc () const { return m_dsc; }

        QSize computePageSize( const QString& mediaName ) const;
        KDSCBBOX boundingBox( int pageNo ) const;
        KDSCBBOX boundingBox() const;

        void setProlog( PsPosition p )   { m_prolog=p; }
        PsPosition prolog() const { return m_prolog ; }
        void setSetup( PsPosition p) { m_setup=p; }
        PsPosition setup() const { return m_setup; }

        Format format() const { return m_format; }
        const Okular::DocumentInfo * generateDocumentInfo();
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

        Okular::DocumentInfo* docInfo;
        FILE* m_internalFile; // TODO QFile
        KDSC* m_dsc;
        Format m_format;
        PsPosition m_prolog;
        PsPosition m_setup;
        QMap<int, PsPosition > pagesInternalData;
};
#endif
