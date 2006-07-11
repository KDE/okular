/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2002 by Chris Cheney <ccheney@cheney.cx>                *
 *   Copyright (C) 2002 by Malcolm Hunter <malcolm.hunter@gmx.co.uk>       *
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Daniel Molkentin <molkentin@kde.org>            *
 *   Copyright (C) 2003 by Andy Goossens <andygoossens@telenet.be>         *
 *   Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                  *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Christoph Cullmann <crossfire@babylon2k.de>     *
 *   Copyright (C) 2004 by Henrique Pinto <stampede@coltec.ufmg.br>        *
 *   Copyright (C) 2004 by Waldo Bastian <bastian@kde.org>                 *
 *   Copyright (C) 2004-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
 *   Copyright (C) 2004 by Antti Markus <antti.markus@starman.ee>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qsplitter.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qlabel.h>
#include <kvbox.h>
#include <qtoolbox.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kdirwatch.h>
#include <kinstance.h>
#include <kprinter.h>
#include <kstdaction.h>
#include <kdeversion.h>
#include <kparts/genericfactory.h>
#include <kfiledialog.h>
#include <kfind.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kfinddialog.h>
#include <knuminput.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kmenu.h>
#include <kxmlguiclient.h>
#include <kxmlguifactory.h>
#include <kprocess.h>
#include <kservicetypetrader.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <ktoggleaction.h>
#include <ktogglefullscreenaction.h>
#include <kio/job.h>

// local includes
#include "part.h"
#include "ui/pageview.h"
#include "ui/toc.h"
#include "ui/searchwidget.h"
#include "ui/thumbnaillist.h"
#include "ui/side_reviews.h"
#include "ui/minibar.h"
#include "ui/newstuff.h"
#include "ui/embeddedfilesdialog.h"
#include "ui/propertiesdialog.h"
#include "ui/presentationwidget.h"
#include "conf/preferencesdialog.h"
#include "settings.h"
#include "core/document.h"
#include "core/generator.h"
#include "core/page.h"

typedef KParts::GenericFactory<okular::Part> okularPartFactory;
K_EXPORT_COMPONENT_FACTORY(libokularpart, okularPartFactory)

using namespace okular;

