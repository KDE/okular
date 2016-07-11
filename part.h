/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Andy Goossens <andygoossens@telenet.be>         *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004-2007 by Albert Astals Cid <aacid@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _PART_H_
#define _PART_H_

#include <kparts/readwritepart.h>
#include <kpluginfactory.h>
#include <kmessagewidget.h>
#include <qicon.h>
#include <qlist.h>
#include <qpointer.h>
#include <qprocess.h>
#include <QUrl>
#include <kio/job.h>
#include "core/observer.h"
#include "core/document.h"
#include "kdocumentviewer.h"
#include "interfaces/viewerinterface.h"

#include "okularpart_export.h"

#include <QtDBus/QtDBus>

class QAction;
class QWidget;
class QPrinter;
class QMenu;

class KConfigDialog;
class KConfigGroup;
class KDirWatch;
class KToggleAction;
class KToggleFullScreenAction;
class KSelectAction;
class KAboutData;
class QTemporaryFile;
class QAction;
namespace KParts { class GUIActivateEvent; }

class FindBar;
class ThumbnailList;
class PageSizeLabel;
class PageView;
class PresentationWidget;
class ProgressWidget;
class SearchWidget;
class Sidebar;
class TOC;
class MiniBar;
class MiniBarLogic;
class FileKeeper;
class Reviews;
class BookmarkList;
class DrawingToolActions;
class Layers;

namespace Okular
{

class BrowserExtension;
class ExportFormat;

/**
 * Describes the possible embedding modes of the part
 *
 * @since 0.14 (KDE 4.8)
 */
enum EmbedMode
{
    UnknownEmbedMode,
    NativeShellMode,         // embedded in the native Okular' shell
    PrintPreviewMode,        // embedded to show the print preview of a document
    KHTMLPartMode,           // embedded in KHTML
    ViewerWidgetMode         // the part acts as a widget that can display all kinds of documents
};

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Wilco Greven <greven@kde.org>
 * @version 0.2
 */
class OKULARPART_EXPORT Part : public KParts::ReadWritePart, public Okular::DocumentObserver, public KDocumentViewer, public Okular::ViewerInterface
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.okular")
    Q_INTERFACES(KDocumentViewer)
    Q_INTERFACES(Okular::ViewerInterface)
    
    friend class PartTest;

    public:
        // Default constructor
        /**
         * If one element of 'args' contains one of the strings "Print/Preview" or "ViewerWidget",
         * the part will be set up in the corresponding mode. Additionally, it is possible to specify
         * which config file should be used by adding a string containing "ConfigFileName=<file name>"
         * to 'args'.
         **/
        Part(QWidget* parentWidget, QObject* parent, const QVariantList& args);

        // Destructor
        ~Part();

        // inherited from DocumentObserver
        void notifySetup( const QVector< Okular::Page * > &pages, int setupFlags ) Q_DECL_OVERRIDE;
        void notifyViewportChanged( bool smoothMove ) Q_DECL_OVERRIDE;
        void notifyPageChanged( int page, int flags ) Q_DECL_OVERRIDE;

        bool openDocument(const QUrl &url, uint page) Q_DECL_OVERRIDE;
        void startPresentation() Q_DECL_OVERRIDE;
        QStringList supportedMimeTypes() const Q_DECL_OVERRIDE;

        QUrl realUrl() const;

        void showSourceLocation(const QString& fileName, int line, int column, bool showGraphically = true) Q_DECL_OVERRIDE;
        void clearLastShownSourceLocation() Q_DECL_OVERRIDE;
        bool isWatchFileModeEnabled() const Q_DECL_OVERRIDE;
        void setWatchFileModeEnabled(bool enable) Q_DECL_OVERRIDE;
        bool areSourceLocationsShownGraphically() const Q_DECL_OVERRIDE;
        void setShowSourceLocationsGraphically(bool show) Q_DECL_OVERRIDE;
        bool openNewFilesInTabs() const Q_DECL_OVERRIDE;

