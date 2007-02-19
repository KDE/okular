/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymański <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_CHMGENERATOR_H_
#define _OKULAR_CHMGENERATOR_H_ 

#include <okular/core/document.h>
#include <okular/core/generator.h>

#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtCore/QSize>

class CHMFile;
class KHTMLPart;

namespace Okular {
class TextPage;
}

namespace DOM {
class Node;
}

class CHMGenerator : public Okular::Generator
{
    Q_OBJECT
    public:
        CHMGenerator();
        bool loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector );
        bool closeDocument();

        const Okular::DocumentInfo * generateDocumentInfo();
        const Okular::DocumentSynopsis * generateDocumentSynopsis();
        const Okular::DocumentFonts * generateDocumentFonts();

        bool canGeneratePixmap() const;
        void generatePixmap( Okular::PixmapRequest * request );

        QVariant metaData( const QString & key, const QVariant & option ) const;

    public slots:
        void slotCompleted();

    protected:
        Okular::TextPage* textPage( Okular::Page *page );

    private:
        void additionalRequestData();
        void recursiveExploreNodes( DOM::Node node, Okular::TextPage *tp );
        void preparePageForSyncOperation( int zoom , const QString &url );
        Okular::DocumentSynopsis m_docSyn;
        CHMFile* m_file;
        KHTMLPart *m_syncGen;
        mutable QMutex syncLock;
        QString m_fileName;
        Okular::PixmapRequest* m_request;
        int m_pixmapRequestZoom;
        Okular::DocumentInfo* m_docInfo;
        QSet<int> m_textpageAddedList;
};

#endif