Part::Part(QWidget *parentWidget,
           QObject *parent,
           const QStringList & /*args*/ )
	: KParts::ReadOnlyPart(parent), m_viewportDirty( 0 ),
	m_showMenuBarAction(0), m_showFullScreenAction(0), m_actionsSearched(false),
	m_searchStarted(false)
{
	QDBus::sessionBus().registerObject("/okular", this, QDBusConnection::ExportSlots);

	// connect the started signal to tell the job the mimetypes we like
	connect(this, SIGNAL(started(KIO::Job *)), this, SLOT(setMimeTypes(KIO::Job *)));
	// load catalog for translation
	KGlobal::locale()->insertCatalog("okular");

	// create browser extension (for printing when embedded into browser)
	m_bExtension = new BrowserExtension(this);

	// we need an instance
	setInstance(okularPartFactory::instance());

	// build the document
	m_document = new KPDFDocument(&m_loadedGenerators);
	connect( m_document, SIGNAL( linkFind() ), this, SLOT( slotFind() ) );
	connect( m_document, SIGNAL( linkGoToPage() ), this, SLOT( slotGoToPage() ) );
	connect( m_document, SIGNAL( linkPresentation() ), this, SLOT( slotShowPresentation() ) );
	connect( m_document, SIGNAL( linkEndPresentation() ), this, SLOT( slotHidePresentation() ) );
	connect( m_document, SIGNAL( openURL(const KUrl &) ), this, SLOT( openURLFromDocument(const KUrl &) ) );
	connect( m_document, SIGNAL( close() ), this, SLOT( close() ) );
	
	if ( parent && parent->metaObject()->indexOfSlot( SLOT( slotQuit() ) ) != -1 )
		connect( m_document, SIGNAL( quit() ), parent, SLOT( slotQuit() ) );
	else
		connect( m_document, SIGNAL( quit() ), this, SLOT( cannotQuit() ) );
        // widgets: ^searchbar (toolbar containing label and SearchWidget)
//      m_searchToolBar = new KToolBar( parentWidget, "searchBar" );
//      m_searchToolBar->boxLayout()->setSpacing( KDialog::spacingHint() );
//      QLabel * sLabel = new QLabel( i18n( "&Search:" ), m_searchToolBar, "kde toolbar widget" );
//      m_searchWidget = new SearchWidget( m_searchToolBar, m_document );
//      sLabel->setBuddy( m_searchWidget );
//      m_searchToolBar->setStretchableWidget( m_searchWidget );


	// widgets: [] splitter []
	m_splitter = new QSplitter( parentWidget );
	m_splitter->setOpaqueResize( true );
	m_splitter->setChildrenCollapsible( false );
	setWidget( m_splitter );

	// widgets: [left panel] | []
	m_leftPanel = new QWidget( m_splitter );
	m_leftPanel->setMinimumWidth( 90 );
	m_leftPanel->setMaximumWidth( 300 );
	QVBoxLayout * leftPanelLayout = new QVBoxLayout( m_leftPanel );
	leftPanelLayout->setMargin( 0 );

	// widgets: [left toolbox/..] | []
	m_toolBox = new QToolBox( m_leftPanel );
	leftPanelLayout->addWidget( m_toolBox );

	int tbIndex;
	// [left toolbox: Table of Contents] | []
	m_toc = new TOC( m_toolBox, m_document );
	connect( m_toc, SIGNAL( hasTOC( bool ) ), this, SLOT( enableTOC( bool ) ) );
	tbIndex = m_toolBox->addItem( m_toc, SmallIconSet("text_left"), i18n("Contents") );
	m_toolBox->setItemToolTip( tbIndex, i18n("Contents") );
	enableTOC( false );

	// [left toolbox: Thumbnails and Bookmarks] | []
	KVBox * thumbsBox = new ThumbnailsBox( m_toolBox );
	thumbsBox->setSpacing( 4 );
	m_searchWidget = new SearchWidget( thumbsBox, m_document );
	m_thumbnailList = new ThumbnailList( thumbsBox, m_document );
//	ThumbnailController * m_tc = new ThumbnailController( thumbsBox, m_thumbnailList );
	connect( m_thumbnailList, SIGNAL( urlDropped( const KUrl& ) ), SLOT( openURLFromDocument( const KUrl & )) );
	connect( m_thumbnailList, SIGNAL( rightClick(const KPDFPage *, const QPoint &) ), this, SLOT( slotShowMenu(const KPDFPage *, const QPoint &) ) );
	tbIndex = m_toolBox->addItem( thumbsBox, SmallIconSet("thumbnail"), i18n("Thumbnails") );
	m_toolBox->setItemToolTip( tbIndex, i18n("Thumbnails") );
	m_toolBox->setCurrentIndex( m_toolBox->indexOf( thumbsBox ) );

	// [left toolbox: Reviews] | []
	Reviews * reviewsWidget = new Reviews( m_toolBox, m_document );
	m_toolBox->addItem( reviewsWidget, SmallIconSet("pencil"), i18n("Reviews") );

	// widgets: [../miniBarContainer] | []
	QWidget * miniBarContainer = new QWidget( m_leftPanel );
	leftPanelLayout->addWidget( miniBarContainer );
	QVBoxLayout * miniBarLayout = new QVBoxLayout( miniBarContainer );
	miniBarLayout->setMargin( 0 );
	// widgets: [../[spacer/..]] | []
	QWidget * miniSpacer = new QWidget( miniBarContainer );
	miniSpacer->setFixedHeight( 6 );
	miniBarLayout->addWidget( miniSpacer );
	// widgets: [../[../MiniBar]] | []
	m_miniBar = new MiniBar( miniBarContainer, m_document );
	miniBarLayout->addWidget( m_miniBar );

	// widgets: [] | [right 'pageView']
//	QWidget * rightContainer = new QWidget( m_splitter );
//	QVBoxLayout * rightLayout = new QVBoxLayout( rightContainer );
//	KToolBar * rtb = new KToolBar( rightContainer, "mainToolBarSS" );
//	rightLayout->addWidget( rtb );
	m_pageView = new PageView( m_splitter, m_document );
	m_pageView->setFocus(); //usability setting
	connect( m_pageView, SIGNAL( urlDropped( const KUrl& ) ), SLOT( openURLFromDocument( const KUrl & )));
	connect( m_pageView, SIGNAL( rightClick(const KPDFPage *, const QPoint &) ), this, SLOT( slotShowMenu(const KPDFPage *, const QPoint &) ) );
	connect( m_document, SIGNAL( error( const QString&, int ) ), m_pageView, SLOT( errorMessage( const QString&, int ) ) );
	connect( m_document, SIGNAL( warning( const QString&, int ) ), m_pageView, SLOT( warningMessage( const QString&, int ) ) );
	connect( m_document, SIGNAL( notice( const QString&, int ) ), m_pageView, SLOT( noticeMessage( const QString&, int ) ) );
//	rightLayout->addWidget( m_pageView );

	// add document observers
	m_document->addObserver( this );
	m_document->addObserver( m_thumbnailList );
	m_document->addObserver( m_pageView );
	m_document->addObserver( m_toc );
	m_document->addObserver( m_miniBar );
	m_document->addObserver( reviewsWidget );

	// ACTIONS
	KActionCollection * ac = actionCollection();

	// Page Traversal actions
	m_gotoPage = KStdAction::gotoPage( this, SLOT( slotGoToPage() ), ac, "goto_page" );
	m_gotoPage->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_G) );
	// dirty way to activate gotopage when pressing miniBar's button
	connect( m_miniBar, SIGNAL( gotoPage() ), m_gotoPage, SLOT( trigger() ) );

	m_prevPage = KStdAction::prior(this, SLOT(slotPreviousPage()), ac, "previous_page");
	m_prevPage->setWhatsThis( i18n( "Moves to the previous page of the document" ) );
	m_prevPage->setShortcut( 0 );
	// dirty way to activate prev page when pressing miniBar's button
	connect( m_miniBar, SIGNAL( prevPage() ), m_prevPage, SLOT( trigger() ) );

	m_nextPage = KStdAction::next(this, SLOT(slotNextPage()), ac, "next_page" );
	m_nextPage->setWhatsThis( i18n( "Moves to the next page of the document" ) );
	m_nextPage->setShortcut( 0 );
	// dirty way to activate next page when pressing miniBar's button
	connect( m_miniBar, SIGNAL( nextPage() ), m_nextPage, SLOT( trigger() ) );

	m_firstPage = KStdAction::firstPage( this, SLOT( slotGotoFirst() ), ac, "first_page" );
	m_firstPage->setWhatsThis( i18n( "Moves to the first page of the document" ) );

	m_lastPage = KStdAction::lastPage( this, SLOT( slotGotoLast() ), ac, "last_page" );
	m_lastPage->setWhatsThis( i18n( "Moves to the last page of the document" ) );

	m_historyBack = KStdAction::back( this, SLOT( slotHistoryBack() ), ac, "history_back" );
	m_historyBack->setWhatsThis( i18n( "Go to the place you were before" ) );

	m_historyNext = KStdAction::forward( this, SLOT( slotHistoryNext() ), ac, "history_forward" );
	m_historyNext->setWhatsThis( i18n( "Go to the place you were after" ) );

	m_prevBookmark = new KAction( KIcon( "previous" ), i18n( "Previous Bookmark" ), ac, "previous_bookmark" );
	m_prevBookmark->setWhatsThis( i18n( "Go to the previous bookmarked page" ) );
	connect( m_prevBookmark, SIGNAL( triggered() ), this, SLOT( slotPreviousBookmark() ) );

	m_nextBookmark = new KAction( KIcon( "next" ), i18n( "Next Bookmark" ), ac, "next_bookmark" );
	m_nextBookmark->setWhatsThis( i18n( "Go to the next bookmarked page" ) );
	connect( m_nextBookmark, SIGNAL( triggered() ), this, SLOT( slotNextBookmark() ) );

	// Find and other actions
	m_find = KStdAction::find( this, SLOT( slotFind() ), ac, "find" );
	m_find->setEnabled( false );

	m_findNext = KStdAction::findNext( this, SLOT( slotFindNext() ), ac, "find_next" );
	m_findNext->setEnabled( false );

	m_saveAs = KStdAction::saveAs( this, SLOT( slotSaveFileAs() ), ac, "save" );
	m_saveAs->setEnabled( false );
	KAction * prefs = KStdAction::preferences( this, SLOT( slotPreferences() ), ac, "preferences" );
	if ( parent && ( parent->objectName() == QLatin1String( "okular::Shell" ) ) )
	{
		prefs->setText( i18n( "Configure okular..." ) );
	}
	else
	{
		prefs->setText( i18n( "Configure Viewer..." ) ); // TODO: improve this message
	}
	
	KAction * genPrefs = KStdAction::preferences( this, SLOT( slotGeneratorPreferences() ), ac, "generator_prefs" );
	genPrefs->setText( i18n( "Configure Backends..." ) );
	QString constraint("([X-KDE-Priority] > 0) and (exist Library) and ([X-KDE-okularHasInternalSettings])") ;
	KService::List gens = KServiceTypeTrader::self()->query("okular/Generator",constraint);
	if (gens.count() <= 0)
	{
		genPrefs->setEnabled( false );
	}

	m_printPreview = KStdAction::printPreview( this, SLOT( slotPrintPreview() ), ac );
	m_printPreview->setEnabled( false );

	m_showLeftPanel = new KToggleAction( KIcon( "show_side_panel" ), i18n( "Show &Navigation Panel"), ac, "show_leftpanel" );
	connect( m_showLeftPanel, SIGNAL( toggled( bool ) ), this, SLOT( slotShowLeftPanel() ) );
	m_showLeftPanel->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_L) );
	m_showLeftPanel->setCheckedState( i18n( "Hide &Navigation Panel" ) );
	m_showLeftPanel->setChecked( KpdfSettings::showLeftPanel() );
	slotShowLeftPanel();

	KAction * importPS= new KAction(KIcon("psimport"), i18n("&Import Postscript as PDF..."), ac, "import_ps");
	connect(importPS, SIGNAL(triggered()), this, SLOT(slotImportPSFile()));
	KAction * ghns = new KAction(KIcon("knewstuff"), i18n("&Get Books From Internet..."), ac, "get_new_stuff");
	connect(ghns, SIGNAL(triggered()), this, SLOT(slotGetNewStuff()));
	ghns->setShortcut( Qt::Key_G );  // TEMP, REMOVE ME!

	m_showProperties = new KAction(KIcon("info"), i18n("&Properties"), ac, "properties");
	connect(m_showProperties, SIGNAL(triggered()), this, SLOT(slotShowProperties()));
	m_showProperties->setEnabled( false );
	
	m_showEmbeddedFiles = new KAction(i18n("&Embedded Files"), ac, "embeddedFiles");
	connect(m_showEmbeddedFiles, SIGNAL(triggered()), this, SLOT(slotShowEmbeddedFiles()));
	m_showEmbeddedFiles->setEnabled( false );

	m_showPresentation = new KAction( KIcon( "kpresenter_kpr" ), i18n("P&resentation"), ac, "presentation");
	connect(m_showPresentation, SIGNAL(triggered()), this, SLOT(slotShowPresentation()));
	m_showPresentation->setShortcut( QKeySequence( Qt::CTRL + Qt::SHIFT + Qt::Key_P ) );
	m_showPresentation->setEnabled( false );

	m_exportAs = new KAction(i18n("E&xport As"), ac, "file_export_as");
	QMenu *menu = new QMenu();
        connect(menu, SIGNAL(triggered(QAction *)), this, SLOT(slotExportAs(QAction *)));
	m_exportAs->setMenu( menu );
	m_exportAsText = menu->addAction( KIcon( "text" ), i18n( "Text..." ) );
	m_exportAsText->setEnabled( false );
	
	// attach the actions of the children widgets too
	m_pageView->setupActions( ac );

	// apply configuration (both internal settings and GUI configured items)
	QList<int> splitterSizes = KpdfSettings::splitterSizes();
	if ( !splitterSizes.count() )
	{
		// the first time use 1/10 for the panel and 9/10 for the pageView
		splitterSizes.push_back( 50 );
		splitterSizes.push_back( 500 );
	}
	m_splitter->setSizes( splitterSizes );
	// get notified about splitter size changes (HACK that will be removed
	// by connecting to Qt4::QSplitter's sliderMoved())
	m_pageView->installEventFilter( this );

	// document watcher and reloader
	m_watcher = new KDirWatch( this );
	connect( m_watcher, SIGNAL( dirty( const QString& ) ), this, SLOT( slotFileDirty( const QString& ) ) );
	m_dirtyHandler = new QTimer( this );
	m_dirtyHandler->setSingleShot( true );
	connect( m_dirtyHandler, SIGNAL( timeout() ),this, SLOT( slotDoFileDirty() ) );

	slotNewConfig();

	// [SPEECH] check for KTTSD presence and usability
	KService::List offers = KServiceTypeTrader::self()->query("DCOP/Text-to-Speech", "Name == 'KTTSD'");
	KpdfSettings::setUseKTTSD( !offers.isEmpty() );
	KpdfSettings::writeConfig();

	// set our XML-UI resource file
	setXMLFile("part.rc");
    // 
	updateViewActions();
}

