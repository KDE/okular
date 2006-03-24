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

#ifndef _KPDF_PART_H_
#define _KPDF_PART_H_

#include <kparts/browserextension.h>
#include <kparts/part.h>
#include <qpointer.h>
#include "core/observer.h"
#include "core/document.h"
#include "dcop.h"

class QWidget;
class QSplitter;
class QToolBox;

class KUrl;
class KAction;
class KConfig;
class KDirWatch;
class KToggleAction;
class KSelectAction;
class KAboutData;
class KPrinter;

class KPDFDocument;
class ThumbnailList;
class ThumbnailController;
class PageView;
class PresentationWidget;
class SearchWidget;
class TOC;
class MiniBar;

namespace oKular {

class BrowserExtension;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Wilco Greven <greven@kde.org>
 * @version 0.2
 */
class Part : public KParts::ReadOnlyPart, public DocumentObserver, virtual public okular_dcop
{
Q_OBJECT

public:
	// Default constructor
	Part(QWidget* parentWidget, const char* widgetName,
	     QObject* parent, const char* name, const QStringList& args);

	// Destructor
	~Part();

    // inherited from DocumentObserver
    uint observerId() const { return PART_ID; }
    void notifyViewportChanged( bool smoothMove );

	static KAboutData* createAboutData();

	ASYNC goToPage(uint page);
	ASYNC openDocument(KUrl doc);
	uint pages();
	uint currentPage();
	KUrl currentDocument();

signals:
	void enablePrintAction(bool enable);

protected:
	// reimplemented from KParts::ReadOnlyPart
	bool openFile();
	bool openURL(const KUrl &url);
	bool closeURL();
	// filter that watches for splitter size changes
	bool eventFilter( QObject * watched, QEvent * e );
	void supportedMimetypes();

protected slots:
	// connected to actions
	void openURLFromDocument(const KUrl &url);
	void slotGoToPage();
	void slotPreviousPage();
	void slotNextPage();
	void slotGotoFirst();
	void slotGotoLast();
	void slotHistoryBack();
	void slotHistoryNext();
	void slotFind();
	void slotFindNext();
	void slotSaveFileAs();
	void slotGetNewStuff();
	void slotPreferences();
	void slotNewConfig();
	void slotPrintPreview();
	void slotShowMenu(const KPDFPage *page, const QPoint &point);
	void slotShowProperties();
	void slotShowLeftPanel();
	void slotShowPresentation();
	void slotHidePresentation();
	bool slotImportPSFile();
	void close();
	void cannotQuit();
	void setMimeTypes(KIO::Job *job);
    void saveSplitterSize();
	// can be connected to widget elements
	void updateViewActions();
	void enableTOC(bool enable);

public slots:
	// connected to Shell action (and browserExtension), not local one
	void slotPrint();
	void restoreDocument(const KUrl &url, int page);
	void saveDocumentRestoreInfo(KConfig* config);
	void slotFileDirty( const QString& );
	void slotDoFileDirty();
	void psTransformEnded();

private:
	void doPrint( KPrinter& printer );
    void fillGenerators();

	// the document
	KPDFDocument * m_document;
	QString m_temporaryLocalFile;

	// main widgets
	QSplitter *m_splitter;
	QWidget *m_leftPanel;
	QToolBox *m_toolBox;
	SearchWidget *m_searchWidget;
	QPointer<ThumbnailList> m_thumbnailList;
	QPointer<PageView> m_pageView;
	QPointer<TOC> m_toc;
	QPointer<MiniBar> m_miniBar;
	QPointer<PresentationWidget> m_presentationWidget;

	// document watcher (and reloader) variables
	KDirWatch *m_watcher;
	QTimer *m_dirtyHandler;
	DocumentViewport m_viewportDirty;

	// actions
	KAction *m_gotoPage;
	KAction *m_prevPage;
	KAction *m_nextPage;
	KAction *m_firstPage;
	KAction *m_lastPage;
	KAction *m_historyBack;
	KAction *m_historyNext;
	KAction *m_find;
	KAction *m_findNext;
	KAction *m_saveAs;
	KAction *m_printPreview;
	KAction *m_showProperties;
	KAction *m_showPresentation;
	KToggleAction* m_showMenuBarAction;
	KToggleAction* m_showLeftPanel;
	KToggleAction* m_showFullScreenAction;

	bool m_actionsSearched;
	bool m_searchStarted;
	BrowserExtension *m_bExtension;

    // QHash: key is the name of the generator
   QHash<QString, Generator*> m_loadedGenerators;
    // list of names of the generators that have settings
    QStringList m_generatorsWithSettings;
    QStringList m_supportedMimeTypes;
    KSelectAction * m_confGens;

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

}

#endif

// vim:ts=2:sw=2:tw=78:et
