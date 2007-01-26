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
#include <qsize.h>
#include <qmutex.h>
#include "dom/dom_node.h"
#define CHM_DATAREADY_ID 6990

class CHMFile;
class KHTMLPart;
class PixmapThreader;
class QCustomEvent;

namespace Okular {
class TextPage;
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

        QVariant metaData( const QString & key, const QVariant & option ) const;

    public slots:
        void slotCompleted();

    protected:
        bool canGeneratePixmap() const;
        void generatePixmap( Okular::PixmapRequest * request );
        void generateSyncTextPage( Okular::Page * page );

    private:
        void additionalRequestData ();
        void recursiveExploreNodes(DOM::Node node,Okular::TextPage *tp);
        void preparePageForSyncOperation( int zoom , const QString & url);
//         void customEvent( QCustomEvent * e );
        Okular::DocumentSynopsis m_docSyn;
        CHMFile* m_file;
        KHTMLPart *m_syncGen;
//         KHTMLPart *m_asyncGen;
//         QSize m_size;
        mutable QMutex syncLock;
        QString m_fileName;
//         QMutex asyncLock;
        QMutex pageLock;
//         friend class PixmapThreader;
//         PixmapThreader * px;
        // 0 is loading document
        // 1 is requesting pixmaps
        // -1 is internal error :)
        int m_state;
        Okular::PixmapRequest* m_request;
        int m_pixmapRequestZoom;
        volatile bool m_doneFlagSet;
        Okular::DocumentInfo* m_docInfo;
};

// for now impossible to use KHTMLPart outside the main app, maybe in KDE4
// no async therefore
/*
class PixmapThreader : public QObject, public QThread
{
    Q_OBJECT
    public:
        PixmapThreader (CHMGenerator* gen) : m_gen(gen) {;} ;
        QPixmap * takePixmap() { return m_pix; };
        void startGeneration(PixmapRequest * req) { m_req=req; start(); };
    private:
        void run();
        PixmapRequest* m_req;
        QString page;
        QPixmap * m_pix;
        CHMGenerator * m_gen;
};*/

#endif
