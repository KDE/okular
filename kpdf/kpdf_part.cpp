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

#include "kpdf_part.moc"

#include <math.h>

#include <qsplitter.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qlabel.h>

#include <kaction.h>
#include <kinstance.h>
#include <kprinter.h>
#include <kstdaction.h>
#include <kconfig.h>
#include <kparts/genericfactory.h>
#include <kurldrag.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <kfinddialog.h>
#include <knuminput.h>

#include "kpdf_error.h"

#include "GString.h"

#include "GlobalParams.h"
#include "QOutputDevKPrinter.h"

#include "thumbnaillist.h"
#include "kpdf_pagewidget.h"
#include "document.h"

typedef KParts::GenericFactory<KPDF::Part> KPDFPartFactory;
K_EXPORT_COMPONENT_FACTORY(libkpdfpart, KPDFPartFactory)

using namespace KPDF;

unsigned int Part::m_count = 0;

Part::Part(QWidget *parentWidget, const char *widgetName,
           QObject *parent, const char *name,
           const QStringList & /*args*/ )
  : KParts::ReadOnlyPart(parent, name)
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
	document = new KPDFDocument();
	connect( document, SIGNAL( pageChanged() ), this, SLOT( updateActions() ) );

	// build widgets
	m_splitter = new QSplitter( parentWidget, widgetName );
	m_splitter->setOpaqueResize( true );
	setWidget( m_splitter );

	m_thumbnailList = new ThumbnailList( m_splitter, document );
	m_thumbnailList->setMaximumWidth( 125 );
	m_thumbnailList->setMinimumWidth( 50 );
	document->addObserver( m_thumbnailList );

	m_pageWidget = new PageWidget( m_splitter, document );
	connect( m_pageWidget, SIGNAL( urlDropped( const KURL& ) ), SLOT( openURL( const KURL & )));
	//connect(m _pageWidget, SIGNAL( rightClick() ), this, SIGNAL( rightClick() ));
	document->addObserver( m_pageWidget );

	// ACTIONS
	KActionCollection * ac = actionCollection();

	// Page Traversal actions
	m_gotoPage = KStdAction::gotoPage( this, SLOT( slotGoToPage() ), ac, "goto_page" );
	m_gotoPage->setShortcut( "CTRL+G" );

	m_prevPage = KStdAction::prior(this, SLOT(slotPreviousPage()), ac, "previous_page");
	m_prevPage->setWhatsThis( i18n( "Moves to the previous page of the document" ) );

	m_nextPage = KStdAction::next(this, SLOT(slotNextPage()), ac, "next_page" );
	m_nextPage->setWhatsThis( i18n( "Moves to the next page of the document" ) );

	m_firstPage = KStdAction::firstPage( this, SLOT( slotGotoFirst() ), ac, "first_page" );
	m_firstPage->setWhatsThis( i18n( "Moves to the first page of the document" ) );

	m_lastPage  = KStdAction::lastPage( this, SLOT( slotGotoLast() ), ac, "last_page" );
	m_lastPage->setWhatsThis( i18n( "Moves to the last page of the document" ) );

	// Find and other actions
	m_find = KStdAction::find( this, SLOT( slotFind() ), ac, "find" );
	m_find->setEnabled(false);

	m_findNext = KStdAction::findNext( this, SLOT( slotFindNext() ), ac, "find_next" );
	m_findNext->setEnabled(false);

	KStdAction::saveAs( this, SLOT( slotSaveFileAs() ), ac, "save" );

	KStdAction::printPreview( this, SLOT( slotPrintPreview() ), ac );

    // attach the actions of the 2 children widgets too
	KConfigGroup settings( KPDFPartFactory::instance()->config(), "General" );
	m_pageWidget->setupActions( ac, &settings );
	m_thumbnailList->setupActions( ac, &settings );

	// local settings
	m_splitter->setSizes( settings.readIntListEntry( "SplitterSizes" ) );

	// set our XML-UI resource file
	setXMLFile("kpdf_part.rc");
	updateActions();
}

Part::~Part()
{
	KConfigGroup settings( KPDFPartFactory::instance()->config(), "General" );
	m_pageWidget->saveSettings( &settings );
	m_thumbnailList->saveSettings( &settings );
	settings.writeEntry( "SplitterSizes", m_splitter->sizes() );
	settings.sync();
	delete document;
	if ( --m_count == 0 )
		delete globalParams;
}

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
	bool ok = document->openFile( m_file );
	m_find->setEnabled( ok );
	return ok;
}

