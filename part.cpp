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
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
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
#include <qvbox.h>
#include <qtoolbox.h>
#include <qpushbutton.h>
#include <dcopobject.h>
#include <kaction.h>
#include <kdirwatch.h>
#include <kinstance.h>
#include <kprinter.h>
#include <kstdaction.h>
#include <kdeversion.h>
#include <kparts/genericfactory.h>
#include <kurldrag.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kfinddialog.h>
#include <knuminput.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kpopupmenu.h>
#include <kxmlguiclient.h>
#include <kxmlguifactory.h>

// local includes
#include "xpdf/GlobalParams.h"
#include "part.h"
#include "ui/pageview.h"
#include "ui/thumbnaillist.h"
#include "ui/searchwidget.h"
#include "ui/toc.h"
#include "ui/propertiesdialog.h"
#include "ui/presentationwidget.h"
#include "conf/preferencesdialog.h"
#include "conf/settings.h"
#include "core/document.h"
#include "core/page.h"

typedef KParts::GenericFactory<KPDF::Part> KPDFPartFactory;
K_EXPORT_COMPONENT_FACTORY(libkpdfpart, KPDFPartFactory)

using namespace KPDF;

unsigned int Part::m_count = 0;

Part::Part(QWidget *parentWidget, const char *widgetName,
           QObject *parent, const char *name,
           const QStringList & /*args*/ )
	: DCOPObject("kpdf"), KParts::ReadOnlyPart(parent, name), m_showMenuBarAction(0), m_actionsSearched(false)
{
	// create browser extension (for printing when embedded into browser)
	new BrowserExtension(this);

	// xpdf 'extern' global class (m_count is a static instance counter)
	//if ( m_count ) TODO check if we need to insert these lines..
	//	delete globalParams;
	globalParams = new GlobalParams("");
	globalParams->setupBaseFonts(NULL);
	m_count++;

	// we need an instance
	setInstance(KPDFPartFactory::instance());

	// build the document
	m_document = new KPDFDocument();
	connect( m_document, SIGNAL( linkFind() ), this, SLOT( slotFind() ) );
	connect( m_document, SIGNAL( linkGoToPage() ), this, SLOT( slotGoToPage() ) );

	// widgets: ^searchbar (toolbar containing label and SearchWidget)
//	m_searchToolBar = new KToolBar( parentWidget, "searchBar" );
//	m_searchToolBar->boxLayout()->setSpacing( KDialog::spacingHint() );
//	QLabel * sLabel = new QLabel( i18n( "&Search:" ), m_searchToolBar, "kde toolbar widget" );
//	m_searchWidget = new SearchWidget( m_searchToolBar, m_document );
//	sLabel->setBuddy( m_searchWidget );
//	m_searchToolBar->setStretchableWidget( m_searchWidget );

	// widgets: [] splitter []
	m_splitter = new QSplitter( parentWidget, widgetName );
	m_splitter->setOpaqueResize( true );
	setWidget( m_splitter );
	m_watchFile = new KToggleAction( i18n( "&Watch File" ), 0, this, SLOT( slotWatchFile() ), actionCollection(), "watch_file" );
	m_watchFile->setChecked( Settings::watchFile() );

	// widgets: [left toolbox] | []
	m_toolBox = new QToolBox( m_splitter );
	m_toolBox->setMinimumWidth( 80 );
	m_toolBox->setMaximumWidth( 300 );

	TOC * tocFrame = new TOC( m_toolBox, m_document );
	m_toolBox->addItem( tocFrame, QIconSet(SmallIcon("text_left")), i18n("Contents") );
	connect(tocFrame, SIGNAL(hasTOC(bool)), this, SLOT(enableTOC(bool)));
	enableTOC( false );

	QVBox * thumbsBox = new ThumbnailsBox( m_toolBox );
    m_searchWidget = new SearchWidget( thumbsBox, m_document );
    m_thumbnailList = new ThumbnailList( thumbsBox, m_document );
	connect( m_thumbnailList, SIGNAL( urlDropped( const KURL& ) ), SLOT( openURL( const KURL & )));
	m_toolBox->addItem( thumbsBox, QIconSet(SmallIcon("thumbnail")), i18n("Thumbnails") );
	m_toolBox->setCurrentItem( thumbsBox );

// commented because probably the thumbnaillist will act as the bookmark widget too
//	QFrame * bookmarksFrame = new QFrame( m_toolBox );
//	int iIdx = m_toolBox->addItem( bookmarksFrame, QIconSet(SmallIcon("bookmark")), i18n("Bookmarks") );
//	m_toolBox->setItemEnabled( iIdx, false );

/*	QFrame * editFrame = new QFrame( m_toolBox );
	int iIdx = m_toolBox->addItem( editFrame, QIconSet(SmallIcon("pencil")), i18n("Annotations") );
	m_toolBox->setItemEnabled( iIdx, false );*/

	// widgets: [] | [right 'pageView']
	m_pageView = new PageView( m_splitter, m_document );
        m_pageView->setFocus(); //usability setting
	connect( m_pageView, SIGNAL( urlDropped( const KURL& ) ), SLOT( openURL( const KURL & )));
	connect(m_pageView, SIGNAL( rightClick(const KPDFPage *, const QPoint &) ), this, SLOT( slotShowMenu(const KPDFPage *, const QPoint &) ));

	// add document observers
	m_document->addObserver( this );
	m_document->addObserver( m_thumbnailList );
	m_document->addObserver( m_pageView );
	m_document->addObserver( tocFrame );

	// ACTIONS
	KActionCollection * ac = actionCollection();

	// Page Traversal actions
	m_gotoPage = KStdAction::gotoPage( this, SLOT( slotGoToPage() ), ac, "goto_page" );
	m_gotoPage->setShortcut( "CTRL+G" );

	m_prevPage = KStdAction::prior(this, SLOT(slotPreviousPage()), ac, "previous_page");
	m_prevPage->setWhatsThis( i18n( "Moves to the previous page of the document" ) );
	m_prevPage->setShortcut( "Backspace" );

	m_nextPage = KStdAction::next(this, SLOT(slotNextPage()), ac, "next_page" );
	m_nextPage->setWhatsThis( i18n( "Moves to the next page of the document" ) );
	m_nextPage->setShortcut( "Space" );

	m_firstPage = KStdAction::firstPage( this, SLOT( slotGotoFirst() ), ac, "first_page" );
	m_firstPage->setWhatsThis( i18n( "Moves to the first page of the document" ) );

	m_lastPage = KStdAction::lastPage( this, SLOT( slotGotoLast() ), ac, "last_page" );
	m_lastPage->setWhatsThis( i18n( "Moves to the last page of the document" ) );

	// Find and other actions
	m_find = KStdAction::find( this, SLOT( slotFind() ), ac, "find" );
	m_find->setEnabled(false);

	m_findNext = KStdAction::findNext( this, SLOT( slotFindNext() ), ac, "find_next" );
	m_findNext->setEnabled(false);

	KStdAction::saveAs( this, SLOT( slotSaveFileAs() ), ac, "save" );
    KStdAction::preferences( this, SLOT( slotPreferences() ), ac, "preferences" );
	KStdAction::printPreview( this, SLOT( slotPrintPreview() ), ac );

	m_showProperties = new KAction(i18n("Properties"), "info", 0, this, SLOT(slotShowProperties()), ac, "properties");
	m_showProperties->setEnabled( false );

    m_showPresentation = new KAction( i18n("Presentation"), "kpresenter_kpr", 0, this, SLOT(slotShowPresentation()), ac, "presentation");
    m_showPresentation->setEnabled( false );

    // attach the actions of the 2 children widgets too
    m_pageView->setupActions( ac );

    // apply configuration (both internal settings and GUI configured items)
    QValueList<int> splitterSizes = Settings::splitterSizes();
    if ( !splitterSizes.count() )
    {
        // the first time use 1/10 for the panel and 9/10 for the pageView
        splitterSizes.push_back( 50 );
        splitterSizes.push_back( 500 );
    }
    m_splitter->setSizes( splitterSizes );
    slotNewConfig();

	m_watcher = new KDirWatch( this );
	connect( m_watcher, SIGNAL( dirty( const QString& ) ), this, SLOT( slotFileDirty( const QString& ) ) );
	m_dirtyHandler = new QTimer( this );
	connect( m_dirtyHandler, SIGNAL( timeout() ),this, SLOT( slotDoFileDirty() ) );

	// set our XML-UI resource file
	setXMLFile("part.rc");
	updateActions();
	slotWatchFile();
}

