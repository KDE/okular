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

#include "synctex/synctex_parser.h"

// qt/kde/system includes
#include <QtCore/QHash>
#include <QtCore/QLinkedList>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QPointer>
#include <QUrl>
#include <KPluginMetaData>

// local includes
#include "fontinfo.h"
#include "generator.h"

class QUndoStack;
class QEventLoop;
class QFile;
class QTimer;
class QTemporaryFile;
class KPluginMetaData;

struct AllocatedPixmap;
struct ArchiveData;
struct RunningSearch;

namespace Okular {
class ConfigInterface;
class PageController;
class SaveInterface;
class Scripter;
class View;
}

struct GeneratorInfo
{
    explicit GeneratorInfo( Okular::Generator *g, const KPluginMetaData &data)
        : generator( g ), metadata( data ), config( nullptr ), save( nullptr ),
          configChecked( false ), saveChecked( false )
    {}

    Okular::Generator * generator;
    KPluginMetaData metadata;
    Okular::ConfigInterface * config;
    Okular::SaveInterface * save;
    bool configChecked : 1;
    bool saveChecked : 1;
};

namespace Okular {

class FontExtractionThread;

struct DoContinueDirectionMatchSearchStruct
{
    QSet< int > *pagesToNotify;
    RegularAreaRect *match;
    int currentPage;
    int searchID;
};

enum LoadDocumentInfoFlag
{
    LoadNone = 0,
    LoadPageInfo = 1,    // Load annotations and forms
    LoadGeneralInfo = 2, // History, rotation, ...
    LoadAllInfo = 0xff
};
Q_DECLARE_FLAGS(LoadDocumentInfoFlags, LoadDocumentInfoFlag)

class DocumentPrivate
{
    public:
        DocumentPrivate( Document *parent )
          : m_parent( parent ),
            m_tempFile( nullptr ),
            m_docSize( -1 ),
            m_allocatedPixmapsTotalMemory( 0 ),
            m_maxAllocatedTextPages( 0 ),
            m_warnedOutOfMemory( false ),
            m_rotation( Rotation0 ),
            m_exportCached( false ),
            m_bookmarkManager( nullptr ),
            m_memCheckTimer( nullptr ),
            m_saveBookmarksTimer( nullptr ),
            m_generator( nullptr ),
            m_walletGenerator( nullptr ),
            m_generatorsLoaded( false ),
            m_pageController( nullptr ),
            m_closingLoop( nullptr ),
            m_scripter( nullptr ),
            m_archiveData( nullptr ),
            m_fontsCached( false ),
            m_annotationEditingEnabled ( true ),
            m_annotationBeingModified( false ),
            m_docdataMigrationNeeded( false ),
            m_synctex_scanner( nullptr )
        {
            calculateMaxTextPages();
        }

        // private methods
        bool updateMetadataXmlNameAndDocSize();
        QString pagesSizeString() const;
        QString namePaperSize(double inchesWidth, double inchesHeight) const;
        QString localizedSize(const QSizeF &size) const;
        qulonglong calculateMemoryToFree();
        void cleanupPixmapMemory();
        void cleanupPixmapMemory( qulonglong memoryToFree );
        AllocatedPixmap * searchLowestPriorityPixmap( bool unloadableOnly = false, bool thenRemoveIt = false, DocumentObserver *observer = nullptr /* any */ );
        void calculateMaxTextPages();
        qulonglong getTotalMemory();
        qulonglong getFreeMemory( qulonglong *freeSwap = nullptr );
        bool loadDocumentInfo( LoadDocumentInfoFlags loadWhat );
        bool loadDocumentInfo( QFile &infoFile, LoadDocumentInfoFlags loadWhat );
        void loadViewsInfo( View *view, const QDomElement &e );
        void saveViewsInfo( View *view, QDomElement &e ) const;
        QUrl giveAbsoluteUrl( const QString & fileName ) const;
        bool openRelativeFile( const QString & fileName );
        Generator * loadGeneratorLibrary( const KPluginMetaData& service );
        void loadAllGeneratorLibraries();
        void loadServiceList( const QVector<KPluginMetaData>& offers );
        void unloadGenerator( const GeneratorInfo& info );
        void cacheExportFormats();
        void setRotationInternal( int r, bool notify );
        ConfigInterface* generatorConfig( GeneratorInfo& info );
        SaveInterface* generatorSave( GeneratorInfo& info );
        Document::OpenResult openDocumentInternal( const KPluginMetaData& offer, bool isstdin, const QString& docFile, const QByteArray& filedata, const QString& password );
        static ArchiveData *unpackDocumentArchive( const QString &archivePath );
        bool savePageDocumentInfo( QTemporaryFile *infoFile, int what ) const;
        DocumentViewport nextDocumentViewport() const;
        void notifyAnnotationChanges( int page );
        void notifyFormChanges( int page );
        bool canAddAnnotationsNatively() const;
        bool canModifyExternalAnnotations() const;
        bool canRemoveExternalAnnotations() const;
        OKULARCORE_EXPORT static QString docDataFileName(const QUrl &url, qint64 document_size);

