#include <config.h>
#include <kaction.h>
#include <kaboutdata.h>
#include <kaboutdialog.h>
#include <kapp.h>
#include <kbugreport.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kimageeffect.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <qobject.h>
#include <qlabel.h>
#include <qstring.h>
#include <qscrollview.h>
#include <qimage.h>
#include <qpixmap.h>

#include "fontpool.h"
#include "kdvi_multipage.moc"
#include "kviewpart.h"
#include "optiondialog.h"
#include "print.h"


extern "C"
{
  void *init_libkdvi()
  {
    return new KDVIMultiPageFactory;
  }
};


KInstance *KDVIMultiPageFactory::s_instance = 0L;


KDVIMultiPageFactory::KDVIMultiPageFactory()
{
}


KDVIMultiPageFactory::~KDVIMultiPageFactory()
{
  if (s_instance)
    delete s_instance;

  s_instance = 0;
}


KParts::Part *KDVIMultiPageFactory::createPart( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *, const QStringList & )
{
  KParts::Part *obj = new KDVIMultiPage(parentWidget, widgetName, parent, name);
  emit objectCreated(obj);
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
  timer_id = -1;
  setInstance(KDVIMultiPageFactory::instance()); 

  window = new dviWindow( 1.0, true, scrollView());
  preferencesChanged();

  docInfoAction   = new KAction(i18n("Document &Info"), 0, this, SLOT(doInfo()), actionCollection(), "info_dvi");
  exportPSAction  = new KAction(i18n("PostScript"), 0, this, SLOT(doExportPS()), actionCollection(), "export_postscript");
  exportPDFAction = new KAction(i18n("PDF"), 0, this, SLOT(doExportPDF()), actionCollection(), "export_pdf");

  new KAction(i18n("&DVI Options"), 0, this, SLOT(doSettings()), actionCollection(), "settings_dvi");
  new KAction(i18n("About the KDVI plugin..."), 0, this, SLOT(about()), actionCollection(), "about_kdvi");
  new KAction(i18n("Help on the KDVI plugin..."), 0, this, SLOT(helpme()), actionCollection(), "help_dvi");
  new KAction(i18n("Report Bug in the KDVI plugin..."), 0, this, SLOT(bugform()), actionCollection(), "bug_dvi");

  setXMLFile("kdvi_part.rc");

  scrollView()->addChild(window);
  connect(window, SIGNAL(request_goto_page(int, int)), this, SLOT(goto_page(int, int) ) );

  readSettings();
  enableActions(false);
}


KDVIMultiPage::~KDVIMultiPage()
{
  if (timer_id != -1)
    killTimer(timer_id);
  timer_id = -1;
  writeSettings();
}


bool KDVIMultiPage::openFile()
{
  bool r = window->setFile(m_file);
  window->gotoPage(1);
  window->changePageSize(); //  This also calles drawPage();

  emit numberOfPages(window->totalPages());
  scrollView()->resizeContents(window->width(), window->height());
  emit previewChanged(true);
  enableActions(r);

  return r;
}


bool KDVIMultiPage::closeURL()
{
  window->setFile(""); // That means: close the file. Resize the widget to 0x0.
  emit previewChanged(false);
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
  window->gotoPage(page+1);

  emit previewChanged(true);

  return true;
}

void KDVIMultiPage::goto_page(int page, int y)
{
  window->gotoPage(page+1, y);
  scrollView()->ensureVisible(scrollView()->width()/2, y );

  emit previewChanged(true);
  emit pageInfo(window->totalPages(), page );
}


double KDVIMultiPage::setZoom(double zoom)
{
  if (zoom < KViewPart::minZoom/1000.0)
    zoom = KViewPart::minZoom/1000.0;
  if (zoom > KViewPart::maxZoom/1000.0)
    zoom = KViewPart::maxZoom/1000.0;

  double z = window->setZoom(zoom);
  scrollView()->resizeContents(window->width(), window->height());

  return z;
}


extern unsigned int page_w, page_h;

double KDVIMultiPage::zoomForHeight(int height)
{
  return (window->zoom() * (double)height)/(double)page_h;
}


double KDVIMultiPage::zoomForWidth(int width)
{
  return (window->zoom() * (double)width)/(double)page_w;
}


void KDVIMultiPage::setPaperSize(double w, double h)
{
  window->setPaper(w, h);
}


bool KDVIMultiPage::preview(QPainter *p, int w, int h)
{
  QPixmap *map = window->pix();

  if (!map)
    return false;

  p->scale((double)w/(double)map->width(), (double)h/(double)map->height());
  p->drawPixmap(0, 0, *map);

  return true;
}


