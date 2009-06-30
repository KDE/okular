/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *   Copyright (C) 2004-2007 by Albert Astals Cid <aacid@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_DOCUMENT_P_H_
#define _OKULAR_DOCUMENT_P_H_

#include "document.h"

// qt/kde/system includes
#include <QtCore/QHash>
#include <QtCore/QLinkedList>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QPointer>

#include <kcomponentdata.h>
#include <kservicetypetrader.h>

// local includes
#include "fontinfo.h"
#include "generator.h"

class QEventLoop;
class QTimer;
class KTemporaryFile;

struct AllocatedPixmap;
struct ArchiveData;
struct RunningSearch;

namespace Okular {
class ConfigInterface;
class SaveInterface;
class Scripter;
class View;
}

struct GeneratorInfo
{
    GeneratorInfo( const KComponentData &_data )
        : generator( 0 ), data( _data ),
          config( 0 ), save( 0 ),
          configChecked( false ), saveChecked( false )
    {}

    Okular::Generator * generator;
    KComponentData data;
    QString catalogName;
    Okular::ConfigInterface * config;
    Okular::SaveInterface * save;
    bool configChecked : 1;
    bool saveChecked : 1;
};

namespace Okular {

class FontExtractionThread;

class DocumentPrivate
{
    public:
        DocumentPrivate( Document *parent )
          : m_parent( parent ),
            m_lastSearchID( -1 ),
            m_tempFile( 0 ),
            m_docSize( -1 ),
            m_allocatedPixmapsTotalMemory( 0 ),
            m_maxAllocatedTextPages( 0 ),
            m_warnedOutOfMemory( false ),
            m_rotation( Rotation0 ),
            m_exportCached( false ),
            m_bookmarkManager( 0 ),
            m_memCheckTimer( 0 ),
            m_saveBookmarksTimer( 0 ),
            m_generator( 0 ),
            m_generatorsLoaded( false ),
            m_closingLoop( 0 ),
            m_scripter( 0 ),
            m_archiveData( 0 ),
            m_fontsCached( false )
        {
            calculateMaxTextPages();
        }

        // private methods
        QString pagesSizeString() const;
        QString localizedSize(const QSizeF &size) const;
        void cleanupPixmapMemory( qulonglong bytesOffset = 0 );
        void calculateMaxTextPages();
        qulonglong getTotalMemory();
        qulonglong getFreeMemory();
        void loadDocumentInfo();
        void loadDocumentInfo( const QString &fileName );
        void loadViewsInfo( View *view, const QDomElement &e );
        void saveViewsInfo( View *view, QDomElement &e ) const;
        QString giveAbsolutePath( const QString & fileName ) const;
        bool openRelativeFile( const QString & fileName );
        Generator * loadGeneratorLibrary( const KService::Ptr &service );
        void loadAllGeneratorLibraries();
        void loadServiceList( const KService::List& offers );
        void unloadGenerator( const GeneratorInfo& info );
        void cacheExportFormats();
        void setRotationInternal( int r, bool notify );
        ConfigInterface* generatorConfig( GeneratorInfo& info );
        SaveInterface* generatorSave( GeneratorInfo& info );
        bool openDocumentInternal( const KService::Ptr& offer, bool isstdin, const QString& docFile, const QByteArray& filedata );
        bool savePageDocumentInfo( KTemporaryFile *infoFile, int what ) const;
        DocumentViewport nextDocumentViewport() const;

