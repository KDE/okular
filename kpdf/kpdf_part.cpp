/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2002 by Chris Cheney <ccheney@cheney.cx>                *
 *   Copyright (C) 2002 by Malcolm Hunter <malcolm.hunter@gmx.co.uk>       *
 *   Copyright (C) 2003-2004 by Christophe Devriese                        *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Daniel Molkentin <molkentin@kde.org>            *
 *   Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                  *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2004 by Dominique Devriese <devriese@kde.org>           *
 *   Copyright (C) 2004 by Christoph Cullmann <crossfire@babylon2k.de>     *
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

#include <qwidget.h>
#include <qlistbox.h>
#include <qfile.h>
#include <qpainter.h>
#include <qtimer.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kprinter.h>
#include <kstdaction.h>
#include <kconfig.h>
#include <kparts/genericfactory.h>
#include <kurldrag.h>
#include <kinputdialog.h>
#include <kfiledialog.h>
#include <kfinddialog.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kuserprofile.h>
#include <kpassdlg.h>
#include <kio/netaccess.h>
#include <ktempfile.h>

#include "kpdf_error.h"
#include "part.h"


#include "GString.h"

#include "gfile.h"
#include "Error.h"
#include "xpdf/ErrorCodes.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "PSOutputDev.h"
#include "TextOutputDev.h"
#include "QOutputDevPixmap.h"

#include "kpdf_pagewidget.h"

typedef KParts::GenericFactory<KPDF::Part> KPDFPartFactory;
K_EXPORT_COMPONENT_FACTORY(libkpdfpart, KPDFPartFactory)

using namespace KPDF;

unsigned int Part::m_count = 0;