bool Part::closeURL()
{
	document->close();
	return KParts::ReadOnlyPart::closeURL();
}

void Part::updateActions()
{
	if ( document->pages() > 0 )
	{
		m_gotoPage->setEnabled(document->pages()>1);
		m_firstPage->setEnabled(!document->atBegin());
		m_prevPage->setEnabled(!document->atBegin());
		m_lastPage->setEnabled(!document->atEnd());
		m_nextPage->setEnabled(!document->atEnd());
	}
	else
	{
		m_gotoPage->setEnabled(false);
		m_firstPage->setEnabled(false);
		m_lastPage->setEnabled(false);
		m_prevPage->setEnabled(false);
		m_nextPage->setEnabled(false);
	}
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
	KPDFGotoPageDialog pageDialog( m_pageWidget, document->currentPage() + 1, document->pages() );
	if ( pageDialog.exec() == QDialog::Accepted )
		document->slotSetCurrentPage( pageDialog.getPage() - 1 );
}

void Part::slotPreviousPage()
{
	if ( !document->atBegin() )
		document->slotSetCurrentPage( document->currentPage() - 1 );
}

void Part::slotNextPage()
{
	if ( !document->atEnd() )
		document->slotSetCurrentPage( document->currentPage() + 1 );
}

void Part::slotGotoFirst()
{
	document->slotSetCurrentPage( 0 );
}

void Part::slotGotoLast()
{
	document->slotSetCurrentPage( document->pages() - 1 );
}

void Part::slotFind()
{
	KFindDialog dlg( widget() );
	if (dlg.exec() == QDialog::Accepted)
	{
		m_findNext->setEnabled( true );
		document->slotFind( dlg.pattern(), dlg.options() );
	}
}

void Part::slotFindNext()
{
	document->slotFind();
}

void Part::slotSaveFileAs()
{
  KURL saveURL = KFileDialog::getSaveURL(
					 url().isLocalFile()
					 ? url().url()
					 : url().fileName(),
					 QString::null,
					 widget(),
					 QString::null );
  if( !KIO::NetAccess::upload( url().path(),
			       saveURL, static_cast<QWidget*>( 0 ) ) )
	; // TODO: Proper error dialog
}


/*
void Part::displayDestination(LinkDest* dest)
{
  int pageNumber;
  // int dx, dy;

  if (dest->isPageRef())
  {
    Ref pageRef = dest->getPageRef();
    pageNumber = m_doc->findPage(pageRef.num, pageRef.gen);
  }
  else
  {
    pageNumber = dest->getPageNum();
  }

  if (pageNumber <= 0 || pageNumber > m_doc->getNumPages())
  {
    pageNumber = 1;
  }

  displayPage(pageNumber);
  return;

  if (fullScreen) {
    return;
  }
  switch (dest->getKind()) {
  case destXYZ:
    out->cvtUserToDev(dest->getLeft(), dest->getTop(), &dx, &dy);
    if (dest->getChangeLeft() || dest->getChangeTop()) {
      if (dest->getChangeLeft()) {
	hScrollbar->setPos(dx, canvas->getWidth());
      }
      if (dest->getChangeTop()) {
	vScrollbar->setPos(dy, canvas->getHeight());
      }
      canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    }
    //~ what is the zoom parameter?
    break;
  case destFit:
  case destFitB:
    //~ do fit
    hScrollbar->setPos(0, canvas->getWidth());
    vScrollbar->setPos(0, canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  case destFitH:
  case destFitBH:
    //~ do fit
    out->cvtUserToDev(0, dest->getTop(), &dx, &dy);
    hScrollbar->setPos(0, canvas->getWidth());
    vScrollbar->setPos(dy, canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  case destFitV:
  case destFitBV:
    //~ do fit
    out->cvtUserToDev(dest->getLeft(), 0, &dx, &dy);
    hScrollbar->setPos(dx, canvas->getWidth());
    vScrollbar->setPos(0, canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  case destFitR:
    //~ do fit
    out->cvtUserToDev(dest->getLeft(), dest->getTop(), &dx, &dy);
    hScrollbar->setPos(dx, canvas->getWidth());
    vScrollbar->setPos(dy, canvas->getHeight());
    canvas->scroll(hScrollbar->getPos(), vScrollbar->getPos());
    break;
  }
}*/