Part::~Part()
{
    delete m_toc;
    delete m_pageView;
    delete m_thumbnailList;
    delete m_miniBar;

    delete m_document;
}

void Part::openURLFromDocument(const KUrl &url)
{
    m_bExtension->openURLNotify();
    m_bExtension->setLocationBarURL(url.prettyUrl());
    openURL(url);
}

void Part::supportedMimetypes()
{
    m_supportedMimeTypes.clear();
    QString constraint("([X-KDE-Priority] > 0) and (exist Library) ") ;
    KService::List offers = KServiceTypeTrader::self()->query("okular/Generator",constraint);
    KService::List::ConstIterator iterator = offers.begin();
    KService::List::ConstIterator end = offers.end();
    QStringList::Iterator mimeType;

    for (; iterator != end; ++iterator)
    {
        KService::Ptr service = *iterator;
        QStringList mimeTypes = service->serviceTypes();
        for (mimeType=mimeTypes.begin();mimeType!=mimeTypes.end();++mimeType)
            if (! (*mimeType).contains("okular"))
                m_supportedMimeTypes << *mimeType;
    }
}

void Part::setMimeTypes(KIO::Job *job)
{
    if (job)
    {
        if (m_supportedMimeTypes.count() <= 0)
            supportedMimetypes();
        job->addMetaData("accept", m_supportedMimeTypes.join(", ") + ", */*;q=0.5");
    }
}