    public Q_SLOTS:                // dbus
        Q_SCRIPTABLE Q_NOREPLY void goToPage(uint page) Q_DECL_OVERRIDE;
        Q_SCRIPTABLE Q_NOREPLY void openDocument( const QString &doc );
        Q_SCRIPTABLE uint pages();
        Q_SCRIPTABLE uint currentPage();
        Q_SCRIPTABLE QString currentDocument();
        Q_SCRIPTABLE QString documentMetaData( const QString &metaData ) const;
        Q_SCRIPTABLE void slotPreferences();
        Q_SCRIPTABLE void slotFind();
        Q_SCRIPTABLE void slotPrintPreview();
        Q_SCRIPTABLE void slotPreviousPage();
        Q_SCRIPTABLE void slotNextPage();
        Q_SCRIPTABLE void slotGotoFirst();
        Q_SCRIPTABLE void slotGotoLast();
        Q_SCRIPTABLE void slotTogglePresentation();
        Q_SCRIPTABLE Q_NOREPLY void reload();
        Q_SCRIPTABLE Q_NOREPLY void enableStartWithPrint();

    Q_SIGNALS:
        void enablePrintAction(bool enable);
        void openSourceReference(const QString& absFileName, int line, int column);
        void viewerMenuStateChange(bool enabled);
        void enableCloseAction(bool enable);
        void mimeTypeChanged(QMimeType mimeType);
        void urlsDropped( const QList<QUrl>& urls );
        void fitWindowToPage( const QSize& pageViewPortSize, const QSize& pageSize );

    protected:
        // reimplemented from KParts::ReadWritePart
        bool openFile() Q_DECL_OVERRIDE;
        bool openUrl(const QUrl &url) Q_DECL_OVERRIDE;
        void guiActivateEvent(KParts::GUIActivateEvent *event) Q_DECL_OVERRIDE;
        void displayInfoMessage( const QString &message, KMessageWidget::MessageType messageType = KMessageWidget::Information, int duration = -1 );
    public:
        bool saveFile() Q_DECL_OVERRIDE;
        bool queryClose() Q_DECL_OVERRIDE;
        bool closeUrl() Q_DECL_OVERRIDE;
        bool closeUrl(bool promptToSave) Q_DECL_OVERRIDE;
        void setReadWrite(bool readwrite) Q_DECL_OVERRIDE;
        bool saveAs(const QUrl & saveUrl) Q_DECL_OVERRIDE;

    protected Q_SLOTS:
        // connected to actions
        void openUrlFromDocument(const QUrl &url);
        void openUrlFromBookmarks(const QUrl &url);
        void handleDroppedUrls( const QList<QUrl>& urls );
        void slotGoToPage();
        void slotHistoryBack();
        void slotHistoryNext();
        void slotAddBookmark();
        void slotRenameBookmarkFromMenu();
        void slotRenameCurrentViewportBookmark();
        void slotAboutToShowContextMenu(QMenu *menu, QAction *action, QMenu *contextMenu);
        void slotPreviousBookmark();
        void slotNextBookmark();
        void slotFindNext();
        void slotFindPrev();
        void slotSaveFileAs();
        void slotSaveCopyAs();
        void slotGetNewStuff();
        void slotNewConfig();
        void slotShowMenu(const Okular::Page *page, const QPoint &point);
        void slotShowProperties();
        void slotShowEmbeddedFiles();
        void slotShowLeftPanel();
        void slotShowBottomBar();
        void slotShowPresentation();
        void slotHidePresentation();
        void slotExportAs(QAction *);
        bool slotImportPSFile();
        void slotAboutBackend();
        void slotReload();
        void close();
        void cannotQuit();
        void slotShowFindBar();
        void slotHideFindBar();
        void slotJobStarted(KIO::Job *job);
        void slotJobFinished(KJob *job);
        void loadCancelled(const QString &reason);
        void setWindowTitleFromDocument();
        // can be connected to widget elements
        void updateViewActions();
        void updateBookmarksActions();
        void enableTOC(bool enable);
        void slotRebuildBookmarkMenu();
        void enableLayers( bool enable );

    public Q_SLOTS:
        // connected to Shell action (and browserExtension), not local one
        void slotPrint();
        void slotFileDirty( const QString& );
        void slotDoFileDirty();
        void psTransformEnded(int, QProcess::ExitStatus);
        KConfigDialog * slotGeneratorPreferences();

        void errorMessage( const QString &message, int duration = 0 );
        void warningMessage( const QString &message, int duration = -1 );
        void noticeMessage( const QString &message, int duration = -1 );

        void moveSplitter( const int sideWidgetSize );

    private:
        Document::OpenResult doOpenFile(const QMimeType &mime, const QString &fileNameToOpen, bool *isCompressedFile);

