#include <config.h>
#include <kaction.h>
#include <kaboutdata.h>
#include <kaboutdialog.h>
#include <kapplication.h>
#include <kbugreport.h>
#include <kconfigdialog.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstdaction.h>
#include <ktip.h>
#include <qtimer.h>

#include "../config.h"
#include "documentWidget.h"
#include "fontpool.h"
#include "kdvi_multipage.h"
#include "kviewpart.h"
#include "performanceMeasurement.h"
#include "prefs.h"
#include "zoomlimits.h"
#include "kprinterwrapper.h"

#include "optionDialogFontsWidget.h"
#include "optionDialogGUIWidget_base.h"
#include "optionDialogSpecialWidget.h"

#include <qlabel.h>

//#define KDVI_MULTIPAGE_DEBUG

#ifdef PERFORMANCE_MEASUREMENT
// These objects are explained in the file "performanceMeasurement.h"
QTime performanceTimer;
int  performanceFlag = 0;
#endif


extern "C"
{
  void *init_kdvipart()
  {
    KGlobal::locale()->insertCatalogue("kviewshell");
    return new KDVIMultiPageFactory;
  }
}


KInstance *KDVIMultiPageFactory::s_instance = 0L;


KDVIMultiPageFactory::KDVIMultiPageFactory()
{
}


KDVIMultiPageFactory::~KDVIMultiPageFactory()
{
    delete s_instance;

    s_instance = 0;
}


KParts::Part *KDVIMultiPageFactory::createPartObject( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *, const QStringList & )
{
  KParts::Part *obj = new KDVIMultiPage(parentWidget, widgetName, parent, name);
  return obj;
}


KInstance *KDVIMultiPageFactory::instance()
{
  if (!s_instance)
    s_instance = new KInstance("kdvi");
  return s_instance;
}