void Part::fillGenerators()
{
    QString constraint("([X-KDE-Priority] > 0) and (exist Library) and ([X-KDE-okularHasInternalSettings])") ;
    KService::List offers = KServiceTypeTrader::self()->query("okular/Generator", constraint);
    QString propName;
    int count=offers.count();
    if (count > 0)
    {
        KLibLoader *loader = KLibLoader::self();
        if (!loader)
        {
            kWarning() << "Could not start library loader: '" << loader->lastErrorMessage() << "'." << endl;
            return;
        }
        for (int i=0;i<count;i++)
        {
          propName=offers[i]->property("Name").toString();
          // dont load already loaded generators
          if (! m_loadedGenerators.take( propName ) )
          {
            KLibrary *lib = loader->globalLibrary( QFile::encodeName( offers[i]->library() ) );
            if (!lib) 
            {
                kWarning() << "Could not load '" << offers[i]->library() << "' library." << endl;
		continue;
            }

            Generator* (*create_plugin)(KPDFDocument* doc) = ( Generator* (*)(KPDFDocument* doc) ) lib->symbol( "create_plugin" );
            if ( !create_plugin )
            {
                kWarning() << "Library '" << offers.at(i)->library() << "' has no symbol 'create_plugin'." << endl;
                continue;
            }

            // the generator should do anything with the document if we are only configuring
            m_loadedGenerators.insert(propName,create_plugin(m_document));
            m_generatorsWithSettings << propName;
          }
        }
    }
}

void Part::slotGeneratorPreferences( )
{
    fillGenerators();
    //Generator* gen= m_loadedGenerators[m_generatorsWithSettings[number]];

    // an instance the dialog could be already created and could be cached,
    // in which case you want to display the cached dialog
    if ( KConfigDialog::showDialog( "generator_prefs" ) )
        return;

    // we didn't find an instance of this dialog, so lets create it
    KConfigDialog * dialog = new KConfigDialog( m_pageView, "generator_prefs", KpdfSettings::self() );
    dialog->setCaption( i18n( "Configure Backends" ) );

    QHashIterator<QString, Generator*> it(m_loadedGenerators);
    while(it.hasNext())
    {
        it.next();
        it.value()->addPages(dialog);
    }

    // (for now dont FIXME) keep us informed when the user changes settings
    // connect( dialog, SIGNAL( settingsChanged() ), this, SLOT( slotNewConfig() ) );
    dialog->show();
}

void Part::notifyViewportChanged( bool /*smoothMove*/ )
{
    // update actions if the page is changed
    static int lastPage = -1;
    int viewportPage = m_document->viewport().pageNumber;
    if ( viewportPage != lastPage )
    {
        updateViewActions();
        lastPage = viewportPage;
    }
}

void Part::goToPage(uint i)
{
    if ( i <= m_document->pages() )
        m_document->setViewportPage( i - 1 );
}

void Part::openDocument(KUrl doc)
{
    openURL(doc);
}

uint Part::pages()
{
    return m_document->pages();
}

uint Part::currentPage()
{
    return m_document->pages() ? m_document->currentPage() + 1 : 0;
}

KUrl Part::currentDocument()
{
    return m_document->currentDocument();
}

//this don't go anywhere but is required by genericfactory.h
KAboutData* Part::createAboutData()
{
	// the non-i18n name here must be the same as the directory in
	// which the part's rc file is installed ('partrcdir' in the
	// Makefile)
	KAboutData* aboutData = new KAboutData("okularpart", I18N_NOOP("okular::Part"), "0.1");
	aboutData->addAuthor("Wilco Greven", 0, "greven@kde.org");
	return aboutData;
}

