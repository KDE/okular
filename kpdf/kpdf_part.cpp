#include "kpdf_part.moc"

#include <math.h>

#include <qwidget.h>
#include <qlistbox.h>
#include <qfile.h>
#include <qtimer.h>

#include <kaction.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kprinter.h>
#include <kstdaction.h>
#include <kconfig.h>
#include <kparts/genericfactory.h>
#include <kurldrag.h>
#include <kinputdialog.h>

#include "part.h"


#include "GString.h"

#include "GlobalParams.h"
#include "PDFDoc.h"
#include "XOutputDev.h"
#include "QOutputDevKPrinter.h"

// #include "kpdf_canvas.h"
#include "kpdf_pagewidget.h"

typedef KParts::GenericFactory<KPDF::Part> KPDFPartFactory;
K_EXPORT_COMPONENT_FACTORY(libkpdfpart, KPDFPartFactory)

using namespace KPDF;

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

  // we need an instance
  setInstance(KPDFPartFactory::instance());

  pdfpartview = new PDFPartView(parentWidget, widgetName);

  connect(pdfpartview->pagesListBox, SIGNAL( currentChanged ( QListBoxItem * ) ),
          this, SLOT( pageClicked ( QListBoxItem * ) ));

  m_outputDev = pdfpartview->outputdev;
  m_outputDev->setAcceptDrops( true );


  setWidget(pdfpartview);

  m_showScrollBars = new KToggleAction( i18n( "Show &Scrollbars" ), 0,
                                       actionCollection(), "show_scrollbars" );
  m_showPageList   = new KToggleAction( i18n( "Show &Page List" ), 0,
                                       actionCollection(), "show_page_list" );
  connect( m_showScrollBars, SIGNAL( toggled( bool ) ),
             SLOT( showScrollBars( bool ) ) );
  connect( m_showPageList, SIGNAL( toggled( bool ) ),
             SLOT( showMarkList( bool ) ) );

  // create our actions
#if 0
  KStdAction::find    (this, SLOT(find()),
                       actionCollection(), "find");
  KStdAction::findNext(this, SLOT(findNext()),
                       actionCollection(), "find_next");
#endif
  m_fitToWidth = new KToggleAction(i18n("Fit to Page &Width"), 0,
                       this, SLOT(slotFitToWidthToggled()),
                       actionCollection(), "fit_to_width");
  KStdAction::zoomIn  (m_outputDev, SLOT(zoomIn()),
                       actionCollection(), "zoom_in");
  KStdAction::zoomOut (m_outputDev, SLOT(zoomOut()),
                       actionCollection(), "zoom_out");

#if 0
  KStdAction::back    (this, SLOT(back()),
                       actionCollection(), "back");
  KStdAction::forward (this, SLOT(forward()),
                       actionCollection(), "forward");
#endif

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
  int idx = 0;
  int cur = 0;
  for ( int i = 0; i < 10;i++)
  {
      translated << QString( "%1%" ).arg( zoomValue[i] * 100.0 );
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
}

Part::~Part()
{
    delete globalParams;
    writeSettings();
}

void Part::slotZoom( const QString&nz )
{
    QString z = nz;
    double zoom;
    z.remove(  z.find(  '%' ), 1 );
    zoom = KGlobal::locale()->readNumber(  z ) / 100;
    kdDebug() << "ZOOM = "  << nz << ", setting zoom = " << zoom << endl;
    m_outputDev->zoomTo( zoom );
}

void Part::slotGoToPage()
{
    if ( m_doc )
    {
        bool ok = false;
        int num = KInputDialog::getInteger(i18n("Go to Page"), i18n("Page:"), m_currentPage+1,
                                           1, m_doc->getNumPages(), 1, 10, &ok/*, _part->widget()*/);
        if (ok)
            goToPage( num );
    }
}

void Part::goToPage( int page )
{
    m_currentPage = page-1;
    pdfpartview->pagesListBox->setCurrentItem(m_currentPage);
    m_outputDev->setPage(m_currentPage+1);
    updateActionPage();
}

void Part::slotOpenUrlDropped( const KURL &url )
{
    openURL(url );
}

void Part::setFullScreen( bool fs )
{
    if ( !fs )
        pdfpartview->pagesListBox->show();
    else
        pdfpartview->pagesListBox->hide();
}