Part::~Part()
{
    // save internal settings
    Settings::setSplitterSizes( m_splitter->sizes() );
    // write to disk config file
    Settings::writeConfig();

    delete m_document;
    if ( --m_count == 0 )
        delete globalParams;
}

void Part::pageSetCurrent( int, const QRect & )
{
    // document tells that page has changed, so update actions
    updateActions();
}

void Part::goToPage(uint i)
{
    if ( i <= m_document->pages() )
        m_document->setCurrentPage( i - 1 );
}

void Part::openDocument(KURL doc)
{
	openURL(doc);
}

uint Part::pages()
{
	return m_document->pages();
}

//this don't go anywhere but is required by genericfactory.h
KAboutData* Part::createAboutData()
{
	// the non-i18n name here must be the same as the directory in
	// which the part's rc file is installed ('partrcdir' in the
	// Makefile)
	KAboutData* aboutData = new KAboutData("kpdfpart", I18N_NOOP("KPDF::Part"), "0.1");
	aboutData->addAuthor("Wilco Greven", 0, "greven@kde.org");
	return aboutData;
}

bool Part::openFile()
{
    bool ok = m_document->openDocument( m_file );
    if ( ok && !m_watcher->contains(m_file)) m_watcher->addFile(m_file);
    m_find->setEnabled( ok );
    m_showProperties->setEnabled( ok );
    m_showPresentation->setEnabled( ok );

    if ( ok && m_document->getMetaData( "StartFullScreen" ) == "yes" )
        slotShowPresentation();

    return ok;
}

