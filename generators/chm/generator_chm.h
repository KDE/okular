/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymański <niedakh@gmail.com>             *
 *   Copyright (C) 2008 by Albert Astals Cid <aacid@kde.org>               * 
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_CHMGENERATOR_H_
#define _OKULAR_CHMGENERATOR_H_ 

#include <core/document.h>
#include <core/generator.h>

#include "lib/libchmfile.h"

#include <qbitarray.h>

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
    Q_INTERFACES( Okular::Generator )
    public:
        CHMGenerator( QObject *parent, const QVariantList &args );
        ~CHMGenerator();
        bool loadDocument( const QString & fileName, QVector< Okular::Page * > & pagesVector ) Q_DECL_OVERRIDE;

        Okular::DocumentInfo generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const Q_DECL_OVERRIDE;
        const Okular::DocumentSynopsis * generateDocumentSynopsis() Q_DECL_OVERRIDE;

        bool canGeneratePixmap() const Q_DECL_OVERRIDE;
        void generatePixmap( Okular::PixmapRequest * request ) Q_DECL_OVERRIDE;

        QVariant metaData( const QString & key, const QVariant & option ) const Q_DECL_OVERRIDE;

    public Q_SLOTS:
        void slotCompleted();

    protected:
        bool doCloseDocument() Q_DECL_OVERRIDE;
        Okular::TextPage* textPage( Okular::Page *page ) Q_DECL_OVERRIDE;

    private:
        void additionalRequestData();
        void recursiveExploreNodes( DOM::Node node, Okular::TextPage *tp );
        void preparePageForSyncOperation( int zoom , const QString &url );
        QMap<QString, int> m_urlPage;
        QVector<QString> m_pageUrl;
        Okular::DocumentSynopsis m_docSyn;
        LCHMFile* m_file;
        KHTMLPart *m_syncGen;
        QString m_fileName;
        QString m_chmUrl;
        Okular::PixmapRequest* m_request;
        int m_pixmapRequestZoom;
        QBitArray m_textpageAddedList;
        QBitArray m_rectsGenerated;
};

#endif
