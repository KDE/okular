#include <config.h>
#include <kaction.h>
#include <kaboutdata.h>
#include <kaboutdialog.h>
#include <kapplication.h>
#include <kbugreport.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kstdaction.h>
#include <ktip.h>
#include <qtimer.h>

#include "../config.h"
#include "documentWidget.h"
#include "fontpool.h"
#include "infodialog.h"
#include "kdvi_multipage.h"
#include "kviewpart.h"
#include "optiondialog.h"
#include "performanceMeasurement.h"
#include "zoomlimits.h"

#ifdef PERFORMANCE_MEASUREMENT
// These objects are explained in the file "performanceMeasurement.h"
QTime performanceTimer;
int  performanceFlag = 0;
#endif


//#define KDVI_MULTIPAGE_DEBUG

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
  : KMultiPage(parentWidget, widgetName, parent, name), window(0), options(0)
{
#ifdef PERFORMANCE_MEASUREMENT
  performanceTimer.start();
#endif

  timer_id = -1;
  setInstance(KDVIMultiPageFactory::instance());

  printer = 0;
  document_history.clear();

  findDialog = 0;
  findNextAction         = 0;
  findPrevAction         = 0;
  
  // initialize the dvi machinery
  dviFile                = 0;

  font_pool              = new fontPool();
  if (font_pool == NULL) {
    kdError(4300) << "Could not allocate memory for the font pool." << endl;
    exit(-1);
  }
  connect(font_pool, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );


  info                   = new infoDialog(scrollView());
  if (info == 0) {
    // The info dialog is not vital. Therefore we don't abort if
    // something goes wrong here.
    kdError(4300) << "Could not allocate memory for the info dialog." << endl;
  } else {
    qApp->connect(font_pool, SIGNAL(MFOutput(QString)), info, SLOT(outputReceiver(QString)));
    qApp->connect(font_pool, SIGNAL(fonts_have_been_loaded(fontPool *)), info, SLOT(setFontInfo(fontPool *)));
    qApp->connect(font_pool, SIGNAL(new_kpsewhich_run(QString)), info, SLOT(clear(QString)));
  }


  dviWidget = new documentWidget(scrollView(), "singlePageWidget" );
  window = new dviWindow( 1.0, this);
  window->setName("DVI renderer");
  currentPage.setName("All purpose document page");
  dviWidget->setPage(&currentPage);

  connect(dviWidget, SIGNAL(localLink(const QString &)), window, SLOT(handleLocalLink(const QString &)));
  connect(dviWidget, SIGNAL(SRCLink(const QString&,QMouseEvent *)), window, SLOT(handleSRCLink(const QString &,QMouseEvent *)));
  connect(dviWidget, SIGNAL(needPixmap(documentPage *)), window, SLOT(drawPage(documentPage *)));
  connect(window, SIGNAL(flash(int)), dviWidget, SLOT(flash(int)));

  connect(window, SIGNAL(needsRepainting()), &currentPage, SLOT(clear()));
  connect(window, SIGNAL(needsRepainting()), dviWidget, SLOT(update()));


  connect(font_pool, SIGNAL(fonts_have_been_loaded(fontPool *)), window, SLOT(all_fonts_loaded(fontPool *)));
  preferencesChanged();

  connect( window, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
  connect( dviWidget, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
  connect( window, SIGNAL( documentSpecifiedPageSize(const pageSize&)), this, SIGNAL( documentSpecifiedPageSize(const pageSize&)) );
  docInfoAction    = new KAction(i18n("Document &Info"), 0, this, SLOT(doInfo()), actionCollection(), "info_dvi");

  backAction       = KStdAction::back(this, SLOT(doGoBack()), actionCollection(), "go_back");
  forwardAction    = KStdAction::forward(this, SLOT(doGoForward()), actionCollection(), "go_forward");
  document_history.setAction(backAction, forwardAction);
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

  copyTextAction     = KStdAction::copy(dviWidget, SLOT(copyText()), actionCollection(), "copy_text");
  dviWidget->DVIselection.setAction(copyTextAction);
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
  
  scrollView()->addChild(dviWidget);
  connect(window, SIGNAL(request_goto_page(int, int)), this, SLOT(goto_page(int, int) ) );
  connect(&currentPage, SIGNAL(pixmapChanged(void)), this, SLOT(contents_of_dviwin_changed(void)) );

  readSettings();
  enableActions(false);
  // Show tip of the day, when the first main window is shown.
  QTimer::singleShot(0,this,SLOT(showTipOnStart()));

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
  if ((window == 0) || (dviFile == 0) || (dviFile->numberOfExternalPSFiles == 0))
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
  if ((window != 0) && (dviFile != 0) && (dviFile->dvi_Data != 0)) {
    QFile out(fileName);
    out.open( IO_Raw|IO_WriteOnly );
    out.writeBlock ( (char *)(dviFile->dvi_Data), dviFile->size_of_file );
    out.close();
    dviFile->isModified = false;
  }

  return;
}


void KDVIMultiPage::slotSave_defaultFilename()
{
  // TODO: error handling...
  if ((window != 0) && (dviFile != 0) && (dviFile->dvi_Data != 0)) {
    QFile out(m_file);
    out.open( IO_Raw|IO_WriteOnly );
    out.writeBlock ( (char *)(dviFile->dvi_Data), dviFile->size_of_file );
    out.close();
    dviFile->isModified = false;
  }

  return;
}


bool KDVIMultiPage::isModified()
{
  if ((window == 0) || (dviFile == 0) || (dviFile->dvi_Data == 0)) 
    return false;
  else
    return dviFile->isModified;
}


KDVIMultiPage::~KDVIMultiPage()
{
  delete info;
  delete dviFile;
  delete font_pool;
  delete dviWidget;

  if (timer_id != -1)
    killTimer(timer_id);
  timer_id = -1;
  writeSettings();
  delete printer;
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

  currentPage.setPageNumber(1);
  dviWidget->update();
  
  return r;
}


void KDVIMultiPage::jumpToReference(QString reference)
{
  if (window) {
    window->reference = reference;
    window->all_fonts_loaded(0); // In spite of its name, this method tries to parse the reference.
  }
}


void KDVIMultiPage::contents_of_dviwin_changed(void)
{
  emit previewChanged(true);
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
  r << i18n("*.dvi *.DVI|TeX Device Independent files (*.dvi)");
  return r;
}


bool KDVIMultiPage::gotoPage(int page)
{
  document_history.add(page,0);
  currentPage.setPageNumber(page+1);
  return true;
}


void KDVIMultiPage::goto_page(int page, int y)
{
  document_history.add(page,y);
  if (y != 0) {
    currentPage.setPageNumber(page+1);
    dviWidget->flash(y);
  } else
    currentPage.setPageNumber(page+1);
  scrollView()->ensureVisible(scrollView()->width()/2, y );
  emit pageInfo(window->totalPages(), page );
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

  currentPage.setPageNumber(pageNr);
  window->drawPage(&currentPage);
  dviWidget->DVIselection.set(beginSelection, endSelection, currentPage.textLinkList[beginSelection].linkText); // @@@

  Q_UINT16 y = currentPage.textLinkList[beginSelection].box.bottom();
  document_history.add(pageNr,y);
  dviWidget->flash(y);
  
  scrollView()->ensureVisible(scrollView()->width()/2, y );
  emit pageInfo(window->totalPages(), pageNr );
}


double KDVIMultiPage::setZoom(double zoom)
{
  if (zoom < ZoomLimits::MinZoom/1000.0)
    zoom = ZoomLimits::MinZoom/1000.0;
  if (zoom > ZoomLimits::MaxZoom/1000.0)
    zoom = ZoomLimits::MaxZoom/1000.0;

  double z = window->setZoom(zoom);
  scrollView()->resizeContents(dviWidget->width(), dviWidget->height());

  return z;
}


double KDVIMultiPage::zoomForHeight(int height)
{
  return (double)(height-1)/(window->xres*(window->paper_height_in_cm/2.54));
}


double KDVIMultiPage::zoomForWidth(int width)
{
  return (double)(width-1)/(window->xres*(window->paper_width_in_cm/2.54));
}


void KDVIMultiPage::setPaperSize(double w, double h)
{
  window->setPaper(w, h);
}


bool KDVIMultiPage::preview(QPainter *p, int w, int h)
{
  QPixmap *map = window->pix();

  if ((map == 0) || (map->isNull()))
    return false;
  
  p->scale((double)w/(double)map->width(), (double)h/(double)map->height());
  p->drawPixmap(0, 0, *map);
  
  return true;
}


void KDVIMultiPage::doInfo(void)
{
  if (info == 0)
    return;
  
  info->setDVIData(dviFile);
  // Call check_if_fonts_filenames_are_looked_up() to make sure that
  // the fonts_info is emitted. That way, the infoDialog will know
  // about the fonts and their status.
  font_pool->check_if_fonts_filenames_are_looked_up();
  info->show();
}


void KDVIMultiPage::doSelectAll(void)
{
  dviWidget->selectAll();
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
  if (!options) {
    options = new OptionDialog(dviWidget);
    connect(options, SIGNAL(preferencesChanged()), this, SLOT(preferencesChanged()));
  }
  options->show();
}


void KDVIMultiPage::about()
{
  KAboutDialog *ab = new KAboutDialog(KAboutDialog::AbtAppStandard,
				      i18n("the KDVI plugin"),
				      KAboutDialog::Close, KAboutDialog::Close);

  ab->setProduct("kdvi", "1.1", QString::null, QString::null);
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

  KConfig *config = instance()->config();

  config->reparseConfiguration();
  config->setGroup( "kdvi" );

  int mfmode = config->readNumEntry( "MetafontMode", DefaultMFMode );
  if (( mfmode < 0 ) || (mfmode >= NumberOfMFModes))
    config->writeEntry( "MetafontMode", mfmode = DefaultMFMode );

  bool makepk = config->readBoolEntry( "MakePK", true );
  bool showPS = config->readBoolEntry( "ShowPS", true );
  if (showPS != window->showPS())
    window->setShowPS(showPS);

  bool showHyperLinks = config->readBoolEntry( "ShowHyperLinks", true );
  if (showHyperLinks != window->showHyperLinks())
    window->setShowHyperLinks(showHyperLinks);

  bool useType1Fonts = config->readBoolEntry( "UseType1Fonts", true );
  bool useFontHints = config->readBoolEntry( "UseFontHints", false );

  font_pool->setParameters(mfmode, makepk, useType1Fonts, useFontHints);

  window->setEditorCommand( config->readPathEntry( "EditorCommand" ));
}


bool KDVIMultiPage::print(const QStringList &pages, int current)
{
  // Make sure the KPrinter is available
  if (printer == 0) {
    printer = new KPrinter();
    if (printer == 0)
      return false;
  }

  // Feed the printer with useful defaults and information.
  printer->setPageSelection( KPrinter::ApplicationSide );
  printer->setCurrentPage( current+1 );
  printer->setMinMax( 1, window->totalPages() );

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
  if (!printer->setup(dviWidget, i18n("Print %1").arg(m_file.section('/', -1))))
    return false;
  if (printer->pageList().isEmpty()) {
    KMessageBox::error( dviWidget,
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
    int currsav = window->curr_page();

    bool r = window->setFile(m_file, QString::null, false);
    enableActions(r);

    // Go to the old page and tell kviewshell where we are.
    currentPage.setPageNumber(currsav);
    // We don't use "currsav" here, because that page may no longer
    // exist. In that case, gotoPage already selected another page.
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
  historyItem *it = document_history.back();
  if (it != 0)
    goto_page(it->page, it->ypos);
  else
    kdDebug(4300) << "Faulty return -- bad history buffer" << endl;
  return;
}


void KDVIMultiPage::doGoForward(void)
{
  historyItem *it = document_history.forward();
  if (it != 0)
    goto_page(it->page, it->ypos);
  else
    kdDebug(4300) << "Faulty return -- bad history buffer" << endl;
  return;
}


void KDVIMultiPage::doEnableWarnings(void)
{
  KMessageBox::information (dviWidget, i18n("All messages and warnings will now be shown."));
  KMessageBox::enableAllMessages();
  kapp->config()->reparseConfiguration();
  KTipDialog::setShowOnStart(true);
}


void KDVIMultiPage::showTip(void)
{
    KTipDialog::showTip(dviWidget, "kdvi/tips", true);
}


void KDVIMultiPage::showTipOnStart(void)
{
    KTipDialog::showTip(dviWidget, "kdvi/tips");
}


void KDVIMultiPage::guiActivateEvent( KParts::GUIActivateEvent * event )
{
  if (event->activated() && url().isEmpty())
    emit setWindowCaption( i18n("KDVI") );
}



#include "kdvi_multipage.moc"