Part::Part(QWidget *parentWidget, const char *widgetName,
           QObject *parent, const char *name,
           const QStringList & /*args*/ )
  : KParts::ReadOnlyPart(parent, name),
    m_doc(0),
    m_currentPage(0),
    m_zoomMode(FixedFactor),
    m_zoomFactor(1.0)
{
  new BrowserExtension(this);

  globalParams = new GlobalParams("");
  globalParams->setupBaseFonts(NULL);

  // we need an instance
  setInstance(KPDFPartFactory::instance());

  pdfpartview = new PDFPartView(parentWidget, widgetName, &m_docMutex);

  connect(pdfpartview, SIGNAL( clicked ( int ) ), this, SLOT( pageClicked ( int ) ));
  connect(pdfpartview, SIGNAL( execute ( LinkAction* ) ), this, SLOT( executeAction ( LinkAction* ) ));
  connect(pdfpartview, SIGNAL( hasTOC ( bool ) ), this, SLOT( hasTOC ( bool ) ));

  m_outputDev = pdfpartview->outputdev;
  connect(m_outputDev, SIGNAL(linkClicked(LinkAction*)), this, SLOT(executeAction(LinkAction*)));
  m_outputDev->setAcceptDrops( true );

  setWidget(pdfpartview);

  m_showScrollBars = new KToggleAction( i18n( "Show &Scrollbars" ), 0,
                                       actionCollection(), "show_scrollbars" );
  m_showScrollBars->setCheckedState(i18n("Hide &Scrollbars"));
  m_showTOC = new KToggleAction( i18n( "Show &Table of Contents" ), 0,
                                       actionCollection(), "show_toc" );
  m_showTOC->setCheckedState(i18n("Hide &Table of Contents"));
  m_showTOC->setEnabled(false);
  m_showPageList   = new KToggleAction( i18n( "Show &Page List" ), 0,
                                       actionCollection(), "show_page_list" );
  m_showPageList->setCheckedState(i18n("Hide &Page List"));
  connect( m_showScrollBars, SIGNAL( toggled( bool ) ),
             SLOT( showScrollBars( bool ) ) );
  connect( m_showTOC, SIGNAL( toggled( bool ) ),
             SLOT( showTOC( bool ) ) );
  connect( m_showPageList, SIGNAL( toggled( bool ) ),
             SLOT( showMarkList( bool ) ) );

  // create our actions
  KStdAction::saveAs(this, SLOT(fileSaveAs()), actionCollection(), "save");
  m_find = KStdAction::find(this, SLOT(find()),
                       actionCollection(), "find");
  m_find->setEnabled(false);
  m_findNext = KStdAction::findNext(this, SLOT(findNext()),
                       actionCollection(), "find_next");
  m_findNext->setEnabled(false);
  m_fitToWidth = new KToggleAction(i18n("Fit to Page &Width"), 0,
                       this, SLOT(slotFitToWidthToggled()),
                       actionCollection(), "fit_to_width");
  KStdAction::zoomIn  (m_outputDev, SLOT(zoomIn()),
                       actionCollection(), "zoom_in");
  KStdAction::zoomOut (m_outputDev, SLOT(zoomOut()),
                       actionCollection(), "zoom_out");

  KStdAction::back    (this, SLOT(back()),
                       actionCollection(), "back");
  KStdAction::forward (this, SLOT(forward()),
                       actionCollection(), "forward");

  KStdAction::printPreview( this, SLOT( printPreview() ), actionCollection() );

  m_prevPage = KStdAction::prior(this, SLOT(slotPreviousPage()),
                       actionCollection(), "previous_page");
  m_prevPage->setWhatsThis( i18n( "Moves to the previous page of the document" ) );

  m_nextPage = KStdAction::next(this, SLOT(slotNextPage()),
                       actionCollection(), "next_page" );
  m_nextPage->setWhatsThis( i18n( "Moves to the next page of the document" ) );

  m_firstPage = KStdAction::firstPage( this, SLOT( slotGotoStart() ),
                                      actionCollection(), "goToStart" );
  m_firstPage->setWhatsThis( i18n( "Moves to the first page of the document" ) );

  m_lastPage  = KStdAction::lastPage( this, SLOT( slotGotoEnd() ),
                                     actionCollection(), "goToEnd" );
  m_lastPage->setWhatsThis( i18n( "Moves to the last page of the document" ) );

  m_gotoPage = KStdAction::gotoPage( this, SLOT( slotGoToPage() ),
                                    actionCollection(), "goToPage" );

  const double zoomValue[14] = {0.125,0.25,0.3333,0.5,0.6667,0.75,1,1.25,1.50,2,3,4,6,8 };

  m_zoomTo = new KSelectAction(  i18n( "Zoom" ), "zoomTo", 0, actionCollection(), "zoomTo" );
  connect(  m_zoomTo, SIGNAL(  activated(  const QString & ) ), this, SLOT(  slotZoom( const QString& ) ) );
  m_zoomTo->setEditable(  true );
  m_zoomTo->clear();

  QStringList translated;
  QString localValue;
  QString double_oh("00");
  int idx = 0;
  int cur = 0;
  for ( int i = 0; i < 10;i++)
  {
      localValue = KGlobal::locale()->formatNumber( zoomValue[i] * 100.0, 2 );
      localValue.remove( KGlobal::locale()->decimalSymbol()+double_oh );

      translated << QString( "%1%" ).arg( localValue );
      if ( zoomValue[i] == 1.0 )
          idx = cur;
      ++cur;
  }

  m_zoomTo->setItems( translated );
  m_zoomTo->setCurrentItem( idx );


  // set our XML-UI resource file
  setXMLFile("kpdf_part.rc");
  connect( m_outputDev, SIGNAL( ZoomIn() ), SLOT( zoomIn() ));
  connect( m_outputDev, SIGNAL( ZoomOut() ), SLOT( zoomOut() ));
  connect( m_outputDev, SIGNAL( ReadUp() ), SLOT( slotReadUp() ));
  connect( m_outputDev, SIGNAL( ReadDown() ), SLOT( slotReadDown() ));
  connect( m_outputDev, SIGNAL( urlDropped( const KURL& ) ), SLOT( slotOpenUrlDropped( const KURL & )));
  connect( m_outputDev, SIGNAL( spacePressed() ), this, SLOT( slotReadDown() ) );
  readSettings();
  updateActionPage();
  m_count++;
}

Part::~Part()
{
    m_count--;
    pdfpartview->stopThumbnailGeneration();
    writeSettings();
    if (m_count == 0) delete globalParams;
    delete m_doc;
}