KDVIMultiPage::KDVIMultiPage(QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name)
  : KMultiPage(parentWidget, widgetName, parent, name), window(0)
{
#ifdef PERFORMANCE_MEASUREMENT
  performanceTimer.start();
#endif

  timer_id = -1;
  setInstance(KDVIMultiPageFactory::instance());

  printer = 0;

  findDialog = 0;
  findNextAction         = 0;
  findPrevAction         = 0;
  lastCurrentPage = 0;

  window = new dviWindow(scrollView());
  window->setName("DVI renderer");
  currentPage.setRenderer(window);

  widgetList.resize(0);
  connect(window, SIGNAL(prescanDone()), this, SLOT(generateDocumentWidgets()));
  connect(window, SIGNAL(setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
  connect(window, SIGNAL(documentSpecifiedPageSize(const pageSize&)), this, SIGNAL( documentSpecifiedPageSize(const pageSize&)) );
  connect(window, SIGNAL(needsRepainting()), this, SLOT(repaintAllVisibleWidgets()));
  docInfoAction    = new KAction(i18n("Document &Info"), 0, window, SLOT(showInfo()), actionCollection(), "info_dvi");


  QStringList viewModes;
  viewModes.append(i18n("Single Page"));
  viewModes.append(i18n("Continuous"));
  viewModes.append(i18n("Continuous - Facing"));
  viewModes.append(i18n("Overview"));
  viewModeAction = new KSelectAction (i18n("View Mode"), 0, 0, 0, actionCollection(), "viewmode");
  viewModeAction->setItems(viewModes);
  connect(viewModeAction, SIGNAL(activated (int)), this, SLOT(setViewMode(int)));

  backAction = new KAction(i18n("&Back"), "1leftarrow", 0, 
                   this, SLOT(doGoBack()), actionCollection(), "go_back");
  forwardAction = new KAction(i18n("&Forward"), "1rightarrow", 0, 
                      this, SLOT(doGoForward()), actionCollection(), "go_forward");

  connect(&document_history, SIGNAL(backItem(bool)), backAction, SLOT(setEnabled(bool)));
  connect(&document_history, SIGNAL(forwardItem(bool)), forwardAction, SLOT(setEnabled(bool)));
  document_history.clear();

  embedPSAction      = new KAction(i18n("Embed External PostScript Files..."), 0, this, SLOT(slotEmbedPostScript()), actionCollection(), "embed_postscript");
  connect(window, SIGNAL(prescanDone()), this, SLOT(setEmbedPostScriptAction()));

  if (window->supportsTextSearch()) {
    findTextAction = KStdAction::find(this, SLOT(showFindTextDialog()), actionCollection(), "find");
    findNextAction = KStdAction::findNext(this, SLOT(findNextText()), actionCollection(), "findnext");
    findNextAction->setEnabled(false);
    findPrevAction = KStdAction::findPrev(this, SLOT(findPrevText()), actionCollection(), "findprev");
    findPrevAction->setEnabled(false);
  }

  copyTextAction     = KStdAction::copy(&userSelection, SLOT(copyText()), actionCollection(), "copy_text");
  userSelection.setAction(copyTextAction);
  selectAllAction    = KStdAction::selectAll(this, SLOT(doSelectAll()), actionCollection(), "edit_select_all");
  new KAction(i18n("Enable All Warnings && Messages"), 0, this, SLOT(doEnableWarnings()), actionCollection(), "enable_msgs");
  exportPSAction     = new KAction(i18n("PostScript..."), 0, this, SLOT(doExportPS()), actionCollection(), "export_postscript");
  exportPDFAction    = new KAction(i18n("PDF..."), 0, this, SLOT(doExportPDF()), actionCollection(), "export_pdf");
  exportTextAction   = new KAction(i18n("Text..."), 0, this, SLOT(doExportText()), actionCollection(), "export_text");

  new KAction(i18n("&DVI Options..."), 0, this, SLOT(doSettings()), actionCollection(), "settings_dvi");
  KStdAction::tipOfDay(this, SLOT(showTip()), actionCollection(), "help_tipofday");
  new KAction(i18n("About KDVI"), 0, this, SLOT(about()), actionCollection(), "about_kdvi");
  new KAction(i18n("KDVI Handbook"), 0, this, SLOT(helpme()), actionCollection(), "help_dvi");
  new KAction(i18n("Report Bug in KDVI..."), 0, this, SLOT(bugform()), actionCollection(), "bug_dvi");

  setXMLFile("kdvi_part.rc");

  connect(window, SIGNAL(request_goto_page(int, int)), this, SLOT(goto_page(int, int) ) );

  connect(scrollView(), SIGNAL(contentsMoving(int, int)), this, SLOT(contentsMovingInScrollView(int, int)) );

  readSettings();
  preferencesChanged();

  enableActions(false);
  // Show tip of the day, when the first main window is shown.
  QTimer::singleShot(0,this,SLOT(showTipOnStart()));
}


void KDVIMultiPage::repaintAllVisibleWidgets(void)
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::repaintAllVisibleWidgets(void) called" << endl;
#endif

  // Clear the page cache
  currentPage.clear();

  // Go through the list of widgets and resize them, if necessary
  bool everResized = false;
  for(Q_UINT16 i=0; i<widgetList.size(); i++) {
    documentWidget *dviWidget = (documentWidget *)(widgetList[i]);
    if (dviWidget == 0)
      continue;
   
    // Resize, if necessary
    QSize sop = window->sizeOfPage(i+1);
    if (sop != dviWidget->size()) {
      dviWidget->resize( window->sizeOfPage(i+1) );
      everResized = true;
    }
  }

  // If at least one widget was resized, all widgets should be
  // re-aligned. This will automatically update all necessary
  // widget. If no widgets were resized, go through the list of
  // widgets again, and update those that are visible
  if (everResized == true)
    scrollView()->centerContents();
  else {
    QRect visiblRect(scrollView()->contentsX(), scrollView()->contentsY(), scrollView()->visibleWidth(), scrollView()->visibleHeight());
    for(Q_UINT16 i=0; i<widgetList.size(); i++) {
      documentWidget *dviWidget = (documentWidget *)(widgetList[i]);
      if (dviWidget == 0)
    continue;
    
      // Check visibility, and update
      QRect widgetRect(scrollView()->childX(dviWidget), scrollView()->childY(dviWidget), dviWidget->width(), dviWidget->height() );
      if (widgetRect.intersects(visiblRect))
    dviWidget->update();
    }
  }
}


void KDVIMultiPage::generateDocumentWidgets(int startPage)
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::generateDocumentWidgets(void) called" << endl;
#endif
  
  widgetList.setAutoDelete(true);
  switch (viewModeAction->currentItem())
  {
    case KVS_SinglePage:
    widgetList.resize(1);
      break;
    case KVS_Overview:
    {
      int visiblePages = Prefs::overviewModeColumns() * Prefs::overviewModeRows();
      // In two column mode the first row contains only one page.
      if (Prefs::overviewModeColumns() == 2)
        visiblePages--;
      // We cannot have more widgets then pages in the document
      visiblePages = QMIN(visiblePages, window->totalPages() - startPage + 1);
      widgetList.resize(visiblePages);
      break;
    }
    default:
    widgetList.resize(window->totalPages());
  }
  widgetList.setAutoDelete(false);

  documentWidget *dviWidget;
  for(Q_UINT16 i=0; i<widgetList.size(); i++) {
    dviWidget = (documentWidget *)(widgetList[i]);
    if (dviWidget == 0) {
      dviWidget = new documentWidget(scrollView()->viewport(), scrollView(), window->sizeOfPage(i+1), &currentPage, &userSelection, "singlePageWidget" );
      widgetList.insert(i, dviWidget);
      dviWidget->setPageNumber(i+startPage);
      dviWidget->show();
      
      connect(dviWidget, SIGNAL(localLink(const QString &)), window, SLOT(handleLocalLink(const QString &)));
      connect(dviWidget, SIGNAL(SRCLink(const QString&,QMouseEvent *, documentWidget *)), window,
          SLOT(handleSRCLink(const QString &,QMouseEvent *, documentWidget *)));
      connect(dviWidget, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
    } else
      dviWidget->setPageNumber(i+startPage);
  }

  scrollView()->addChild(&widgetList);
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::generateDocumentWidgets(void) ended" << endl;
#endif
}


void KDVIMultiPage::setViewMode(int mode)
{
  // Save viewMode for future uses of KDVI

  switch (mode) 
  {
    case KVS_SinglePage:
      Prefs::setViewMode(KVS_SinglePage);
      scrollView()->setNrColumns(1);
      scrollView()->setNrRows(1);
      scrollView()->setContinuousViewMode(false);
      break;
    case KVS_ContinuousFacing:
      Prefs::setViewMode(KVS_ContinuousFacing);
    scrollView()->setNrColumns(2);
      scrollView()->setNrRows(1);
      scrollView()->setContinuousViewMode(true);
      break;
    case KVS_Overview:
      Prefs::setViewMode(KVS_Overview);
      scrollView()->setNrColumns(Prefs::overviewModeColumns());
      scrollView()->setNrRows(Prefs::overviewModeRows());
      scrollView()->setContinuousViewMode(false);
      break;
    default:  //KVS_Continuous
      Prefs::setViewMode(KVS_Continuous);
    scrollView()->setNrColumns(1);
      scrollView()->setNrRows(1);
      scrollView()->setContinuousViewMode(true);
  }
  
  generateDocumentWidgets();
  emit viewModeChanged();
}

void KDVIMultiPage::slotEmbedPostScript(void)
{
  if (window) {
    window->embedPostScript();
    emit askingToCheckActions();
  }
}


void KDVIMultiPage::setEmbedPostScriptAction(void)
{
  if ((window == 0) || (window->dviFile == 0) || (window->dviFile->numberOfExternalPSFiles == 0))
    embedPSAction->setEnabled(false);
  else
    embedPSAction->setEnabled(true);
}


void KDVIMultiPage::slotSave()
{
  // Try to guess the proper ending...
  QString formats;
  QString ending;
  int rindex = m_file.findRev(".");
  if (rindex == -1) {
    ending = QString::null;
    formats = QString::null;
  } else {
    ending = m_file.mid(rindex); // e.g. ".dvi"
    formats = fileFormats().grep(ending).join("\n");
  }

  QString fileName = KFileDialog::getSaveFileName(QString::null, formats, 0, i18n("Save File As"));

  if (fileName.isEmpty())
    return;

  // Add the ending to the filename. I hope the user likes it that
  // way.
  if (!ending.isEmpty() && fileName.find(ending) == -1)
    fileName = fileName+ending;

  if (QFile(fileName).exists()) {
    int r = KMessageBox::warningYesNo (0, i18n("The file %1\nexists. Do you want to overwrite that file?").arg(fileName),
                       i18n("Overwrite File"));
    if (r == KMessageBox::No)
      return;
  }

  // TODO: error handling...
  if ((window != 0) && (window->dviFile != 0) && (window->dviFile->dvi_Data() != 0))
    window->dviFile->saveAs(fileName);
  
  return;
}


void KDVIMultiPage::slotSave_defaultFilename()
{
  // TODO: error handling...
  if ((window != 0) && (window->dviFile != 0))
    window->dviFile->saveAs(m_file);
  return;
}


bool KDVIMultiPage::isModified()
{
  if ((window == 0) || (window->dviFile == 0) || (window->dviFile->dvi_Data() == 0))
    return false;
  else
    return window->dviFile->isModified;
}


KDVIMultiPage::~KDVIMultiPage()
{
  if (timer_id != -1)
    killTimer(timer_id);
  timer_id = -1;
  writeSettings();
  Prefs::writeConfig();
  delete printer;
}


Q_UINT16 KDVIMultiPage::getCurrentPageNumber()
{
  documentWidget *dviWidget;
  switch(widgetList.size()) {
  case 0:
    lastCurrentPage = 0;
    return 0;
  case 1:
    // If there is only one widget, the matter is easy: just return
    // the pageNumber of that widget (or 0 if the ptr to the widget is
    // 0)
    dviWidget= (documentWidget *)(widgetList[0]);
    if (dviWidget == 0) {
      lastCurrentPage = 0;
      return 0;
    }
    lastCurrentPage = dviWidget->getPageNumber();
    return lastCurrentPage;
  default:
    // Check if the page 'lastCurrentPage' is still visible. If yes,
    // return that. We call that 'lazy page number display': the
    // displayed page number remains unchanged as long as the
    // appropriate widget is still visible.
    if ((widgetList.size() > lastCurrentPage)&&(lastCurrentPage != 0)) {
      dviWidget = (documentWidget *)(widgetList[lastCurrentPage-1]);
      if (dviWidget != 0) {
    if (dviWidget->getPageNumber() == lastCurrentPage) {
      // Found the widget. Now check if it is visible
      if ((scrollView()->childY(dviWidget) < (scrollView()->contentsY() + scrollView()->visibleHeight())) &&
          ((scrollView()->childY(dviWidget)+dviWidget->height()) > scrollView()->contentsY())) {
        return lastCurrentPage;
      }
    }
      }
    }

    // Otherwise, find the first widget that is visible, and return
    // the page number of that.
    for(Q_UINT16 i=0; i<widgetList.size(); i++) {
      dviWidget = (documentWidget *)(widgetList[i]);
      if (dviWidget == 0)
    continue;

      int Y = scrollView()->childY(dviWidget) + dviWidget->height();
      if (Y > scrollView()->contentsY()) {
    lastCurrentPage = dviWidget->getPageNumber();
    return lastCurrentPage;
      }
    }
  }
  lastCurrentPage = 0;
  return lastCurrentPage;
}


void KDVIMultiPage::contentsMovingInScrollView(int x, int y)
{
  Q_UINT16 cpg = getCurrentPageNumber();
  if ((cpg != 0) && (window != 0) && (window->dviFile != 0))
    emit(pageInfo(window->dviFile->total_pages, cpg-1));
}


bool KDVIMultiPage::openFile()
{
  document_history.clear();
  emit setStatusBarText(i18n("Loading file %1").arg(m_file));

  bool r = window->setFile(m_file,url().ref());
  if (!r)
    emit setStatusBarText(QString::null);

  window->changePageSize();
  emit numberOfPages(window->totalPages());
  enableActions(r);
  return r;
}


void KDVIMultiPage::jumpToReference(QString reference)
{
  if (window) {
    window->reference = reference;
    window->all_fonts_loaded(0); // In spite of its name, this method tries to parse the reference.
  }
}


bool KDVIMultiPage::closeURL()
{
  document_history.clear();
  window->setFile(""); // That means: close the file. Resize the widget to 0x0.
  enableActions(false);
  return true;
}


QStringList KDVIMultiPage::fileFormats()
{
  QStringList r;
  r << i18n("*.dvi *.DVI|TeX Device Independent Files (*.dvi)");
  return r;
}


bool KDVIMultiPage::gotoPage(int page)
{
  if (widgetList.size() == 0) {
    kdError(4300) << "KDVIMultiPage::gotoPage(" << page << ") called, but widgetList is empty" << endl;
    return false;
  }

  document_history.add(page,0);

  // If we are in overview viewmode
  if (Prefs::viewMode() == KVS_Overview)
  {
    int visiblePages = Prefs::overviewModeRows() * Prefs::overviewModeColumns();
    // Pagenumber of the first visibile Page
    int firstPage = ((documentWidget*)widgetList[0])->getPageNumber();
    int newFirstPage = page + 1 - (page % visiblePages);
    if (firstPage != newFirstPage) // widgets need to be updated
    {
      if ( (window->totalPages() - newFirstPage + 1 < visiblePages) ||
           (widgetList.size() < visiblePages) ) 
      {
        // resize widgetList
        // the pages are also set correctly be "generateDocumentWidgets"
        generateDocumentWidgets(newFirstPage);
      }
      else
      {
        // "page" is not shown in the scrollview, so we have to switch widgets.
        // Here we don't need to resize the widgetList.
        for (int i = 0; i < widgetList.size(); i++)
        {
          documentWidget* ptr = (documentWidget*)(widgetList[i]);
          if (ptr != 0)
          {
            ptr->setPageNumber(newFirstPage + i);
          }
        }
      }
    }
    // move scrollview to "page".
    
    // Make the widget &ptr visible in the scrollview. Somehow this
    // doesn't seem to trigger the signal contentsMoved in the
    // QScrollview, so that we better set lastCurrentPage ourselves.
    documentWidget* ptr = (documentWidget*)(widgetList[page % visiblePages]);
    scrollView()->moveViewportToWidget(ptr);
    lastCurrentPage = page+1;
    return true;
  }
  
  if (widgetList.size() == 1) {
    // If the widget list contains only a single element, then either
    // the document contains only one page, or we are in "single page"
    // view mode. In either case, we set the page number of the single
    // widget to 'page'
    documentWidget *ptr = (documentWidget *)(widgetList[0]);

    // Paranoia security check
    if (ptr == 0) {
      kdError(4300) << "KDVIMultiPage::gotoPage() called with widgetList.size() == 1, but widgetList[0] == 0" << endl;
      return false;
    }
    ptr->setPageNumber(page+1);
  } else {
    // There are multiple widgets, then we are either in the
    // "Continuous" or in the "Continouous-Facing" view mode. In that
    // case, we find the widget which is supposed to display page
    // 'page' and move the scrollview to make it visible

    // Paranoia security checks
    if (widgetList.size() < page) {
      kdError(4300) << "KDVIMultiPage::gotoPage(page) called with widgetList.size()=" << widgetList.size() << ", and page=" << page << endl;
      return false;
    }
    documentWidget *ptr = (documentWidget *)(widgetList[page]);
    if (ptr == 0) {
      kdError(4300) << "KDVIMultiPage::gotoPage() called with widgetList.size() > 1, but widgetList[page] == 0" << endl;
      return false;
    }

    // Make the widget &ptr visible in the scrollview. Somehow this
    // doesn't seem to trigger the signal contentsMoved in the
    // QScrollview, so that we better set lastCurrentPage ourselves.
    scrollView()->moveViewportToWidget(ptr);
    lastCurrentPage = page+1;
  }
  return true;
}


void KDVIMultiPage::goto_page(int page, int y, bool isLink)
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::gotoPage( pageNr=" << page << ", y=" << y <<" )" << endl;
#endif

  if (widgetList.size() == 0) {
    kdError(4300) << "KDVIMultiPage::goto_Page(" << page << ", y) called, but widgetList is empty" << endl;
    return;
  }

  if (isLink)
    document_history.add(page, y);

  documentWidget *ptr;
  
  // If we are in overview viewmode
  if (Prefs::viewMode() == KVS_Overview)
  {
    int visiblePages = Prefs::overviewModeRows() * Prefs::overviewModeColumns();
    // Pagenumber of the first visibile Page
    int firstPage = ((documentWidget*)widgetList[0])->getPageNumber();
    int newFirstPage = page + 1 - (page % visiblePages);
    if (firstPage != newFirstPage) // widgets need to be updated
    {
        if ( (window->totalPages() - newFirstPage + 1 < visiblePages) ||
            (widgetList.size() < visiblePages) ) 
        {
            // resize widgetList
            // the pages are also set correctly be "generateDocumentWidgets"
            generateDocumentWidgets(newFirstPage);
        }
        else
        {
            // "page" is not shown in the scrollview, so we have to switch widgets.
            // Here we don't need to resize the widgetList.
            for (int i = 0; i < widgetList.size(); i++)
            {
                ptr = (documentWidget*)(widgetList[i]);
                if (ptr != 0)
                {
                    ptr->setPageNumber(newFirstPage + i);
                }
            }
        }
    }
    // move scrollview to "page".
    
    // Make the widget &ptr visible in the scrollview. Somehow this
    // doesn't seem to trigger the signal contentsMoved in the
    // QScrollview, so that we better set lastCurrentPage ourselves.
    ptr = (documentWidget*)(widgetList[page % visiblePages]);
    // FIXME: This doesn't calculate the right position in all cases.
    scrollView()->setContentsPos(scrollView()->childX(ptr) - scrollView()->distanceBetweenPages(), 
                                 scrollView()->childY(ptr) + y);
    lastCurrentPage = page+1;
  } 
  else if (widgetList.size() == 1) 
  {
    // If the widget list contains only a single element, then either
    // the document contains only one page, or we are in "single page"
    // view mode. In either case, we set the page number of the single
    // widget to 'page'
    ptr = (documentWidget *)(widgetList[0]);

    // Paranoia security check
    if (ptr == 0) {
      kdError(4300) << "KDVIMultiPage::goto_Page() called with widgetList.size() == 1, but widgetList[0] == 0" << endl;
      return;
    }
    ptr->setPageNumber(page+1);

    scrollView()->ensureVisible(0, scrollView()->childY(ptr)+y);
  } else {
    // There are multiple widgets, then we are either in the
    // "Continuous" or in the "Continouous-Facing" view mode. In that
    // case, we find the widget which is supposed to display page
    // 'page' and move the scrollview to make it visible

    // Paranoia security checks
    if (widgetList.size() < page) {
      kdError(4300) << "KDVIMultiPage::goto_Page(page,y ) called with widgetList.size()=" << widgetList.size() << ", and page=" << page << endl;
      return;
    }
    ptr = (documentWidget *)(widgetList[page]);
    if (ptr == 0) {
      kdError(4300) << "KDVIMultiPage::goto_Page() called with widgetList.size() > 1, but widgetList[page] == 0" << endl;
      return;
    }

    // Make the widget &ptr visible in the scrollview. We try to do
    // that intelligently, so that the user gets to see most of the
    // widget
    if (ptr->height() < scrollView()->visibleHeight()) {
      // If the widget is less tall then the visible portion of the
      // viewPort, try to center the widget in the viewport
      scrollView()->setContentsPos(0, scrollView()->childY(ptr) - (scrollView()->visibleHeight()-ptr->height())/2);
    } else
      scrollView()->ensureVisible(0, scrollView()->childY(ptr)+y);
    lastCurrentPage = page+1;
    ptr->update();
  }
  if (isLink)
    ptr->flash(y);
  emit(pageInfo(window->dviFile->total_pages, page));
}


