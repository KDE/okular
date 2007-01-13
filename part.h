/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Andy Goossens <andygoossens@telenet.be>         *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _PART_H_
#define _PART_H_

#include <kparts/browserextension.h>
#include <kparts/part.h>
#include <qlist.h>
#include <qpointer.h>
#include "core/observer.h"
#include "core/document.h"
#include "kdocumentviewer.h"

#include <QtDBus/QtDBus>

class QAction;
class QWidget;
class QSplitter;
class QToolBox;

class KUrl;
class KAction;
class KConfig;
class KDirWatch;
class KToggleAction;
class KToggleFullScreenAction;
class KSelectAction;
class KAboutData;
class KPrinter;
class KTemporaryFile;

class ThumbnailList;
class ThumbnailController;
class PageSizeLabel;
class PageView;
class PageViewTopMessage;
class PresentationWidget;
class ProgressWidget;
class SearchWidget;
class TOC;
class MiniBar;

namespace Okular {
class Document;
class ExportFormat;
}

class BrowserExtension;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Wilco Greven <greven@kde.org>
 * @version 0.2
 */
class Part : public KParts::ReadOnlyPart, public Okular::DocumentObserver, public KDocumentViewer
{
Q_OBJECT
Q_CLASSINFO("D-Bus Interface", "org.kde.okular")
Q_INTERFACES(KDocumentViewer)

public:
	// Default constructor
	Part(QWidget* parentWidget, QObject* parent, const QStringList& args);

	// Destructor
	~Part();

    // inherited from DocumentObserver
    uint observerId() const { return PART_ID; }
    void notifyViewportChanged( bool smoothMove );

	static KAboutData* createAboutData();

	bool openDocument(const KUrl& url, uint page);
	void startPresentation();

public slots: // dbus
	Q_SCRIPTABLE Q_NOREPLY void goToPage(uint page);
	Q_SCRIPTABLE Q_NOREPLY void openDocument(KUrl doc);
	Q_SCRIPTABLE uint pages();
	Q_SCRIPTABLE uint currentPage();
	Q_SCRIPTABLE KUrl currentDocument();
	Q_SCRIPTABLE void slotPreferences();
	Q_SCRIPTABLE void slotFind();
	Q_SCRIPTABLE void slotPrintPreview();
	Q_SCRIPTABLE void slotPreviousPage();
	Q_SCRIPTABLE void slotNextPage();
	Q_SCRIPTABLE void slotGotoFirst();
	Q_SCRIPTABLE void slotGotoLast();

signals:
	void enablePrintAction(bool enable);

protected:
	// reimplemented from KParts::ReadOnlyPart
	bool openFile();
	bool openUrl(const KUrl &url);
	bool closeUrl();
	void supportedMimetypes();

protected slots:
	// connected to actions
	void openUrlFromDocument(const KUrl &url);
	void slotGoToPage();
	void slotHistoryBack();
	void slotHistoryNext();
	void slotPreviousBookmark();
	void slotNextBookmark();
	void slotFindNext();
	void slotSaveFileAs();
	void slotGetNewStuff();
	void slotNewConfig();
	void slotShowMenu(const Okular::Page *page, const QPoint &point);
	void slotShowProperties();
	void slotShowEmbeddedFiles();
	void slotShowLeftPanel();
	void slotShowPresentation();
	void slotHidePresentation();
	void slotExportAs(QAction *);
	bool slotImportPSFile();
	void close();
	void cannotQuit();
	void splitterMoved( int pos, int index );
	void setMimeTypes(KIO::Job *job);
	void saveSplitterSize();
	// can be connected to widget elements
	void updateViewActions();
	void enableTOC(bool enable);

public slots:
	// connected to Shell action (and browserExtension), not local one
	void slotPrint();
	void restoreDocument(KConfig* config);
	void saveDocumentRestoreInfo(KConfig* config);
	void slotFileDirty( const QString& );
	void slotDoFileDirty();
	void psTransformEnded();

private:
	void doPrint( KPrinter& printer );
    void fillGenerators();
        bool handleCompressed(KUrl & url, const QString &path, const KMimeType::Ptr mimetype);
 	KTemporaryFile *m_tempfile;
	
	// the document
	Okular::Document * m_document;
	QString m_temporaryLocalFile;

	// main widgets
	QSplitter *m_splitter;
	QWidget *m_leftPanel;
	QToolBox *m_toolBox;
	SearchWidget *m_searchWidget;
	PageViewTopMessage * m_topMessage;
	QPointer<ThumbnailList> m_thumbnailList;
	QPointer<PageView> m_pageView;
	QPointer<TOC> m_toc;
	QPointer<MiniBar> m_miniBar;
	QPointer<PresentationWidget> m_presentationWidget;
	QPointer<ProgressWidget> m_progressWidget;
	QPointer<PageSizeLabel> m_pageSizeLabel;

	// document watcher (and reloader) variables
	KDirWatch *m_watcher;
	QTimer *m_dirtyHandler;
	Okular::DocumentViewport m_viewportDirty;
	bool m_wasPresentationOpen;

	// Remember the search history
	QStringList m_searchHistory;

	// actions
	QAction *m_gotoPage;
	QAction *m_prevPage;
	QAction *m_nextPage;
	QAction *m_firstPage;
	QAction *m_lastPage;
	QAction *m_historyBack;
	QAction *m_historyNext;
	QAction *m_prevBookmark;
	QAction *m_nextBookmark;
	QAction *m_copy;
	QAction *m_find;
	QAction *m_findNext;
	QAction *m_saveAs;
	QAction *m_printPreview;
	QAction *m_showProperties;
	QAction *m_showEmbeddedFiles;
	QAction *m_exportAs;
	QAction *m_exportAsText;
	QAction *m_showPresentation;
	KToggleAction* m_showMenuBarAction;
	KToggleAction* m_showLeftPanel;
	KToggleFullScreenAction* m_showFullScreenAction;	

	bool m_actionsSearched;
	bool m_searchStarted;
	BrowserExtension *m_bExtension;

  // QHash: key is the name of the generator
  QHash<QString, Okular::Generator*> m_loadedGenerators;
  // list of names of the generators that have settings
  QStringList m_generatorsWithSettings;
  QStringList m_supportedMimeTypes;
  KSelectAction * m_confGens;
	QList<Okular::ExportFormat> m_exportFormats;
	bool m_cliPresentation;

private slots:
    void slotGeneratorPreferences();
};


class BrowserExtension : public KParts::BrowserExtension
{
Q_OBJECT

public:
	BrowserExtension(Part*);

public slots:
	// Automatically detected by the host.
	void print();
};

#endif

// vim:ts=2:sw=2:tw=78:et