void Part::slotZoom( const QString&nz )
{
    QString z = nz;
    double zoom;
    z.remove(  z.find(  '%' ), 1 );
    bool isNumber = true;
    zoom = KGlobal::locale()->readNumber(  z, &isNumber ) / 100;

    if ( isNumber )
    {
    	kdDebug() << "ZOOM = "  << nz << ", setting zoom = " << zoom << endl;
    	m_outputDev->zoomTo( zoom );
    }
}

void Part::slotGoToPage()
{
    if ( m_doc )
    {
        bool ok = false;
        int num = KInputDialog::getInteger(i18n("Go to Page"), i18n("Page:"), m_currentPage,
                                           1, m_doc->getNumPages(), 1, 10, &ok/*, _part->widget()*/);
        if (ok)
            goToPage( num );
    }
}

void Part::goToPage( int page )
{
    if (page != m_currentPage)
    {
        m_currentPage = page;
        pdfpartview->setCurrentThumbnail(m_currentPage);
        m_outputDev->setPage(m_currentPage);
        updateActionPage();
    }
}

void Part::slotOpenUrlDropped( const KURL &url )
{
    openURL(url);
}

void Part::setFullScreen( bool fs )
{
    pdfpartview->showPageList(!fs);
}


void Part::updateActionPage()
{
    if ( m_doc )
    {
        m_firstPage->setEnabled(m_currentPage>1);
        m_lastPage->setEnabled(m_currentPage<m_doc->getNumPages());
        m_prevPage->setEnabled(m_currentPage>1);
        m_nextPage->setEnabled(m_currentPage<m_doc->getNumPages());
    }
    else
    {
        m_firstPage->setEnabled(false);
        m_lastPage->setEnabled(false);
        m_prevPage->setEnabled(false);
        m_nextPage->setEnabled(false);
    }
}

void Part::slotReadUp()
{
    if( !m_doc )
	return;

    if( !m_outputDev->readUp() ) {
        if ( previousPage() )
            m_outputDev->scrollBottom();
    }
}

void Part::slotReadDown()
{
    if( !m_doc )
	return;

    if( !m_outputDev->readDown() ) {
        if ( nextPage() )
            m_outputDev->scrollTop();
    }
}

void Part::writeSettings()
{
    KConfigGroup general( KPDFPartFactory::instance()->config(), "General" );
    general.writeEntry( "ShowScrollBars", m_showScrollBars->isChecked() );
    general.writeEntry( "ShowTOC", m_showTOC->isChecked() );
    general.writeEntry( "ShowPageList", m_showPageList->isChecked() );
    general.sync();
}

void Part::readSettings()
{
    KConfigGroup general( KPDFPartFactory::instance()->config(), "General" );
    m_showScrollBars->setChecked( general.readBoolEntry( "ShowScrollBars", true ) );
    showScrollBars( m_showScrollBars->isChecked() );
    m_showTOC->setChecked( general.readBoolEntry( "ShowTOC", true ) );
    showTOC( m_showTOC->isChecked() );
    m_showPageList->setChecked( general.readBoolEntry( "ShowPageList", true ) );
    showMarkList( m_showPageList->isChecked() );
}

void Part::showScrollBars( bool show )
{
    m_outputDev->enableScrollBars( show );
}

void Part::showMarkList( bool show )
{
    pdfpartview->showPageList(show);
}

void Part::showTOC( bool show )
{
    pdfpartview->showTOC(show);
}

void Part::hasTOC( bool show )
{
    m_showTOC->setEnabled(show);
}

void Part::slotGotoEnd()
{
    if ( m_doc && m_doc->getNumPages() > 0 )
    {
        goToPage(m_doc->getNumPages());
    }
}

void Part::slotGotoStart()
{
    if ( m_doc && m_doc->getNumPages() > 0 )
    {
        goToPage(1);
     }
}

bool Part::nextPage()
{
    if ( m_doc && m_currentPage + 1 > m_doc->getNumPages()) return false;

    goToPage(m_currentPage+1);
    return true;
}

void Part::slotNextPage()
{
    nextPage();
}

void Part::slotPreviousPage()
{
    previousPage();
}

bool Part::previousPage()
{
    if (m_currentPage - 1 < 1) return false;

    goToPage(m_currentPage-1);
    return true;
}

  KAboutData*