void KDVIMultiPage::gotoPage(int pageNr, int beginSelection, int endSelection )
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "KDVIMultiPage::gotoPage( pageNr=" << pageNr << ", beginSelection=" << beginSelection <<", endSelection=" << endSelection <<" )" << endl;
#endif

  if (pageNr == 0) {
    kdError(4300) << "KDVIMultiPage::gotoPage(...) called with pageNr=0" << endl;
    return;
  }

  documentPage *pageData = currentPage.getPage(pageNr);
  if (pageData == 0) {
#ifdef DEBUG_DOCUMENTWIDGET
    kdDebug(4300) << "documentWidget::paintEvent: no documentPage generated" << endl;
#endif
    return;
  }

  QString selectedText("");
  for(unsigned int i = beginSelection; i < endSelection; i++) {
    selectedText += pageData->textLinkList[i].linkText;
    selectedText += "\n";
  }
  userSelection.set(pageNr, beginSelection, endSelection, selectedText);


  Q_UINT16 y = pageData->textLinkList[beginSelection].box.bottom();
  goto_page(pageNr-1, y);
  /*
    document_history.add(pageNr,y);

    scrollView()->ensureVisible(scrollView()->width()/2, y );
    emit pageInfo(window->totalPages(), pageNr );
  */
}


double KDVIMultiPage::setZoom(double zoom)
{
  if (zoom < ZoomLimits::MinZoom/1000.0)
    zoom = ZoomLimits::MinZoom/1000.0;
  if (zoom > ZoomLimits::MaxZoom/1000.0)
    zoom = ZoomLimits::MaxZoom/1000.0;

  double z = window->setZoom(zoom);
  return z;
}