        void setupViewerActions();
        void setViewerShortcuts();
        void setupActions();

        void setupPrint( QPrinter &printer );
        void doPrint( QPrinter &printer );
        bool handleCompressed( QString &destpath, const QString &path, const QString &compressedMimetype );
        void rebuildBookmarkMenu( bool unplugActions = true );
        void updateAboutBackendAction();
        void unsetDummyMode();
        void slotRenameBookmark( const DocumentViewport &viewport );
        void resetStartArguments();
        
        static int numberOfParts;

        QTemporaryFile *m_tempfile;

        // the document
        Okular::Document * m_document;
        QString m_temporaryLocalFile;
        bool isDocumentArchive;

        // main widgets
        Sidebar *m_sidebar;
        SearchWidget *m_searchWidget;
        FindBar * m_findBar;
        KMessageWidget * m_topMessage;
        KMessageWidget * m_formsMessage;
        KMessageWidget * m_infoMessage;
        QPointer<ThumbnailList> m_thumbnailList;
        QPointer<PageView> m_pageView;
        QPointer<TOC> m_toc;
        QPointer<MiniBarLogic> m_miniBarLogic;
        QPointer<MiniBar> m_miniBar;
        QPointer<MiniBar> m_pageNumberTool;
        QPointer<QWidget> m_bottomBar;
        QPointer<PresentationWidget> m_presentationWidget;
        QPointer<ProgressWidget> m_progressWidget;
        QPointer<PageSizeLabel> m_pageSizeLabel;
        QPointer<Reviews> m_reviewsWidget;
        QPointer<BookmarkList> m_bookmarkList;
        QPointer<Layers> m_layers;

        // document watcher (and reloader) variables
        KDirWatch *m_watcher;
        QTimer *m_dirtyHandler;
        QUrl m_oldUrl;
        Okular::DocumentViewport m_viewportDirty;
        bool m_wasPresentationOpen;
        QWidget *m_dirtyToolboxItem;
        bool m_wasSidebarVisible;
        bool m_wasSidebarCollapsed;
        bool m_fileWasRemoved;
        Rotation m_dirtyPageRotation;

        // Remember the search history
        QStringList m_searchHistory;

        // actions
        QAction *m_gotoPage;
        QAction *m_prevPage;
        QAction *m_nextPage;
        QAction *m_beginningOfDocument;
        QAction *m_endOfDocument;
        QAction *m_historyBack;
        QAction *m_historyNext;
        QAction *m_addBookmark;
        QAction *m_renameBookmark;
        QAction *m_prevBookmark;
        QAction *m_nextBookmark;
        QAction *m_copy;
        QAction *m_selectAll;
        QAction *m_find;
        QAction *m_findNext;
        QAction *m_findPrev;
        QAction *m_saveAs;
        QAction *m_saveCopyAs;
        QAction *m_printPreview;
        QAction *m_showProperties;
        QAction *m_showEmbeddedFiles;
        QAction *m_exportAs;
        QAction *m_exportAsText;
        QAction *m_exportAsDocArchive;
        QAction *m_showPresentation;
        KToggleAction* m_showMenuBarAction;
        KToggleAction* m_showLeftPanel;
        KToggleAction* m_showBottomBar;
        KToggleFullScreenAction* m_showFullScreenAction;
        QAction *m_aboutBackend;
        QAction *m_reload;
        QMenu *m_exportAsMenu;
        QAction *m_closeFindBar;
        DrawingToolActions *m_presentationDrawingActions;

        bool m_actionsSearched;
        BrowserExtension *m_bExtension;

        QList<Okular::ExportFormat> m_exportFormats;
        QList<QAction*> m_bookmarkActions;
        bool m_cliPresentation;
        bool m_cliPrint;
        QString m_addBookmarkText;
        QIcon m_addBookmarkIcon;

        EmbedMode m_embedMode;

        QUrl m_realUrl;

        KXMLGUIClient *m_generatorGuiClient;
        FileKeeper *m_keeper;

        // Timer for m_infoMessage
        QTimer *m_infoTimer;

    private Q_SLOTS:
        void slotAnnotationPreferences();
        void slotHandleActivatedSourceReference(const QString& absFileName, int line, int col, bool *handled);
};

}

#endif

/* kate: replace-tabs on; indent-width 4; */