bool Part::openURL(const KURL &url)
{
    bool b = KParts::ReadOnlyPart::openURL(url);
    // if can't open document, update windows so they display blank contents
    if ( !b )
    {
        m_pageView->updateContents();
        m_thumbnailList->updateContents();
        KMessageBox::error( widget(), i18n("Could not open %1").arg(url.prettyURL()) );
    }
    return b;
}

void
Part::slotWatchFile()
{
  Settings::setWatchFile(m_watchFile->isChecked());
  if( m_watchFile->isChecked() )
    m_watcher->startScan();
  else
  {
    m_dirtyHandler->stop();
    m_watcher->stopScan();
  }
}

  void
Part::slotFileDirty( const QString& fileName )
{
  // The beauty of this is that each start cancels the previous one.
  // This means that timeout() is only fired when there have
  // no changes to the file for the last 750 milisecs.
  // This is supposed to ensure that we don't update on every other byte
  // that gets written to the file.
  if ( fileName == m_file )
  {
    m_dirtyHandler->start( 750, true );
  }
}

  void
Part::slotDoFileDirty()
{
  uint p = m_document->currentPage() + 1;
  if (openFile())
  {
    if (p > m_document->pages()) p = m_document->pages();
    goToPage(p);
  }
}

bool Part::closeURL()
{
	if (!m_file.isEmpty()) m_watcher->removeFile(m_file);
	m_document->closeDocument();
	return KParts::ReadOnlyPart::closeURL();
}

void Part::updateActions()
{
    if ( m_document->pages() > 0 )
    {
        bool atBegin = m_document->currentPage() < 1;
        bool atEnd = m_document->currentPage() >= (m_document->pages() - 1);
        m_gotoPage->setEnabled( m_document->pages() > 1 );
        m_firstPage->setEnabled( !atBegin );
        m_prevPage->setEnabled( !atBegin );
        m_lastPage->setEnabled( !atEnd );
        m_nextPage->setEnabled( !atEnd );
    }
    else
    {
        m_gotoPage->setEnabled( false );
        m_firstPage->setEnabled( false );
        m_lastPage->setEnabled( false );
        m_prevPage->setEnabled( false );
        m_nextPage->setEnabled( false );
    }
}

void Part::enableTOC(bool enable)
{
	m_toolBox->setItemEnabled(0, enable);
}

//BEGIN go to page dialog
class KPDFGotoPageDialog : public KDialogBase
{
public:
	KPDFGotoPageDialog(QWidget *p, int current, int max) : KDialogBase(p, 0L, true, i18n("Go to Page"), Ok | Cancel, Ok) {
		QWidget *w = new QWidget(this);
		setMainWidget(w);

		QVBoxLayout *topLayout = new QVBoxLayout( w, 0, spacingHint() );
		e1 = new KIntNumInput(current, w);
		e1->setRange(1, max);
		e1->setEditFocus(true);

		QLabel *label = new QLabel( e1,i18n("&Page:"), w );
		topLayout->addWidget(label);
		topLayout->addWidget(e1);
		topLayout->addSpacing(spacingHint()); // A little bit extra space
		topLayout->addStretch(10);
		e1->setFocus();
	}