double KDVIMultiPage::zoomForHeight(int height)
{
  return (double)(height)/(window->xres*(window->paper_height_in_cm/2.54));
}


double KDVIMultiPage::zoomForWidth(int width)
{
  return (double)(width)/(window->xres*(window->paper_width_in_cm/2.54));
}


void KDVIMultiPage::setPaperSize(double w, double h)
{
  window->setPaper(w, h);
}


void KDVIMultiPage::doSelectAll(void)
{
  switch( widgetList.size() ) {
  case 0:
    kdError(4300) << "KDVIMultiPage::doSelectAll(void) while widgetList is empty" << endl;
    break;
  case 1:
    ((documentWidget *)widgetList[0])->selectAll();
    break;
  default:
    if (widgetList.size() < getCurrentPageNumber())
      kdError(4300) << "KDVIMultiPage::doSelectAll(void) while widgetList.size()=" << widgetList.size() << "and getCurrentPageNumber()=" << getCurrentPageNumber() << endl;
    else
      ((documentWidget *)widgetList[getCurrentPageNumber()-1])->selectAll();
  }
}


void KDVIMultiPage::doExportPS(void)
{
  window->exportPS();
}


void KDVIMultiPage::doExportPDF(void)
{
  window->exportPDF();
}


void KDVIMultiPage::doSettings()
{
  static optionDialogFontsWidget* fontConfigWidget = 0;
  if (KConfigDialog::showDialog("kdvi_config"))
    return;
  
  KConfigDialog* configDialog = new KConfigDialog(scrollView(), "kdvi_config", Prefs::self());
  
  fontConfigWidget = new optionDialogFontsWidget(scrollView());
  optionDialogSpecialWidget* specialConfigWidget = new optionDialogSpecialWidget(scrollView());
  optionDialogGUIWidget_base* guiWidget = new optionDialogGUIWidget_base(scrollView());
  
  configDialog->addPage(fontConfigWidget, i18n("Tex Fonts"), "fonts");
  configDialog->addPage(specialConfigWidget, i18n("DVI Specials"), "dvi");
  configDialog->addPage(guiWidget, i18n("Interface"), "view_icon");
  configDialog->setHelp("preferences", "kdvi");
  
  connect(configDialog, SIGNAL(settingsChanged()), this, SLOT(preferencesChanged()));
  
  configDialog->show();
}