void Part::updateActionPage()
{
    if ( m_doc )
    {
        m_firstPage->setEnabled(m_currentPage!=0);
        m_lastPage->setEnabled(m_currentPage<m_doc->getNumPages()-1);
        m_prevPage->setEnabled(m_currentPage!=0);
        m_nextPage->setEnabled(m_currentPage<m_doc->getNumPages()-1);
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
    general.writeEntry( "ShowPageList", m_showPageList->isChecked() );
    general.sync();
}

void Part::readSettings()
{
    KConfigGroup general( KPDFPartFactory::instance()->config(), "General" );
    m_showScrollBars->setChecked( general.readBoolEntry( "ShowScrollBars", true ) );
    showScrollBars( m_showScrollBars->isChecked() );
    m_showPageList->setChecked( general.readBoolEntry( "ShowPageList", true ) );
    showMarkList( m_showPageList->isChecked() );
}

void Part::showScrollBars( bool show )
{
    m_outputDev->enableScrollBars( show );
}

void Part::showMarkList( bool show )
{
    if ( show )
        pdfpartview->pagesListBox->show();
    else
        pdfpartview->pagesListBox->hide();
}

void Part::slotGotoEnd()
{
    if ( m_doc && m_doc->getNumPages() > 0 );
    {
        m_currentPage = m_doc->getNumPages()-1;
        m_outputDev->setPage(m_currentPage+1);
        pdfpartview->pagesListBox->setCurrentItem(m_currentPage);
        updateActionPage();
    }
}

void Part::slotGotoStart()
{
    if ( m_doc && m_doc->getNumPages() > 0 );
    {
        m_currentPage = 0;

        m_outputDev->setPage(m_currentPage+1);
        pdfpartview->pagesListBox->setCurrentItem(m_currentPage);
        updateActionPage();
     }
}

bool Part::nextPage()
{
    m_currentPage++;
    
    if (m_doc && m_currentPage >= m_doc->getNumPages())
    {
        m_currentPage--;
        return false;
    }

    m_outputDev->setPage(m_currentPage+1);
    pdfpartview->pagesListBox->setCurrentItem(m_currentPage);
    updateActionPage();
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
    m_currentPage--;
    
    if (m_currentPage < 0)
    {
        m_currentPage++;
        return false;
    }

    m_outputDev->setPage(m_currentPage+1);
    pdfpartview->pagesListBox->setCurrentItem(m_currentPage);
    updateActionPage();
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

  GString* filename = new GString(m_file.ascii());
  m_doc = new PDFDoc(filename, 0, 0);

  if (!m_doc->isOk())
    return false;
  // just for fun, set the status bar
  // emit setStatusBarText( QString::number( m_doc->getNumPages() ) );

  // fill the listbox with entries for every page
  pdfpartview->pagesListBox->setUpdatesEnabled(false);
  pdfpartview->pagesListBox->clear();
  for (int i = 1; i <= m_doc->getNumPages(); i++)
  {
    pdfpartview->pagesListBox->insertItem(QString::number(i));
  }
  pdfpartview->pagesListBox->setUpdatesEnabled(true);
  pdfpartview->pagesListBox->update();

  displayPage(1);
  m_outputDev->setPDFDocument(m_doc);
  return true;
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
/*
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
*/
}

  void
Part::print()
{
  if (m_doc == 0)
    return;

  KPrinter printer;

  printer.setPageSelection(KPrinter::ApplicationSide);
  printer.setMinMax(1, m_doc->getNumPages());
  printer.setCurrentPage(m_currentPage+1);

  if (printer.setup(widget()))
  {
    doPrint( printer );
  }
}

  void
Part::displayNextPage()
{
  displayPage(m_currentPage + 1);
}

  void
Part::displayPreviousPage()
{
  displayPage(m_currentPage - 1);
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
    /*
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
    */

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
}

  void
Part::slotFitToWidthToggled()
{
  m_zoomMode = m_fitToWidth->isChecked() ? FitWidth : FixedFactor;
  displayPage(m_currentPage);
}

// for optimization
bool redrawing = false;

void
Part::update()
{
	if (m_outputDev && ! redrawing)
	{
		redrawing = true;
		QTimer::singleShot(200, this, SLOT( redrawPage() ));
	}
}

void
Part::redrawPage()
{
	redrawing = false;
	displayPage(m_currentPage);
}

void Part::pageClicked ( QListBoxItem * qbi )
{
    if ( !qbi )
        return;
    m_currentPage = pdfpartview->pagesListBox->index(qbi);
    
    m_outputDev->setPage(m_currentPage+1);
    updateActionPage();
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

  KPrinter printer;
  printer.setPreviewOnly( true );
  QPainter painter( &printer );
  QOutputDevKPrinter printdev( painter, printer );
  int max = m_doc->getNumPages();
  for ( int i = 1; i <= max; ++i )
  {
    m_doc->displayPage( &printdev, i, printer.resolution(), 0, true );
    if ( i != max )
      printer.newPage();
  }
}

void Part::doPrint( KPrinter& printer )
{
  QPainter painter( &printer );
  QOutputDevKPrinter printdev( painter, printer );
  QValueList<int> pages = printer.pageList();
  for ( QValueList<int>::ConstIterator i = pages.begin(); i != pages.end();)
  {
    m_doc->displayPage( &printdev, *i, printer.resolution(), 0, true );
    if ( ++i != pages.end() )
      printer.newPage();
  }
}

// vim:ts=2:sw=2:tw=78:et