        // private slots
        void saveDocumentInfo() const;
        void slotTimedMemoryCheck();
        void sendGeneratorRequest();
        void rotationFinished( int page, Okular::Page *okularPage );
        void fontReadingProgress( int page );
        void fontReadingGotFont( const Okular::FontInfo& font );
        void slotGeneratorConfigChanged( const QString& );
        void refreshPixmaps( int );
        void _o_configChanged();
        void doContinueNextMatchSearch(void *pagesToNotifySet, void * match, int currentPage, int searchID, const QString & text, int caseSensitivity, bool moveViewport, const QColor & color, bool noDialogs, int donePages);
        void doContinuePrevMatchSearch(void *pagesToNotifySet, void * theMatch, int currentPage, int searchID, const QString & text, int theCaseSensitivity, bool moveViewport, const QColor & color, bool noDialogs, int donePages);
        void doContinueAllDocumentSearch(void *pagesToNotifySet, void *pageMatchesMap, int currentPage, int searchID, const QString & text, int caseSensitivity, const QColor & color);
        void doContinueGooglesDocumentSearch(void *pagesToNotifySet, void *pageMatchesMap, int currentPage, int searchID, const QStringList & words, int caseSensitivity, const QColor & color, bool matchAll);

        // generators stuff
        /**
         * This method is used by the generators to signal the finish of
         * the pixmap generation @p request.
         */
        void requestDone( PixmapRequest * request );
        void textGenerationDone( Page *page );
        /**
         * Sets the bounding box of the given @p page (in terms of upright orientation, i.e., Rotation0).
         */
        void setPageBoundingBox( int page, const NormalizedRect& boundingBox );
        /**
         * Request a particular metadata of the Document itself (ie, not something
         * depending on the document type/backend).
         */
        QVariant documentMetaData( const QString &key, const QVariant &option ) const;

        // member variables
        Document *m_parent;
        QPointer<QWidget> m_widget;

        // find descriptors, mapped by ID (we handle multiple searches)
        QMap< int, RunningSearch * > m_searches;
        int m_lastSearchID;
        bool m_searchCancelled;

        // needed because for remote documents docFileName is a local file and
        // we want the remote url when the document refers to relativeNames
        KUrl m_url;

        // cached stuff
        QString m_docFileName;
        QString m_xmlFileName;
        KTemporaryFile *m_tempFile;
        qint64 m_docSize;

        // viewport stuff
        QLinkedList< DocumentViewport > m_viewportHistory;
        QLinkedList< DocumentViewport >::iterator m_viewportIterator;
        DocumentViewport m_nextDocumentViewport; // see Link::Goto for an explanation
        QString m_nextDocumentDestination;

        // observers / requests / allocator stuff
        QMap< int, DocumentObserver * > m_observers;
        QLinkedList< PixmapRequest * > m_pixmapRequestsStack;
        QLinkedList< PixmapRequest * > m_executingPixmapRequests;
        QMutex m_pixmapRequestsMutex;
        QLinkedList< AllocatedPixmap * > m_allocatedPixmapsFifo;
        qulonglong m_allocatedPixmapsTotalMemory;
        QList< int > m_allocatedTextPagesFifo;
        int m_maxAllocatedTextPages;
        bool m_warnedOutOfMemory;

        // the rotation applied to the document
        Rotation m_rotation;

        // the current size of the pages (if available), and the cache of the
        // available page sizes
        PageSize m_pageSize;
        PageSize::List m_pageSizes;

        // cache of the export formats
        bool m_exportCached;
        ExportFormat::List m_exportFormats;
        ExportFormat m_exportToText;

        // our bookmark manager
        BookmarkManager *m_bookmarkManager;

        // timers (memory checking / info saver)
        QTimer *m_memCheckTimer;
        QTimer *m_saveBookmarksTimer;

        QHash<QString, GeneratorInfo> m_loadedGenerators;
        Generator * m_generator;
        QString m_generatorName;
        bool m_generatorsLoaded;
        QVector< Page * > m_pagesVector;
        QVector< VisiblePageRect * > m_pageRects;

        // cache of the mimetype we support
        QStringList m_supportedMimeTypes;

        QEventLoop *m_closingLoop;

        Scripter *m_scripter;

        ArchiveData *m_archiveData;

        QPointer< FontExtractionThread > m_fontThread;
        bool m_fontsCached;
        FontInfo::List m_fontsCache;

        QSet< View * > m_views;
};

}

#endif