void KDVIMultiPage::about()
{
  KAboutDialog *ab = new KAboutDialog(KAboutDialog::AbtAppStandard,
                      i18n("the KDVI plugin"),
                      KAboutDialog::Close, KAboutDialog::Close);

  ab->setProduct("kdvi", "1.2", QString::null, QString::null);
  ab->addTextPage (i18n("About"),
           i18n("A previewer for Device Independent files (DVI files) produced "
            "by the TeX typesetting system.<br>"
            "Based on kdvi 0.4.3 and on xdvik, version 18f.<br><hr>"
            "For latest information, visit "
            "<a href=\"http://devel-home.kde.org/~kdvi\">KDVI's Homepage</a>."),
           true);
  ab->addTextPage (i18n("Authors"),
           i18n("Stefan Kebekus<br>"
            "<a href=\"http://www.mi.uni-koeln.de/~kebekus\">"
            "http://www.mi.uni-koeln.de/~kebekus</a><br>"
            "<a href=\"mailto:kebekus@kde.org\">kebekus@kde.org</a><br>"
            "Current maintainer of kdvi. Major rewrite of version 0.4.3."
            "Implementation of hyperlinks.<br>"
            "<hr>"
            "Philipp Lehmann<br>"
            "testing and bug reporting"
            "<hr>"
            "Markku Hinhala<br>"
            "Author of kdvi 0.4.3"
            "<hr>"
            "Nicolai Langfeldt<br>"
            "Maintainer of xdvik"
            "<hr>"
            "Paul Vojta<br>"
            " Author of xdvi<br>"
            "<hr>"
            "Many others. Really, lots of people who were involved in kdvi, xdvik and "
            "xdvi. I apologize to those who I did not mention here. Please send me an "
            "email if you think your name belongs here."),
           true);

  ab->setMinimumWidth(500);
  ab->show();
}