Part::createAboutData()
{
  // the non-i18n name here must be the same as the directory in
  // which the part's rc file is installed ('partrcdir' in the
  // Makefile)
  KAboutData* aboutData = new KAboutData("kpdfpart", I18N_NOOP("KPDF::Part"),
                                         "0.1");
  aboutData->addAuthor("Wilco Greven", 0, "greven@kde.org");
  return aboutData;
}

  bool
Part::closeURL()
{
  pdfpartview->stopThumbnailGeneration();
  delete m_doc;
  m_doc = 0;

  return KParts::ReadOnlyPart::closeURL();
}

  bool
Part::openFile()
{
  // m_file is always local so we can use QFile on it
  QFile file(m_file);
  if (file.open(IO_ReadOnly) == false)
    return false;

  GString* filename = new GString( QFile::encodeName( m_file ) );
  m_doc = new PDFDoc(filename, 0, 0);

  if (!m_doc->isOk())
  {
    if (m_doc->getErrorCode() == errEncrypted)
    {
      bool first, correct;
      QString prompt;
      QCString pwd;

      first = true;
      correct = false;
      while(!correct)
      {
        if (first)
        {
          prompt = i18n("Please insert the password to read the document:");
          first = false;
        }
        else prompt = i18n("Incorrect password. Try again:");
        if (KPasswordDialog::getPassword(pwd, prompt) == KPasswordDialog::Accepted)
        {
          GString *pwd2;
          pwd2 = new GString(pwd.data());
          m_doc = new PDFDoc(filename, pwd2, pwd2);
          delete pwd2;
          correct = m_doc->isOk();
          if (!correct && m_doc->getErrorCode() != errEncrypted) return false;
        }
        else return false;
      }
    }
    else return false;
  }

  m_find->setEnabled(true);
  m_findNext->setEnabled(true);

  errors::clear();
  m_currentPage = 0; //so that the if in goToPage is true
  if (m_doc->getNumPages() > 0)
  {
    // TODO use a qvaluelist<int> to pass aspect ratio?
    // TODO move it to inside pdfpartview or even the thumbnail list itself?
    // TODO take page rotation into acount for calculating aspect ratio
    pdfpartview->setPages(m_doc->getNumPages(), m_doc->getPageHeight(1)/m_doc->getPageWidth(1));
    pdfpartview->generateThumbnails(m_doc);
    pdfpartview->generateTOC(m_doc);

    m_outputDev->setPDFDocument(m_doc);
    goToPage(1);

  }

  return true;
}

bool Part::openURL(const KURL &url)
{
	bool b;
	b = KParts::ReadOnlyPart::openURL(url);
	if (!b)
	{
		KMessageBox::error(widget(), i18n("Could not open %1").arg(url.prettyURL()));
	}
	return b;
}

  void
Part::fileSaveAs()
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

  void
Part::displayPage(int pageNumber, float /*zoomFactor*/)
{
    if (pageNumber <= 0 || pageNumber > m_doc->getNumPages())
        return;
    updateActionPage();
    const double pageWidth  = m_doc->getPageWidth (pageNumber) * m_zoomFactor;
    const double pageHeight = m_doc->getPageHeight(pageNumber) * m_zoomFactor;

    // Pixels per point when the zoomFactor is 1.
    const float basePpp  = QPaintDevice::x11AppDpiX() / 72.0;

    switch (m_zoomMode)
    {
    case FitWidth:
    {
        const double pageAR = pageWidth/pageHeight; // Aspect ratio

        const int canvasWidth    = m_outputDev->contentsRect().width();
        const int canvasHeight   = m_outputDev->contentsRect().height();
        const int scrollBarWidth = m_outputDev->verticalScrollBar()->width();

        // Calculate the height so that the page fits the viewport width
        // assuming that we need a vertical scrollbar.
        float height = float(canvasWidth - scrollBarWidth) / pageAR;

        // If the vertical scrollbar wasn't needed after all, calculate the page
        // size so that the page fits the viewport width without the scrollbar.
        if (ceil(height) <= canvasHeight)
        {
            height = float(canvasWidth) / pageAR;

            // Handle the rare case that enlarging the page resulted in the need of
            // a vertical scrollbar. We can fit the page to the viewport height in
            // this case.
            if (ceil(height) > canvasHeight)
                height = float(canvasHeight) * pageAR;
        }

        m_zoomFactor = (height / pageHeight) / basePpp;
        break;
    }
    case FixedFactor:
    default:
        break;
    }

//const float ppp = basePpp * m_zoomFactor; // pixels per point

//  m_doc->displayPage(m_outputDev, pageNumber, int(m_zoomFactor * ppp * 72.0), 0, true);

//  m_outputDev->show();

//  m_currentPage = pageNumber;
}

  void