	int getPage() {
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
        m_document->setCurrentPage( pageDialog.getPage() - 1 );
}

void Part::slotPreviousPage()
{
    if ( !m_document->currentPage() < 1 )
        m_document->setCurrentPage( m_document->currentPage() - 1 );
}

void Part::slotNextPage()
{
    if ( m_document->currentPage() < (m_document->pages() - 1) )
        m_document->setCurrentPage( m_document->currentPage() + 1 );
}

void Part::slotGotoFirst()
{
    m_document->setCurrentPage( 0 );
}

void Part::slotGotoLast()
{
    m_document->setCurrentPage( m_document->pages() - 1 );
}

void Part::slotFind()
{
    KFindDialog dlg( widget() );
    dlg.setHasCursor( false );
#if KDE_IS_VERSION(3,3,90)
    dlg.setSupportsBackwardsFind( false );
    dlg.setSupportsWholeWordsFind( false );
    dlg.setSupportsRegularExpressionFind( false );
#endif
    if ( dlg.exec() == QDialog::Accepted )
    {
        m_findNext->setEnabled( true );
        m_document->findText( dlg.pattern(), dlg.options() & KFindDialog::CaseSensitive );
    }
}

void Part::slotFindNext()
{
    m_document->findText();
}

void Part::slotSaveFileAs()
{
    KURL saveURL = KFileDialog::getSaveURL( url().isLocalFile() ? url().url() : url().fileName(), QString::null, widget() );
    if ( saveURL.isValid() && !saveURL.isEmpty() )
    {
        if ( !KIO::NetAccess::file_copy( url(), saveURL, -1, true ) )
            KMessageBox::information( 0, i18n("File could not be saved in '%1'. Try to save it to another location.").arg( saveURL.prettyURL() ) );
    }
}

void Part::slotPreferences()
{
    // an instance the dialog could be already created and could be cached,
    // in which case you want to display the cached dialog
    if ( PreferencesDialog::showDialog( "preferences" ) )
        return;

    // we didn't find an instance of this dialog, so lets create it
    PreferencesDialog * dialog = new PreferencesDialog( m_pageView, Settings::self() );
    // keep us informed when the user changes settings
    connect( dialog, SIGNAL( settingsChanged() ), this, SLOT( slotNewConfig() ) );

    dialog->show();
}

void Part::slotNewConfig()
{
    // Apply settings here. A good policy is to check wether the setting has
    // changed before applying changes.

    // Left Panel and search Widget
    bool showLeft = Settings::showLeftPanel();
    if ( m_toolBox->isShown() != showLeft )
    {
        // show/hide left qtoolbox
        m_toolBox->setShown( showLeft );
        // this needs to be hidden explicitly to disable thumbnails gen
        m_thumbnailList->setShown( showLeft );
    }
    bool showSearch = Settings::showSearchBar();
    if ( m_searchWidget->isShown() != showSearch )
        m_searchWidget->setShown( showSearch );

    // Main View (pageView)
    QScrollView::ScrollBarMode scrollBarMode = Settings::showScrollBars() ?
        QScrollView::AlwaysOn : QScrollView::AlwaysOff;
    if ( m_pageView->hScrollBarMode() != scrollBarMode )
    {
        m_pageView->setHScrollBarMode( scrollBarMode );
        m_pageView->setVScrollBarMode( scrollBarMode );
    }

    // update document settings
    m_document->reparseConfig();

    // update Main View and ThumbnailList contents
    // TODO do this only when changing Settings::renderMode()
    m_pageView->updateContents();
    if ( showLeft && m_thumbnailList->isShown() )
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
        if (page->rotation() == 90 || page->rotation() == 270) qSwap(width, height);
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
		KXMLGUIClient *client;
		KActionCollection *ac;
		KActionPtrList::const_iterator it, end, begin;
		KActionPtrList actions;
		QPtrList<KXMLGUIClient> clients(factory()->clients());
		QPtrListIterator<KXMLGUIClient> clientsIt( clients );
		for( ; (!m_showMenuBarAction || !m_showFullScreenAction) && clientsIt.current(); ++clientsIt)
		{
			client = clientsIt.current();
			ac = client->actionCollection();
			actions = ac->actions();
			end = actions.end();
			begin = actions.begin();
			for ( it = begin; it != end; ++it )
			{
				if (QString((*it)->name()) == "options_show_menubar") m_showMenuBarAction = (KToggleAction*)(*it);
				if (QString((*it)->name()) == "fullscreen") m_showFullScreenAction = (KToggleAction*)(*it);
			}
		}
		m_actionsSearched = true;
	}
	
	
	KPopupMenu *popup = new KPopupMenu( widget(), "rmb popup" );
	if (page)
	{
		popup->insertTitle( i18n( "Page %1" ).arg( page->number() + 1 ) );
		if ( page->attributes() & KPDFPage::Bookmark )
			popup->insertItem( SmallIcon("bookmark"), i18n("Remove Bookmark"), 1 );
		else
			popup->insertItem( SmallIcon("bookmark_add"), i18n("Add Bookmark"), 1 );
		popup->insertItem( SmallIcon("viewmagfit"), i18n("Fit Width"), 2 );
		//popup->insertItem( SmallIcon("pencil"), i18n("Edit"), 3 );
		//popup->setItemEnabled( 3, false );
		reallyShow = true;
	}
/*
	//Albert says: I have not ported this as i don't see it does anything
    if ( d->mouseOnRect ) // and rect->pointerType() == KDPFPageRect::Image ...
	{
		m_popup->insertItem( SmallIcon("filesave"), i18n("Save Image ..."), 4 );
		m_popup->setItemEnabled( 4, false );
}*/
	