bool Part::slotImportPSFile()
{
	QString app = KStandardDirs::findExe( "ps2pdf" );
	if ( app.isEmpty() )
	{
		// TODO point the user to their distro packages?
		KMessageBox::error( widget(), i18n( "The program \"ps2pdf\" was not found, so okular can not import PS files using it." ), i18n("ps2pdf not found") );
		return false;
	}

    KUrl url = KFileDialog::getOpenUrl( KUrl(), "application/postscript", this->widget() );
    KTempFile tf( QString::null, ".pdf" );

    if ( tf.status() == 0 && url.isLocalFile())
    {
        tf.close();
        m_file = url.path();
        m_temporaryLocalFile = tf.name();
        KProcess *p = new KProcess;
        *p << app;
        *p << m_file << m_temporaryLocalFile;
        m_pageView->displayMessage(i18n("Importing PS file as PDF (this may take a while)..."));
        connect(p, SIGNAL(processExited(KProcess *)), this, SLOT(psTransformEnded()));
        p -> start();
        return true;
    }

    m_temporaryLocalFile = QString::null;
    return false;
}

bool Part::openFile()
{
    KMimeType::Ptr mime;
    if ( m_bExtension->urlArgs().serviceType.isEmpty() )
    {
        mime = KMimeType::findByPath( m_file );
    }
    else
    {
        mime = KMimeType::mimeType( m_bExtension->urlArgs().serviceType );
    }
    bool ok = m_document->openDocument( m_file, url(), mime );
    bool canSearch = m_document->supportsSearching();

    // update one-time actions
    m_find->setEnabled( ok && canSearch );
    m_findNext->setEnabled( ok && canSearch );
    m_saveAs->setEnabled( ok );
    m_printPreview->setEnabled( ok );
    m_showProperties->setEnabled( ok );
    m_showEmbeddedFiles->setEnabled( ok && m_document->embeddedFiles() && m_document->embeddedFiles()->count() > 0);
    m_showPresentation->setEnabled( ok );
    if ( ok )
    {
        m_exportItems = m_document->exportFormats();
        QList<ExportEntry*>::ConstIterator it = m_exportItems.constBegin();
        QList<ExportEntry*>::ConstIterator itEnd = m_exportItems.constEnd();
        QMenu *menu = m_exportAs->menu();
        for ( ; it != itEnd; ++it )
        {
            ExportEntry* cur = *it;
            if ( !cur->icon.isEmpty() )
            {
                menu->addAction( KIcon( cur->icon ), cur->description );
            }
            else
            {
                menu->addAction( cur->description );
            }
        }
    }
    m_exportAsText->setEnabled( ok && m_document->canExportToText() );

    // update viewing actions
    updateViewActions();

    if ( !ok )
    {
        // if can't open document, update windows so they display blank contents
        m_pageView->updateContents();
        m_thumbnailList->update();
        return false;
    }

    // set the file to the fileWatcher
    if ( !m_watcher->contains(m_file) )
        m_watcher->addFile(m_file);

    // if the 'StartFullScreen' flag is set, start presentation
    if ( m_document->getMetaData( "StartFullScreen" ) == "yes" )
    {
        KMessageBox::information( m_presentationWidget, i18n("The document is going to be launched on presentation mode because the file requested it."), QString::null, "autoPresentationWarning" );
        slotShowPresentation();
    }
/*    if (m_document->getXMLFile() != QString::null)
        setXMLFile(m_document->getXMLFile(),true);*/
    m_document->setupGUI(actionCollection(),m_toolBox);
    return true;
}

bool Part::openURL(const KUrl &url)
{
    // note: this can be the right place to check the file for gz or bz2 extension
    // if it matches then: download it (if not local) extract to a temp file using
    // KTar and proceed with the URL of the temporary file

    // this calls in sequence the 'closeURL' and 'openFile' methods
    bool openOk = KParts::ReadOnlyPart::openURL(url);

    if ( openOk )
        m_viewportDirty = 0;
    else
        KMessageBox::error( widget(), i18n( "Could not open %1", url.prettyUrl() ) );
    emit enablePrintAction(openOk);
    return openOk;
}

bool Part::closeURL()
{
    if (!m_temporaryLocalFile.isNull())
    {
        QFile::remove( m_temporaryLocalFile );
        m_temporaryLocalFile = QString::null;
    }

    slotHidePresentation();
    m_find->setEnabled( false );
    m_findNext->setEnabled( false );
    m_saveAs->setEnabled( false );
    m_printPreview->setEnabled( false );
    m_showProperties->setEnabled( false );
    m_showEmbeddedFiles->setEnabled( false );
    m_exportAsText->setEnabled( false );
    m_exportItems.clear();
    QMenu *menu = m_exportAs->menu();
    QList<QAction*> acts = menu->actions();
    int num = acts.count();
    for ( int i = 1; i < num; ++i )
    {
        menu->removeAction( acts.at(i) );
        delete acts.at(i);
    }
    m_showPresentation->setEnabled( false );
    emit setWindowCaption("");
    emit enablePrintAction(false);
    m_searchStarted = false;
    if (!m_file.isEmpty()) m_watcher->removeFile(m_file);
    m_document->closeDocument();
    updateViewActions();
    m_searchWidget->clearText();
    return KParts::ReadOnlyPart::closeURL();
}

void Part::close()
{
  if (parent() && (parent()->objectName() == QLatin1String("okular::Shell")))
  {
    closeURL();
  }
  else KMessageBox::information(widget(), i18n("This link points to a close document action that does not work when using the embedded viewer."), QString::null, "warnNoCloseIfNotInKPDF");
}

void Part::cannotQuit()
{
	KMessageBox::information(widget(), i18n("This link points to a quit application action that does not work when using the embedded viewer."), QString::null, "warnNoQuitIfNotInKPDF");
}

bool Part::eventFilter( QObject * watched, QEvent * e )
{
    // if pageView has been resized, save splitter sizes
    if ( watched == m_pageView && e->type() == QEvent::Resize )
        saveSplitterSize();

    // only intercept events, don't block them
    return false;
}