void KDVIMultiPage::bugform()
{
  KAboutData *kab = new KAboutData("kdvi", I18N_NOOP("KDVI"), "1.1", 0, 0, 0, 0, 0);
  KBugReport *kbr = new KBugReport(0, true, kab );
  kbr->show();
}


void KDVIMultiPage::helpme()
{
  kapp->invokeHelp( "", "kdvi" );
}


void KDVIMultiPage::preferencesChanged()
{
#ifdef  KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "preferencesChanged" << endl;
#endif

  int mfmode = Prefs::metafontMode();

  bool makepk = Prefs::makePK();
  bool showPS = Prefs::showPS();
  bool showHyperLinks = Prefs::showHyperLinks();
  bool useType1Fonts = Prefs::useType1Fonts();
  bool useFontHints = Prefs::useFontHints();

  viewModeAction->setCurrentItem(Prefs::viewMode());
  setViewMode(Prefs::viewMode());
  window->setPrefs( showPS, showHyperLinks, Prefs::editorCommand(), mfmode, makepk, useType1Fonts, useFontHints);
}


bool KDVIMultiPage::print(const QStringList &pages, int current)
{
  // Make sure the KPrinter is available
  if (printer == 0) {
    printer = new KPrinter();
    if (printer == 0) {
      kdError(4300) << "Could not allocate printer structure" << endl;
      return false;
    }
  }
  
  // Feed the printer with useful defaults and information.
  printer->setPageSelection( KPrinter::ApplicationSide );
  printer->setCurrentPage( current+1 );
  printer->setMinMax( 1, window->totalPages() );
  printer->setFullPage( true );

  // Give a suggestion for the paper orientation. Unfortunately, as of
  // now, KPrinter then automatically disables the orientation widget,
  // so that the user cannot change our suggestion here. Thus, we
  // comment this out for now.
  /*
  if (window != 0)
    if (window->paper_height_in_cm >= window->paper_width_in_cm)
      printer->setOrientation( KPrinter::Portrait );
    else
      printer->setOrientation( KPrinter::Landscape );
  */

  // If pages are marked, give a list of marked pages to the
  // printer. We try to be smart and optimize the list by using ranges
  // ("5-11") wherever possible. The user will be tankful for
  // that. Complicated? Yeah, but that's life.
  if (pages.isEmpty() == true)
    printer->setOption( "kde-range", "" );
  else {
    int commaflag = 0;
    QString range;
    QStringList::ConstIterator it = pages.begin();
    do{
      int val = (*it).toUInt()+1;
      if (commaflag == 1)
    range +=  QString(", ");
      else
    commaflag = 1;
      int endval = val;
      if (it != pages.end()) {
    QStringList::ConstIterator jt = it;
    jt++;
    do{
      int val2 = (*jt).toUInt()+1;
      if (val2 == endval+1)
        endval++;
      else
        break;
      jt++;
    } while( jt != pages.end() );
    it = jt;
      } else
    it++;
      if (endval == val)
    range +=  QString("%1").arg(val);
      else
    range +=  QString("%1-%2").arg(val).arg(endval);
    } while (it != pages.end() );
    printer->setOption( "kde-range", range );
  }

  // Show the printer options requestor
  if (!printer->setup(scrollView(), i18n("Print %1").arg(m_file.section('/', -1))))
    return false;
  // This funny method call is necessary for the KPrinter to return
  // proper results in printer->orientation() below. It seems that
  // KPrinter does some options parsing in that method.
  ((KPrinterWrapper *)printer)->doPreparePrinting();
  if (printer->pageList().isEmpty()) {
    KMessageBox::error( scrollView(),
            i18n("The list of pages you selected was empty.\n"
                 "Maybe you made an error in selecting the pages, "
                 "e.g. by giving an invalid range like '7-2'.") );
    return false;
  }

  // Turn the results of the options requestor into a list arguments
  // which are used by dvips.
  QString dvips_options = QString::null;
  // Print in reverse order.
  if ( printer->pageOrder() == KPrinter::LastPageFirst )
    dvips_options += "-r ";
  // Print only odd pages.
  if ( printer->pageSet() == KPrinter::OddPages )
    dvips_options += "-A ";
  // Print only even pages.
  if ( printer->pageSet() == KPrinter::EvenPages )
    dvips_options += "-B ";
  // We use the printer->pageSize() method to find the printer page
  // size, and pass that information on to dvips. Unfortunately, dvips
  // does not understand all of these; what exactly dvips understands,
  // depends on its configuration files. Consequence: expect problems
  // with unusual paper sizes.
  switch( printer->pageSize() ) {
  case KPrinter::A4:
    dvips_options += "-t a4 ";
    break;
  case KPrinter::B5:
    dvips_options += "-t b5 ";
    break;
    case KPrinter::Letter:
      dvips_options += "-t letter ";
      break;
  case KPrinter::Legal:
    dvips_options += "-t legal ";
    break;
    case KPrinter::Executive:
      dvips_options += "-t executive ";
      break;
  case KPrinter::A0:
    dvips_options += "-t a0 ";
    break;
  case KPrinter::A1:
    dvips_options += "-t a1 ";
    break;
  case KPrinter::A2:
    dvips_options += "-t a2 ";
    break;
  case KPrinter::A3:
      dvips_options += "-t a3 ";
      break;
  case KPrinter::A5:
    dvips_options += "-t a5 ";
    break;
  case KPrinter::A6:
    dvips_options += "-t a6 ";
    break;
  case KPrinter::A7:
    dvips_options += "-t a7 ";
    break;
  case KPrinter::A8:
    dvips_options += "-t a8 ";
      break;
  case KPrinter::A9:
    dvips_options += "-t a9 ";
    break;
  case KPrinter::B0:
    dvips_options += "-t b0 ";
    break;
  case KPrinter::B1:
    dvips_options += "-t b1 ";
    break;
  case KPrinter::B10:
    dvips_options += "-t b10 ";
    break;
  case KPrinter::B2:
    dvips_options += "-t b2 ";
    break;
  case KPrinter::B3:
    dvips_options += "-t b3 ";
    break;
  case KPrinter::B4:
    dvips_options += "-t b4 ";
    break;
  case KPrinter::B6:
    dvips_options += "-t b6 ";
    break;
  case KPrinter::B7:
    dvips_options += "-t b7 ";
    break;
  case KPrinter::B8:
    dvips_options += "-t b8 ";
    break;
  case KPrinter::B9:
    dvips_options += "-t b9 ";
    break;
  case KPrinter::C5E:
    dvips_options += "-t c5e ";
    break;
  case KPrinter::Comm10E:
    dvips_options += "-t comm10e ";
    break;
  case KPrinter::DLE:
    dvips_options += "-t dle ";
    break;
  case KPrinter::Folio:
    dvips_options += "-t folio ";
    break;
  case KPrinter::Ledger:
    dvips_options += "-t ledger ";
    break;
  case KPrinter::Tabloid:
    dvips_options += "-t tabloid ";
    break;
  }
  // Orientation
  if ( printer->orientation() == KPrinter::Landscape )
    dvips_options += "-t landscape ";


  // List of pages to print.
  QValueList<int> pageList = printer->pageList();
  dvips_options += "-pp ";
  int commaflag = 0;
  for( QValueList<int>::ConstIterator it = pageList.begin(); it != pageList.end(); ++it ) {
    if (commaflag == 1)
      dvips_options +=  QString(",");
    else
      commaflag = 1;
    dvips_options += QString("%1").arg(*it);
  }

  // Now print. For that, export the DVI-File to PostScript. Note that
  // dvips will run concurrently to keep the GUI responsive, keep log
  // of dvips and allow abort. Giving a non-zero printer argument
  // means that the dvi-widget will print the file when dvips
  // terminates, and then delete the output file.
  KTempFile tf;
  window->exportPS(tf.name(), dvips_options, printer);

  // "True" may be a bit euphemistic. However, since dvips runs
  // concurrently, there is no way of telling the result of the
  // printing command at this stage.
  return true;
}