void KDVIMultiPage::doInfo(void)
{
  window->showInfo();
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
  if (options) {
    options->show();
    return;
  }

  options = new OptionDialog(window);
  connect(options, SIGNAL(preferencesChanged()), this, SLOT(preferencesChanged()));
  options->show();
}

void KDVIMultiPage::about()
{
  KAboutDialog *ab = new KAboutDialog(KAboutDialog::AbtAppStandard, 
				      i18n("the KDVI plugin"), 
				      KAboutDialog::Close, KAboutDialog::Close);

  ab->setProduct("kdvi", "0.9f", QString::null, QString::null);
  ab->addTextPage (i18n("About"), 
		   i18n("A previewer for Device Independent files (DVI files) produced "
			"by the TeX typesetting system.<br>"
			"Based on kdvi 0.4.3 and on xdvik, version 18f.<br><hr>"
			"For latest information, visit "
			"<a href=\"http://devel-home.kde.org/~kdvi\">KDVI's Homepage</a>."),
		   true);
  ab->addTextPage (i18n("Authors"), 
		   i18n("Stefan Kebekus<br>"
			"<a href=\"http://btm8x5.mat.uni-bayreuth.de/~kebekus\">"
			"http://btm8x5.mat.uni-bayreuth.de/~kebekus</a><br>"
			"<a href=\"mailto:kebekus@kde.org\">kebekus@kde.org</a><br>"
			"Current maintainer of kdvi. Major rewrite of version 0.4.3."
			"Implementation of hyperlinks.<br>"
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
  KAboutData *kab = new KAboutData("kdvi", I18N_NOOP("KDVI"), "0.9f", 0, 0, 0, 0, 0);
  KBugReport *kbr = new KBugReport(0, true, kab );
  kbr->show();
}

void KDVIMultiPage::helpme()
{
  kapp->invokeHelp( "", "kdvi" );
}

void KDVIMultiPage::preferencesChanged()
{
#ifdef DEBUG_MULTIPAGE
  kdDebug(4300) << "preferencesChanged" << endl;
#endif

  KConfig *config = instance()->config();

  QString s;

  config->reparseConfiguration();
  config->setGroup( "kdvi" );

  int mfmode = config->readNumEntry( "MetafontMode", DefaultMFMode );
  if (( mfmode < 0 ) || (mfmode >= NumberOfMFModes))
    config->writeEntry( "MetafontMode", mfmode = DefaultMFMode );
  window->setMetafontMode( mfmode );

  int makepk = config->readBoolEntry( "MakePK", true );
  if ( makepk != window->makePK() )
    window->setMakePK( makepk );

  int showPS = config->readNumEntry( "ShowPS", 1 );
  if (showPS != window->showPS())
    window->setShowPS(showPS);

  int showHyperLinks = config->readNumEntry( "ShowHyperLinks", 1 );
  if (showHyperLinks != window->showHyperLinks())
    window->setShowHyperLinks(showHyperLinks);
}


bool KDVIMultiPage::print(const QStringList &pages, int current)
{
  Print * printdlg = new Print(window, "printdlg");

  printdlg->setFile(m_file);
  printdlg->setCurrentPage(current+1, window->totalPages());
  printdlg->setMarkList(pages);
  printdlg->exec();

  delete printdlg;

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
// second and check if the file becaome good. If so, we stop the
// timer. It may well happen that KDirWatch calls us several times
// while we are waiting for the file to become good, but that does not
// do any harm.
//
// -- Stefan Kebekus.

void KDVIMultiPage::timerEvent( QTimerEvent * )
{
#ifdef DEBUG_MULTIPAGE
  kdDebug(4300) << "Timer Event " << endl;
#endif
  reload();
}

void KDVIMultiPage::reload()
{
#ifdef DEBUG_MULTIPAGE
  kdDebug(4300) << "Reload file " << m_file << endl;
#endif

  if (window->correctDVI(m_file)) {
    killTimer(timer_id);
    timer_id = -1;
    int currsav = window->curr_page();

    bool r = window->setFile(m_file);
    enableActions(r);

    // Go to the old page and tell kviewshell where we are.
    window->gotoPage(currsav);
    // We don't use "currsav" here, because that page may no longer
    // exist. In that case, gotoPage already selected another page.
    emit pageInfo(window->totalPages(), window->curr_page()-1 ); 

    //    scrollView()->resizeContents(window->width(), window->height());
    emit previewChanged(true);
  } else {
    if (timer_id == -1)
      timer_id = startTimer(1000);
  }
}

void KDVIMultiPage::enableActions(bool b)
{
  docInfoAction->setEnabled(b);
  exportPSAction->setEnabled(b);
  exportPDFAction->setEnabled(b);
}