        // Methods that implement functionality needed by undo commands
        void performAddPageAnnotation( int page, Annotation *annotation );
        void performRemovePageAnnotation( int page, Annotation * annotation );
        void performModifyPageAnnotation( int page, Annotation * annotation, bool appearanceChanged );
        void performSetAnnotationContents( const QString & newContents, Annotation *annot, int pageNumber );

        void recalculateForms();

        // private slots
        void saveDocumentInfo() const;
        void slotTimedMemoryCheck();
        void sendGeneratorPixmapRequest();
        void rotationFinished( int page, Okular::Page *okularPage );
        void slotFontReadingProgress( int page );
        void fontReadingGotFont( const Okular::FontInfo& font );
        void slotGeneratorConfigChanged( const QString& );
        void refreshPixmaps( int );
        void _o_configChanged();
        void doContinueDirectionMatchSearch(void *doContinueDirectionMatchSearchStruct);
        void doContinueAllDocumentSearch(void *pagesToNotifySet, void *pageMatchesMap, int currentPage, int searchID);
        void doContinueGooglesDocumentSearch(void *pagesToNotifySet, void *pageMatchesMap, int currentPage, int searchID, const QStringList & words);

        void doProcessSearchMatch( RegularAreaRect *match, RunningSearch *search, QSet< int > *pagesToNotify, int currentPage, int searchID, bool moveViewport, const QColor & color );

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
        QVariant documentMetaData( const Generator::DocumentMetaDataKey key, const QVariant &option ) const;

        /**
         * Return whether the normalized rectangle @p rectOfInterest on page number @p rectPage
         * is fully visible.
         */
        bool isNormalizedRectangleFullyVisible( const Okular::NormalizedRect & rectOfInterest, int rectPage );

        // For sync files
        void loadSyncFile( const QString & filePath );

        // member variables
        Document *m_parent;
        QPointer<QWidget> m_widget;

        // find descriptors, mapped by ID (we handle multiple searches)
        QMap< int, RunningSearch * > m_searches;
        bool m_searchCancelled;

        // needed because for remote documents docFileName is a local file and
        // we want the remote url when the document refers to relativeNames
        QUrl m_url;

        // cached stuff
        QString m_docFileName;
        QString m_xmlFileName;
        QTemporaryFile *m_tempFile;
        qint64 m_docSize;

        // viewport stuff
        QLinkedList< DocumentViewport > m_viewportHistory;
        QLinkedList< DocumentViewport >::iterator m_viewportIterator;
        DocumentViewport m_nextDocumentViewport; // see Link::Goto for an explanation
        QString m_nextDocumentDestination;

        // observers / requests / allocator stuff
        QSet< DocumentObserver * > m_observers;
        QLinkedList< PixmapRequest * > m_pixmapRequestsStack;
        QLinkedList< PixmapRequest * > m_executingPixmapRequests;
        QMutex m_pixmapRequestsMutex;
        QLinkedList< AllocatedPixmap * > m_allocatedPixmaps;
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
        Generator * m_walletGenerator;
        bool m_generatorsLoaded;
        QVector< Page * > m_pagesVector;
        QVector< VisiblePageRect * > m_pageRects;

        // cache of the mimetype we support
        QStringList m_supportedMimeTypes;

        PageController *m_pageController;
        QEventLoop *m_closingLoop;

        Scripter *m_scripter;

        ArchiveData *m_archiveData;
        QString m_archivedFileName;

        QPointer< FontExtractionThread > m_fontThread;
        bool m_fontsCached;
        QSet<DocumentInfo::Key> m_documentInfoAskedKeys;
        DocumentInfo m_documentInfo;
        FontInfo::List m_fontsCache;

        QSet< View * > m_views;

        bool m_annotationEditingEnabled;
        bool m_annotationBeingModified; // is an annotation currently being moved or resized?
        bool m_metadataLoadingCompleted;

        QUndoStack *m_undoStack;
        QDomNode m_prevPropsOfAnnotBeingModified;

        // Since 0.21, we no longer support saving annotations and form data in
        // the docdata/ directory and we ask the user to migrate them to an
        // external file as soon as possible, otherwise the document will be
        // shown in read-only mode. This flag is set if the docdata/ XML file
        // for the current document contains any annotation or form.
        bool m_docdataMigrationNeeded;

        synctex_scanner_p m_synctex_scanner;

        // generator selection
        static QVector<KPluginMetaData> availableGenerators();
        static QVector<KPluginMetaData> configurableGenerators();
        static KPluginMetaData generatorForMimeType(const QMimeType& type, QWidget* widget, const QVector<KPluginMetaData> &triedOffers = QVector<KPluginMetaData>());
};

class DocumentInfoPrivate
{
    public:
        QMap<QString, QString> values; // key -> value
        QMap<QString, QString> titles; // key -> title For the custom keys
};

}

#endif

/* kate: replace-tabs on; indent-width 4; */