Part::displayDestination(LinkDest* dest)
{
  Ref pageRef;
  int pg;

  if (dest->isPageRef())
  {
    pageRef = dest->getPageRef();
    pg = m_doc->findPage(pageRef.num, pageRef.gen);
  }
  else
  {
    pg = dest->getPageNum();
  }
  if (pg <= 0 || pg > m_doc->getNumPages()) pg = 1;
  if (pg != m_currentPage) goToPage(pg);

  m_outputDev->position(dest);
}

  void
Part::print()
{
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
}

/*
  void
Part::setFixedZoomFactor(float zoomFactor)
{

}
*/

void
Part::executeAction(LinkAction* action)
{
	LinkActionKind kind;
	LinkDest *dest;
	GString *namedDest;
	char *s;
	GString *fileName, *fileName2;
// 	GString *cmd;
	GString *actionName;
	Object movieAnnot, obj1, obj2;
	int i;
	KService::Ptr ptr;
	KURL::List lst;

	kind = action -> getKind();
	// copied from xpdf have a look and understand a bit :D
	switch (kind)
	{
		// GoTo / GoToR action
		case actionGoTo:
		case actionGoToR:
			if (kind == actionGoTo)
			{
				dest = NULL;
				namedDest = NULL;
				if ((dest = ((LinkGoTo *)action)->getDest()))
				{
					dest = dest->copy();
				}
				else if ((namedDest = ((LinkGoTo *)action)->getNamedDest()))
				{
					namedDest = namedDest->copy();
				}
			}
			else
			{
				dest = NULL;
				namedDest = NULL;
				if ((dest = ((LinkGoToR *)action)->getDest()))
				{
					dest = dest->copy();
				}
				else if ((namedDest = ((LinkGoToR *)action)->getNamedDest()))
				{
					namedDest = namedDest->copy();
				}
				s = ((LinkGoToR *)action)->getFileName()->getCString();
				//~ translate path name for VMS (deal with '/')
				if (isAbsolutePath(s))
				{
					fileName = new GString(s);
				}
				else
				{
					fileName = appendToPath(grabPath(m_doc->getFileName()->getCString()), s);
				}
				if (!openURL(fileName->getCString()))
				{
					if (dest)
					{
						delete dest;
					}
					if (namedDest)
					{
						delete namedDest;
					}
					delete fileName;
					return;
				}
				delete fileName;
			}
			if (namedDest)
			{
				dest = m_doc->findDest(namedDest);
				delete namedDest;
			}
			if (dest)
			{
				displayDestination(dest);
				delete dest;
			}
			else
			{
				if (kind == actionGoToR) goToPage(1);
			}
		break;

		// Launch action
		case actionLaunch:
			fileName = ((LinkLaunch *)action)->getFileName();
			s = fileName->getCString();
			if (!strcmp(s + fileName->getLength() - 4, ".pdf") ||
			    !strcmp(s + fileName->getLength() - 4, ".PDF"))
			{
				//~ translate path name for VMS (deal with '/')
				if (isAbsolutePath(s))
				{
					fileName = fileName->copy();
				}
				else
				{
					fileName = appendToPath(grabPath(m_doc->getFileName()->getCString()), s);
				}
				if (!openURL(fileName->getCString()))
				{
					delete fileName;
					return;
				}
				delete fileName;
				goToPage(1);
			}
			else
			{
				KMessageBox::information(widget(), i18n("The pdf file is trying to execute an external application and for your safety kpdf does not allow that."));
				/* core developers say this is too dangerous
				fileName = fileName->copy();
				if (((LinkLaunch *)action)->getParams())
				{
					fileName->append(' ');
					fileName->append(((LinkLaunch *)action)->getParams());
				}
				fileName->append(" &");
				if (KMessageBox::questionYesNo(widget(), i18n("Do you want to execute the command:\n%1").arg(fileName->getCString()), i18n("Launching external application")) == KMessageBox::Yes)
				{
					system(fileName->getCString());
				}*/
				delete fileName;
			}
		break;

		// URI action
		case actionURI:
			ptr = KServiceTypeProfile::preferredService("text/html", "Application");
			lst.append(((LinkURI *)action)->getURI()->getCString());
			KRun::run(*ptr, lst);
		break;

		// Named action
		case actionNamed:
			actionName = ((LinkNamed *)action)->getName();
			if (!actionName->cmp("NextPage")) nextPage();
			else if (!actionName->cmp("PrevPage")) previousPage();
			else if (!actionName->cmp("FirstPage")) goToPage(1);
			else if (!actionName->cmp("LastPage")) goToPage(m_doc->getNumPages());
			else if (!actionName->cmp("GoBack"))
			{
				// TODO i think that means back in history not in page, check i XPDFCore.cc
				//goBackward();
			}
			else if (!actionName->cmp("GoForward"))
			{
				// TODO i think that means forward in history not in page, check i XPDFCore.cc
				// goForward();
			}
			// TODO is this legal if integrated on konqy?
			else if (!actionName->cmp("Quit")) kapp -> quit();
			else
			{
				error(-1, "Unknown named action: '%s'", actionName->getCString());
			}
		break;

		// Movie action
		case actionMovie:
			if (((LinkMovie *)action)->hasAnnotRef())
			{
				m_doc->getXRef()->fetch(((LinkMovie *)action)->getAnnotRef()->num,
				                      ((LinkMovie *)action)->getAnnotRef()->gen,
				                      &movieAnnot);
			}
			else
			{
				m_doc->getCatalog()->getPage(m_currentPage)->getAnnots(&obj1);
				if (obj1.isArray())
				{
					for (i = 0; i < obj1.arrayGetLength(); ++i)
					{
						if (obj1.arrayGet(i, &movieAnnot)->isDict())
						{
							if (movieAnnot.dictLookup("Subtype", &obj2)->isName("Movie"))
							{
								obj2.free();
								break;
							}
							obj2.free();
						}
						movieAnnot.free();
					}
					obj1.free();
				}
			}
			if (movieAnnot.isDict())
			{
				if (movieAnnot.dictLookup("Movie", &obj1)->isDict())
				{
					if (obj1.dictLookup("F", &obj2))
					{
						if ((fileName = LinkAction::getFileSpecName(&obj2)))
						{
							if (!isAbsolutePath(fileName->getCString()))
							{
								fileName2 = appendToPath(grabPath(m_doc->getFileName()->getCString()),
								                         fileName->getCString());
								delete fileName;
								fileName = fileName2;
							}
							// TODO implement :D
							// get the mimetype of fileName and then run KServiceTypeProfile
							// preferredService on it
							// Having a file where test would be cool
							//runCommand(cmd, fileName);
							delete fileName;
						}
						obj2.free();
					}
					obj1.free();
				}
			}
			movieAnnot.free();
		break;

		// unknown action type
		case actionUnknown:
			error(-1, "Unknown link action type: '%s'", ((LinkUnknown *)action)->getAction()->getCString());
		break;
	}
}
void Part::slotFitToWidthToggled()
{
  m_zoomMode = m_fitToWidth->isChecked() ? FitWidth : FixedFactor;
  displayPage(m_currentPage);
}