void Part::slotShowLeftPanel()
{
    bool showLeft = m_showLeftPanel->isChecked();
    KpdfSettings::setShowLeftPanel( showLeft );
    KpdfSettings::writeConfig();
    // show/hide left panel
    m_leftPanel->setVisible( showLeft );
    // this needs to be hidden explicitly to disable thumbnails gen
    m_thumbnailList->setVisible( showLeft );
}

void Part::saveSplitterSize()
{
    KpdfSettings::setSplitterSizes( m_splitter->sizes() );
    KpdfSettings::writeConfig();
}

void Part::slotFileDirty( const QString& fileName )
{
  // The beauty of this is that each start cancels the previous one.
  // This means that timeout() is only fired when there have
  // no changes to the file for the last 750 milisecs.
  // This ensures that we don't update on every other byte that gets
  // written to the file.
  if ( fileName == m_file )
  {
    m_dirtyHandler->start( 750 );
  }
}

void Part::slotDoFileDirty()
{
    // do the following the first time the file is reloaded
    if ( m_viewportDirty.pageNumber == -1 )
    {
        // store the current viewport
        m_viewportDirty = DocumentViewport( m_document->viewport() );

        // inform the user about the operation in progress
        m_pageView->displayMessage( i18n("Reloading the document...") );
    }

    // close and (try to) reopen the document
    if ( KParts::ReadOnlyPart::openURL(m_file) )
    {
        // on successfull opening, restore the previous viewport
        if ( m_viewportDirty.pageNumber >= (int) m_document->pages() ) 
            m_viewportDirty.pageNumber = (int) m_document->pages() - 1;
        m_document->setViewport( m_viewportDirty );
        m_viewportDirty.pageNumber = -1;
        emit enablePrintAction(true);
    }
    else
    {
        // start watching the file again (since we dropped it on close)
        m_watcher->addFile(m_file);
        m_dirtyHandler->start( 750 );
    }
}

void Part::updateViewActions()
{
    bool opened = m_document->pages() > 0;
    if ( opened )
    {
        bool atBegin = m_document->currentPage() < 1;
        bool atEnd = m_document->currentPage() >= (m_document->pages() - 1);
        m_gotoPage->setEnabled( m_document->pages() > 1 );
        m_firstPage->setEnabled( !atBegin );
        m_prevPage->setEnabled( !atBegin );
        m_lastPage->setEnabled( !atEnd );
        m_nextPage->setEnabled( !atEnd );
        m_historyBack->setEnabled( !m_document->historyAtBegin() );
        m_historyNext->setEnabled( !m_document->historyAtEnd() );
    }
    else
    {
        m_gotoPage->setEnabled( false );
        m_firstPage->setEnabled( false );
        m_lastPage->setEnabled( false );
        m_prevPage->setEnabled( false );
        m_nextPage->setEnabled( false );
        m_historyBack->setEnabled( false );
        m_historyNext->setEnabled( false );
    }
}

void Part::enableTOC(bool enable)
{
	m_toolBox->setItemEnabled(0, enable);
}

//BEGIN go to page dialog
class KPDFGotoPageDialog : public KDialog
{
public:
	KPDFGotoPageDialog(QWidget *p, int current, int max) : KDialog(p) {
		setCaption(i18n("Go to Page"));
		setButtons(Ok | Cancel);
		setDefaultButton(Ok);

		QWidget *w = new QWidget(this);
		setMainWidget(w);

		QVBoxLayout *topLayout = new QVBoxLayout(w);
		topLayout->setMargin(0);
		topLayout->setSpacing(spacingHint());
		e1 = new KIntNumInput(current, w);
		e1->setRange(1, max);
		e1->setEditFocus(true);

		QLabel *label = new QLabel(i18n("&Page:"), w);
		label->setBuddy(e1);
		topLayout->addWidget(label);
		topLayout->addWidget(e1);
		topLayout->addSpacing(spacingHint()); // A little bit extra space
		topLayout->addStretch(10);
		e1->setFocus();
	}

	int getPage() const {
		return e1->value();
	}

  protected:
    KIntNumInput *e1;
};
//END go to page dialog

void Part::slotGoToPage()
{
    KPDFGotoPageDialog pageDialog( m_pageView, m_document->currentPage() + 1, m_document->pages() );
    if ( pageDialog.exec() == QDialog::Accepted )
        m_document->setViewportPage( pageDialog.getPage() - 1 );
}

void Part::slotPreviousPage()
{
    if ( m_document->isOpened() && !(m_document->currentPage() < 1) )
        m_document->setViewportPage( m_document->currentPage() - 1 );
}

void Part::slotNextPage()
{
    if ( m_document->isOpened() && m_document->currentPage() < (m_document->pages() - 1) )
        m_document->setViewportPage( m_document->currentPage() + 1 );
}

void Part::slotGotoFirst()
{
    if ( m_document->isOpened() )
        m_document->setViewportPage( 0 );
}

void Part::slotGotoLast()
{
    if ( m_document->isOpened() )
        m_document->setViewportPage( m_document->pages() - 1 );
}

void Part::slotHistoryBack()
{
    m_document->setPrevViewport();
}

void Part::slotHistoryNext()
{
    m_document->setNextViewport();
}

void Part::slotPreviousBookmark()
{
    uint current = m_document->currentPage();
    // we are at the first page
    if ( current == 0 )
        return;

    for ( int i = current - 1; i >= 0; --i )
    {
        if ( m_document->page( i )->hasBookmark() )
        {
            m_document->setViewportPage( i );
            break;
        }
    }
}

void Part::slotNextBookmark()
{
    uint current = m_document->currentPage();
    uint pages = m_document->pages();
    // we are at the last page
    if ( current == pages )
        return;

    for ( uint i = current + 1; i < pages; ++i )
    {
        if ( m_document->page( i )->hasBookmark() )
        {
            m_document->setViewportPage( i );
            break;
        }
    }
}