// Explanation of the timerEvent.
//
// This is a dreadful hack. The problem we adress with this timer
// event is the following: the kviewshell has a KDirWatch object which
// looks at the DVI file and calls reload() when the object has
// changed. That works very nicely in principle, but in practise, when
// TeX runs for several seconds over a complicated file, this does not
// work at all. First, most of the time, while TeX is still writing,
// the file is invalid. Thus, reload() is very often called when the
// DVI file is bad. We solve this problem by checking the file
// first. If the file is bad, we do not reload. Second, when the file
// finally becomes good, it very often happens that KDirWatch does not
// notify us anymore. Whether this is a bug or a side effect of a
// feature of KDirWatch, I dare not say. We remedy that problem by
// using a timer: when reload() was called on a bad file, we
// automatically come back (via the timerEvent() function) every
// second and check if the file became good. If so, we stop the
// timer. It may well happen that KDirWatch calls us several times
// while we are waiting for the file to become good, but that does not
// do any harm.
//
// -- Stefan Kebekus.

void KDVIMultiPage::timerEvent( QTimerEvent * )
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "Timer Event " << endl;
#endif
  reload();
}

void KDVIMultiPage::reload()
{
#ifdef KDVI_MULTIPAGE_DEBUG
  kdDebug(4300) << "Reload file " << m_file << endl;
#endif

  if (window->correctDVI(m_file)) {
    killTimer(timer_id);
    timer_id = -1;
    bool r = window->setFile(m_file, QString::null, false);
    enableActions(r);

    emit pageInfo(window->totalPages(), window->curr_page()-1 );
  } else {
    if (timer_id == -1)
      timer_id = startTimer(1000);
  }
}