// for optimization
bool redrawing = false;

void Part::update()
{
	if (m_outputDev && ! redrawing)
	{
		redrawing = true;
		QTimer::singleShot(200, this, SLOT( redrawPage() ));
	}
}

void Part::redrawPage()
{
	redrawing = false;
	displayPage(m_currentPage);
}

void Part::pageClicked ( int i )
{
    // ThumbnailList is 0 based
    goToPage(i+1);
}

BrowserExtension::BrowserExtension(Part* parent)
  : KParts::BrowserExtension( parent, "KPDF::BrowserExtension" )
{
  emit enableAction("print", true);
  setURLDropHandlingEnabled(true);
}

  void
BrowserExtension::print()
{
  static_cast<Part*>(parent())->print();
}

void Part::printPreview()
{
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
}

void Part::doPrint( KPrinter& printer )
{
  if (!m_doc->okToPrint())
  {
    KMessageBox::error(widget(), i18n("Printing this document is not allowed."));
    return;
  }

  KTempFile tf( QString::null, ".ps" );
  PSOutputDev *psOut = new PSOutputDev(tf.name().latin1(), m_doc->getXRef(), m_doc->getCatalog(), 1, m_doc->getNumPages(), psModePS);

  if (psOut->isOk())
  {
    std::list<int> pages;

    if (!printer.previewOnly())
    {
      QValueList<int> pageList = printer.pageList();
      QValueList<int>::const_iterator it;

      for(it = pageList.begin(); it != pageList.end(); ++it) pages.push_back(*it);
    }
    else
    {
      for(int i = 1; i <= m_doc->getNumPages(); i++) pages.push_back(i);
    }

    m_docMutex.lock();
    m_doc->displayPages(psOut, pages, 72, 72, 0, globalParams->getPSCrop(), gFalse);
    m_docMutex.unlock();

    // needs to be here so that the file is flushed, do not merge with the one
    // in the else
    delete psOut;

    printer.printFiles(tf.name(), true);
  }
  else
  {
    KMessageBox::error(widget(), i18n("Could not print the document. Please report to bugs.kde.org"));
    delete psOut;
  }
}