/*
void Part::executeAction(LinkAction* action)
{
  if (action == 0)
    return;

  LinkActionKind kind = action->getKind();

  switch (kind)
  {
  case actionGoTo:
  case actionGoToR:
  {
    LinkDest* dest = 0;
    GString* namedDest = 0;

    if (kind == actionGoTo)
    {
      if ((dest = ((LinkGoTo*)action)->getDest()))
        dest = dest->copy();
      else if ((namedDest = ((LinkGoTo*)action)->getNamedDest()))
        namedDest = namedDest->copy();
    }

    else
    {
      if ((dest = ((LinkGoToR*)action)->getDest()))
        dest = dest->copy();
      else if ((namedDest = ((LinkGoToR*)action)->getNamedDest()))
        namedDest = namedDest->copy();
      s = ((LinkGoToR*)action)->getFileName()->getCString();
      //~ translate path name for VMS (deal with '/')
      if (!loadFile(fileName))
      {
        delete dest;
        delete namedDest;
        return;
      }
    }


    if (namedDest != 0)
    {
      dest = m_doc->findDest(namedDest);
      delete namedDest;
    }
    if (dest != 0)
    {
      displayDestination(dest);
      delete dest;
    }
    else
    {
      if (kind == actionGoToR)
        displayPage(1);
    }
    break;
  }
  default:
      break;
  }
}*/

void Part::slotPrint()
{
/*
  if (m_doc == 0)
    return;

  double width, height;
  int landscape, portrait;
  KPrinter printer;
  
  printer.setPageSelection(KPrinter::ApplicationSide);
  printer.setMinMax(1, m_doc->getNumPages());
  printer.setCurrentPage(m_currentPage);
  printer.setMargins(0, 0, 0, 0);
  
  // if some pages are landscape and others are not the most common win as kprinter does
  // not accept a per page setting
  landscape = 0;
  portrait = 0;
  for (int i = 1; i <= m_doc->getNumPages(); i++)
  {
    width = m_doc->getPageWidth(i);
    height = m_doc->getPageHeight(i);
    if (m_doc->getPageRotate(i) == 90 || m_doc->getPageRotate(i) == 270) qSwap(width, height);
    if (width > height) landscape++;
    else portrait++;
  }
  if (landscape > portrait) printer.setOrientation(KPrinter::Landscape);
  
  if (printer.setup(widget()))
  {
    doPrint( printer );
  }
*/
}

void Part::slotPrintPreview()
{
/*
  if (m_doc == 0)
    return;

  double width, height;
  int landscape, portrait;
  KPrinter printer;
  
  printer.setMinMax(1, m_doc->getNumPages());
  printer.setPreviewOnly( true );
  printer.setMargins(0, 0, 0, 0);
  
  // if some pages are landscape and others are not the most common win as kprinter does
  // not accept a per page setting
  landscape = 0;
  portrait = 0;
  for (int i = 1; i <= m_doc->getNumPages(); i++)
  {
    width = m_doc->getPageWidth(i);
    height = m_doc->getPageHeight(i);
    if (m_doc->getPageRotate(i) == 90 || m_doc->getPageRotate(i) == 270) qSwap(width, height);
    if (width > height) landscape++;
    else portrait++;
  }
  if (landscape > portrait) printer.setOption("orientation-requested", "4");
  
  doPrint(printer);
*/
}

void Part::doPrint( KPrinter& /*printer*/ )
{
/*
  QPainter painter( &printer );
  SplashColor paperColor;
  paperColor.rgb8 = splashMakeRGB8(0xff, 0xff, 0xff);
  QOutputDevKPrinter printdev( painter, paperColor, printer );
  printdev.startDoc(m_doc->getXRef());
  QValueList<int> pages = printer.pageList();
  
  for ( QValueList<int>::ConstIterator i = pages.begin(); i != pages.end();)
  {
    m_docMutex.lock();
    m_doc->displayPage(&printdev, *i, printer.resolution(), printer.resolution(), 0, true, true);
    if ( ++i != pages.end() )
      printer.newPage();
    m_docMutex.unlock();
  }
*/
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

// vim:ts=2:sw=2:tw=78:et