void Part::slotFind()
{
    KFindDialog dlg( widget() );
    dlg.setHasCursor( false );
    if ( !m_searchHistory.empty() )
        dlg.setFindHistory( m_searchHistory );
    dlg.setSupportsBackwardsFind( false );
    dlg.setSupportsWholeWordsFind( false );
    dlg.setSupportsRegularExpressionFind( false );
    if ( dlg.exec() == QDialog::Accepted )
    {
        m_searchHistory = dlg.findHistory();
        m_searchStarted = true;
        m_document->resetSearch( PART_SEARCH_ID );
        m_document->searchText( PART_SEARCH_ID, dlg.pattern(), false, dlg.options() & KFind::CaseSensitive,
                                KPDFDocument::NextMatch, true, qRgb( 255, 255, 64 ) );
    }
}

void Part::slotFindNext()
{
    if (!m_document->continueLastSearch())
        slotFind();
}

void Part::slotSaveFileAs()
{
    KUrl saveURL = KFileDialog::getSaveUrl( url().isLocalFile() ? url().url() : url().fileName(), QString::null, widget() );
    if ( saveURL.isValid() && !saveURL.isEmpty() )
    {
        if ( KIO::NetAccess::exists( saveURL, false, widget() ) )
        {
            if (KMessageBox::warningContinueCancel( widget(), i18n("A file named \"%1\" already exists. Are you sure you want to overwrite it?", saveURL.fileName()), QString::null, i18n("Overwrite")) != KMessageBox::Continue)
                return;
        }

        if ( !KIO::NetAccess::file_copy( url(), saveURL, -1, true ) )
            KMessageBox::information( widget(), i18n("File could not be saved in '%1'. Try to save it to another location.", saveURL.prettyUrl() ) );
    }
}

void Part::slotGetNewStuff()
{
    // show the modal dialog over pageview and execute it
    NewStuffDialog * dialog = new NewStuffDialog( m_pageView );
    dialog->exec();
    delete dialog;
}

void Part::slotPreferences()
{
    // an instance the dialog could be already created and could be cached,
    // in which case you want to display the cached dialog
    if ( PreferencesDialog::showDialog( "preferences" ) )
        return;

    // we didn't find an instance of this dialog, so lets create it
    PreferencesDialog * dialog = new PreferencesDialog( m_pageView, KpdfSettings::self() );
    // keep us informed when the user changes settings
    connect( dialog, SIGNAL( settingsChanged( const QString & ) ), this, SLOT( slotNewConfig() ) );

    dialog->show();
}

void Part::slotNewConfig()
{
    // Apply settings here. A good policy is to check wether the setting has
    // changed before applying changes.

    // Watch File
    bool watchFile = KpdfSettings::watchFile();
    if ( watchFile && m_watcher->isStopped() )
        m_watcher->startScan();
    if ( !watchFile && !m_watcher->isStopped() )
    {
        m_dirtyHandler->stop();
        m_watcher->stopScan();
    }

    bool showSearch = KpdfSettings::showSearchBar();
    if ( !m_searchWidget->isHidden() != showSearch )
        m_searchWidget->setVisible( showSearch );

    // Main View (pageView)
    Q3ScrollView::ScrollBarMode scrollBarMode = KpdfSettings::showScrollBars() ?
        Q3ScrollView::AlwaysOn : Q3ScrollView::AlwaysOff;
    if ( m_pageView->hScrollBarMode() != scrollBarMode )
    {
        m_pageView->setHScrollBarMode( scrollBarMode );
        m_pageView->setVScrollBarMode( scrollBarMode );
    }

    // update document settings
    m_document->reparseConfig();

    // update Main View and ThumbnailList contents
    // TODO do this only when changing KpdfSettings::renderMode()
    m_pageView->updateContents();
    if ( KpdfSettings::showLeftPanel() && !m_thumbnailList->isHidden() )
        m_thumbnailList->updateWidgets();
}

void Part::slotPrintPreview()
{
    if (m_document->pages() == 0) return;

    double width, height;
    int landscape, portrait;
    KPrinter printer;
    const KPDFPage *page;

    printer.setMinMax(1, m_document->pages());
    printer.setPreviewOnly( true );
    printer.setMargins(0, 0, 0, 0);

    // if some pages are landscape and others are not the most common win as kprinter does
    // not accept a per page setting
    landscape = 0;
    portrait = 0;
    for (uint i = 0; i < m_document->pages(); i++)
    {
        page = m_document->page(i);
        width = page->width();
        height = page->height();
        if (page->orientation() == 90 || page->orientation() == 270) qSwap(width, height);
        if (width > height) landscape++;
        else portrait++;
    }
    if (landscape > portrait) printer.setOption("orientation-requested", "4");

    doPrint(printer);
}