void Part::find()
{
  KFindDialog dlg(pdfpartview);
  dlg.setHasCursor(false);
  dlg.setSupportsBackwardsFind(false);
  dlg.setSupportsCaseSensitiveFind(false);
  dlg.setSupportsWholeWordsFind(false);
  dlg.setSupportsRegularExpressionFind(false);
  if (dlg.exec() != QDialog::Accepted) return;

  doFind(dlg.pattern(), false);
}

void Part::findNext()
{
  if (!m_findText.isEmpty()) doFind(m_findText, true);
}

void Part::doFind(const QString &s, bool next)
{
  TextOutputDev *textOut;
  Unicode *u;
  bool found;
  double xMin1, yMin1, xMax1, yMax1;
  int len, pg;

  // This is more or less copied from what xpdf does, surely can be optimized
  len = s.length();
  u = (Unicode *)gmalloc(len * sizeof(Unicode));
  for (int i = 0; i < len; ++i) u[i] = (Unicode)(s.latin1()[i] & 0xff);

  // search current
  found = m_outputDev->find(u, len, next);

  if (!found)
  {
    // search following pages
    textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
    if (!textOut->isOk())
    {
      gfree(u);
      delete textOut;
      return;
    }

    pg = m_currentPage + 1;
    while(!found && pg <= m_doc->getNumPages())
    {
      m_docMutex.lock();
      m_doc->displayPage(textOut, pg, 72, 72, 0, gTrue, gFalse);
      m_docMutex.unlock();
      found = textOut->findText(u, len, gTrue, gTrue, gFalse, gFalse, &xMin1, &yMin1, &xMax1, &yMax1);
      if (!found) pg++;
    }

    if (!found && m_currentPage != 1)
    {
      if (KMessageBox::questionYesNo(widget(), i18n("End of document reached.\nContinue from the beginning?")) == KMessageBox::Yes)
      {
        // search previous pages
        pg = 1;
        while(!found && pg < m_currentPage)
        {
          m_docMutex.lock();
          m_doc->displayPage(textOut, pg, 72, 72, 0, gTrue, gFalse);
          m_docMutex.unlock();
          found = textOut->findText(u, len, gTrue, gTrue, gFalse, gFalse, &xMin1, &yMin1, &xMax1, &yMax1);
          if (!found) pg++;
        }
      }
    }

    delete textOut;
    if (found)
    {
       kdDebug() << "found at " << pg << endl;
       goToPage(pg);
       // xpdf says: can happen that we do not find the text if coalescing is bad OUCH
       m_outputDev->find(u, len, false);
    }
    else
    {
        if (next) KMessageBox::information(widget(), i18n("No more matches found for '%1'.").arg(s));
        else KMessageBox::information(widget(), i18n("No matches found for '%1'.").arg(s));
    }
  }

  if (found) m_findText = s;
  else m_findText = QString::null;

  gfree(u);
}

// vim:ts=2:sw=2:tw=78:et