void KDVIMultiPage::enableActions(bool b)
{
  docInfoAction->setEnabled(b);
  selectAllAction->setEnabled(b);
  findTextAction->setEnabled(b);
  exportPSAction->setEnabled(b);
  exportPDFAction->setEnabled(b);
  exportTextAction->setEnabled(b);
  setEmbedPostScriptAction();
}


void KDVIMultiPage::doGoBack(void)
{
  HistoryItem *it = document_history.back();
  if (it != 0)
    goto_page(it->page, it->ypos, false); // Do not add a history item.
  else
    kdDebug(4300) << "Faulty return -- bad history buffer" << endl;
  return;
}


void KDVIMultiPage::doGoForward(void)
{
  HistoryItem *it = document_history.forward();
  if (it != 0)
    goto_page(it->page, it->ypos, false); // Do not add a history item.
  else
    kdDebug(4300) << "Faulty return -- bad history buffer" << endl;
  return;
}


void KDVIMultiPage::doEnableWarnings(void)
{
  KMessageBox::information (scrollView(), i18n("All messages and warnings will now be shown."));
  KMessageBox::enableAllMessages();
  KTipDialog::setShowOnStart(true);
}


void KDVIMultiPage::showTip(void)
{
  KTipDialog::showTip(scrollView(), "kdvi/tips", true);
}


void KDVIMultiPage::showTipOnStart(void)
{
  KTipDialog::showTip(scrollView(), "kdvi/tips");
}


void KDVIMultiPage::guiActivateEvent( KParts::GUIActivateEvent * event )
{
  if (event->activated() && url().isEmpty())
    emit setWindowCaption( i18n("KDVI") );
}


#include "kdvi_multipage.moc"