	if ((m_showMenuBarAction && !m_showMenuBarAction->isChecked()) || (m_showFullScreenAction && m_showFullScreenAction->isChecked()))
	{
		popup->insertTitle( i18n( "Tools" ) );
		if (m_showMenuBarAction && !m_showMenuBarAction->isChecked()) m_showMenuBarAction->plug(popup);
		if (m_showFullScreenAction && m_showFullScreenAction->isChecked()) m_showFullScreenAction->plug(popup);
		reallyShow = true;
		
	}
	
	if (reallyShow)
	{
		switch ( popup->exec(point) )
		{
			case 1:
				m_document->toggleBookmark( page->number() );
				break;
			case 2: // zoom: Fit Width, columns: 1. setActions + relayout + setPage + update
				// (FIXME restore faster behavior and txt change as in old pageview implementation)
				m_pageView->setZoomFitWidth();
				m_document->setCurrentPage( page->number() );
				break;
	//		case 3: // ToDO switch to edit mode
	//			m_pageView->slotSetMouseDraw();
	//			break;
		}
	}
	delete popup;
}

void Part::slotShowProperties()
{
	propertiesDialog *d = new propertiesDialog(widget(), m_document);
	d->exec();
	delete d;
}

void Part::slotShowPresentation()
{
    new PresentationWidget( m_document );
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
        if (page->rotation() == 90 || page->rotation() == 270) qSwap(width, height);
        if (width > height) landscape++;
        else portrait++;
    }
    if (landscape > portrait) printer.setOrientation(KPrinter::Landscape);

    if (printer.setup(widget())) doPrint( printer );
}

void Part::doPrint(KPrinter &printer)
{
    if (!m_document->okToPrint())
    {
        KMessageBox::error(widget(), i18n("Printing this document is not allowed."));
        return;
    }

    if (!m_document->print(printer))
    {
        KMessageBox::error(widget(), i18n("Could not print the document. Please report to bugs.kde.org"));	
    }
}

void Part::restoreDocument(const KURL &url, int page)
{
  if (openURL(url)) goToPage(page);
}

void Part::saveDocumentRestoreInfo(KConfig* config)
{
  config->writePathEntry( "URL", url().url() );
  if (m_document->pages() > 0) config->writeEntry( "Page", m_document->currentPage() + 1 );
}

/*
* BrowserExtension class
*/
BrowserExtension::BrowserExtension(Part* parent)
  : KParts::BrowserExtension( parent, "KPDF::BrowserExtension" )
{
	emit enableAction("print", true);
	setURLDropHandlingEnabled(true);
}

void BrowserExtension::print()
{
	static_cast<Part*>(parent())->slotPrint();
}

#include "part.moc"