void Part::slotShowMenu(const KPDFPage *page, const QPoint &point)
{
	bool reallyShow = false;
	if (!m_actionsSearched)
	{
		// the quest for options_show_menubar
		KActionCollection *ac;
		KAction *act;
		
		if (factory())
		{
			QList<KXMLGUIClient*> clients(factory()->clients());
			for(int i = 0 ; (!m_showMenuBarAction || !m_showFullScreenAction) && i < clients.size(); ++i)
			{
				ac = clients.at(i)->actionCollection();
				// show_menubar
				act = ac->action("options_show_menubar");
				if (act && qobject_cast<KToggleAction*>(act))
					m_showMenuBarAction = qobject_cast<KToggleAction*>(act);
				// fullscreen
				act = ac->action("fullscreen");
				if (act && qobject_cast<KToggleFullScreenAction*>(act))
					m_showFullScreenAction = qobject_cast<KToggleFullScreenAction*>(act);
			}
		}
		m_actionsSearched = true;
	}
	
	KMenu *popup = new KMenu( widget() );
	QAction *toggleBookmark, *fitPageWidth;
	toggleBookmark = 0;
	fitPageWidth = 0;
	if (page)
	{
		popup->addTitle( i18n( "Page %1", page->number() + 1 ) );
		if ( page->hasBookmark() )
			toggleBookmark = popup->addAction( KIcon("bookmark"), i18n("Remove Bookmark") );
		else
			toggleBookmark = popup->addAction( KIcon("bookmark_add"), i18n("Add Bookmark") );
		if ( m_pageView->canFitPageWidth() )
			fitPageWidth = popup->addAction( KIcon("viewmagfit"), i18n("Fit Width") );
		//popup->insertItem( SmallIcon("pencil"), i18n("Edit"), 3 );
		//popup->setItemEnabled( 3, false );
		popup->addAction( m_prevBookmark );
		popup->addAction( m_nextBookmark );
		reallyShow = true;
        }
/*
	//Albert says: I have not ported this as i don't see it does anything
    if ( d->mouseOnRect ) // and rect->objectType() == ObjectRect::Image ...
	{
		m_popup->insertItem( SmallIcon("filesave"), i18n("Save Image..."), 4 );
		m_popup->setItemEnabled( 4, false );
}*/
	
	if ((m_showMenuBarAction && !m_showMenuBarAction->isChecked()) || (m_showFullScreenAction && m_showFullScreenAction->isChecked()))
	{
		popup->addTitle( i18n( "Tools" ) );
		if (m_showMenuBarAction && !m_showMenuBarAction->isChecked()) popup->addAction(m_showMenuBarAction);
		if (m_showFullScreenAction && m_showFullScreenAction->isChecked()) popup->addAction(m_showFullScreenAction);
		reallyShow = true;
		
	}
	
	if (page && reallyShow)
	{
		QAction *res = popup->exec(point);
		if (res == toggleBookmark) m_document->toggleBookmark( page->number() );
		else if (res == fitPageWidth) m_pageView->fitPageWidth( page->number() );
	}
	delete popup;
}

void Part::slotShowProperties()
{
	PropertiesDialog *d = new PropertiesDialog(widget(), m_document);
	d->exec();
	delete d;
}

void Part::slotShowEmbeddedFiles()
{
	EmbeddedFilesDialog *d = new EmbeddedFilesDialog(widget(), m_document);
	d->exec();
	delete d;
}

void Part::slotShowPresentation()
{
    if ( !m_presentationWidget )
    {
        m_presentationWidget = new PresentationWidget( widget(), m_document );
        m_presentationWidget->setupActions( actionCollection() );
    }
}

void Part::slotHidePresentation()
{
    if ( m_presentationWidget )
        delete (PresentationWidget*) m_presentationWidget;
}

void Part::slotExportAs(QAction * act)
{
    QList<QAction*> acts = m_exportAs->menu() ? m_exportAs->menu()->actions() : QList<QAction*>();
    int id = acts.indexOf( act );
    if ( ( id < 0 ) || ( id >= acts.count() ) )
        return;

    QString filter = id == 0 ? "text/plain" : m_exportItems.at( id - 1 )->mime->name();
    QString fileName = KFileDialog::getSaveFileName( url().isLocalFile() ? url().fileName() : QString::null, filter, widget() );
    if ( !fileName.isEmpty() )
    {
        bool saved = id == 0 ? m_document->exportToText( fileName ) : m_document->exportTo( fileName, m_exportItems.at( id - 1 )->mime );
        if ( !saved )
            KMessageBox::information( widget(), i18n("File could not be saved in '%1'. Try to save it to another location.", fileName ) );
    }
}

void Part::slotPrint()
{
    if (m_document->pages() == 0) return;

    double width, height;
    int landscape, portrait;
    KPrinter printer;
    const KPDFPage *page;

    printer.setPageSelection(KPrinter::ApplicationSide);
    printer.setMinMax(1, m_document->pages());
    printer.setCurrentPage(m_document->currentPage()+1);
    printer.setMargins(0, 0, 0, 0);

    // if some pages are landscape and others are not the most common win as kprinter does
    // not accept a per page setting
    landscape = 0;
    portrait = 0;
    for (uint i = 0; i < m_document->pages(); i++)
    {
        page = m_document->page(i);
        width = page->width();
        height = page->height();
        if (page->orientation() == 90 || page->orientation() == 270) qSwap(width, height);
        if (width > height) landscape++;
        else portrait++;
    }
    if (landscape > portrait) printer.setOrientation(KPrinter::Landscape);

    if ( m_document->canConfigurePrinter() )
        doPrint( printer );
    else if (printer.setup(widget())) doPrint( printer );
}

void Part::doPrint(KPrinter &printer)
{
    if (!m_document->isAllowed(KPDFDocument::AllowPrint))
    {
        KMessageBox::error(widget(), i18n("Printing this document is not allowed."));
        return;
    }

    if (!m_document->print(printer))
    {
        KMessageBox::error(widget(), i18n("Could not print the document. Please report to bugs.kde.org"));	
    }
}

void Part::restoreDocument(const KUrl &url, int page)
{
  if (openURL(url)) goToPage(page);
}

void Part::saveDocumentRestoreInfo(KConfig* config)
{
  config->writePathEntry( "URL", url().url() );
  if (m_document->pages() > 0) config->writeEntry( "Page", m_document->currentPage() + 1 );
}

void Part::psTransformEnded()
{
       m_file = m_temporaryLocalFile;
       openFile();
}

/*
* BrowserExtension class
*/
BrowserExtension::BrowserExtension(Part* parent)
  : KParts::BrowserExtension( parent )
{
	emit enableAction("print", true);
	setURLDropHandlingEnabled(true);
}

void BrowserExtension::print()
{
	static_cast<Part*>(parent())->slotPrint();
}

#include "part.moc"
