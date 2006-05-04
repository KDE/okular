// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
#include <config.h>

#include "kdvi_multipage.h"
#include "dviFile.h"
#include "dviPageCache.h"
#include "dviWidget.h"
#include "kprinterwrapper.h"
#include "kvs_debug.h"
#include "optionDialogFontsWidget.h"
#include "optionDialogSpecialWidget.h"
#include "performanceMeasurement.h"
#include "prefs.h"

#include <kaboutdata.h>
#include <kaction.h>
#include <kconfigdialog.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kparts/genericfactory.h>
#include <kstdaction.h>
#include <ktip.h>

#include <QLabel>
#include <QTemporaryFile>
#include <QTimer>


//#define KDVI_MULTIPAGE_DEBUG

#ifdef PERFORMANCE_MEASUREMENT
// These objects are explained in the file "performanceMeasurement.h"
QTime performanceTimer;
int  performanceFlag = 0;
#endif

typedef KParts::GenericFactory<KDVIMultiPage> KDVIMultiPageFactory;
K_EXPORT_COMPONENT_FACTORY(kdvipart, KDVIMultiPageFactory)



KDVIMultiPage::KDVIMultiPage(QWidget *parentWidget, const char *widgetName, QObject *parent,
                             const char *name, const QStringList& args)
  : KMultiPage(parentWidget, parent), DVIRenderer(parentWidget)
{
  Q_UNUSED(args);
#ifdef PERFORMANCE_MEASUREMENT
  performanceTimer.start();
#endif

  searchUsed = false;

  setInstance(KDVIMultiPageFactory::instance());

  // Points to the same object as renderer to avoid downcasting.
  // FIXME: Remove when the API of the Renderer-class is finished.
  DVIRenderer.setName("DVI renderer");
  setRenderer(&DVIRenderer);

  docInfoAction    = new KAction(i18n("Document &Info"), "info", 0, &DVIRenderer, SLOT(showInfo()), actionCollection(), "info_dvi");
  embedPSAction    = new KAction(i18n("Embed External PostScript Files..."), 0, this, SLOT(slotEmbedPostScript()), actionCollection(), "embed_postscript");
  KAction *action = new KAction(i18n("Enable All Warnings && Messages"), actionCollection(), "enable_msgs");
  connect(action, SIGNAL(triggered(bool) ), SLOT(doEnableWarnings()));
  exportPSAction   = new KAction(i18n("PostScript..."), 0, &DVIRenderer, SLOT(exportPS()), actionCollection(), "export_postscript");
  exportPDFAction  = new KAction(i18n("PDF..."), 0, &DVIRenderer, SLOT(exportPDF()), actionCollection(), "export_pdf");

  KStdAction::tipOfDay(this, SLOT(showTip()), actionCollection(), "help_tipofday");

  setXMLFile("kdvi_part.rc");

  preferencesChanged();

  enableActions(false);
  // Show tip of the day, when the first main window is shown.
  QTimer::singleShot(0,this,SLOT(showTipOnStart()));
}


KDVIMultiPage::~KDVIMultiPage()
{
  delete docInfoAction;
  delete embedPSAction;
  delete exportPSAction;
  delete exportPDFAction;

  Prefs::writeConfig();
}


KAboutData* KDVIMultiPage::createAboutData()
{
  KAboutData* about = new KAboutData("kdvi", I18N_NOOP("KDVI"), "1.3",
                      I18N_NOOP("A previewer for Device Independent files (DVI files) produced by the TeX typesetting system."),
                     KAboutData::License_GPL,
                     "Markku Hinhala, Stephan Kebekus",
                     I18N_NOOP("This program displays Device Independent (DVI) files which are produced by the TeX typesetting system.\n"
                     "KDVI 1.3 is based on original code from KDVI version 0.43 and xdvik."));

  about->addAuthor ("Stefan Kebekus",
                    I18N_NOOP("Current Maintainer."),
                    "kebekus@kde.org",
                    "http://www.mi.uni-koeln.de/~kebekus");

  about->addAuthor ("Markku Hinhala", I18N_NOOP("Author of kdvi 0.4.3"));
  about->addAuthor ("Nicolai Langfeldt", I18N_NOOP("Maintainer of xdvik"));
  about->addAuthor ("Paul Vojta", I18N_NOOP("Author of xdvi"));
  about->addCredit ("Philipp Lehmann", I18N_NOOP("Testing and bug reporting."));
  about->addCredit ("Wilfried Huss", I18N_NOOP("Re-organisation of source code."));

  return about;
}


void KDVIMultiPage::slotEmbedPostScript()
{
  DVIRenderer.embedPostScript();
  if (DVIRenderer.isModified())
    emit renderModeChanged();
  setEmbedPostScriptAction();
}


void KDVIMultiPage::setEmbedPostScriptAction()
{
  if ((DVIRenderer.dviFile == 0) || (DVIRenderer.dviFile->numberOfExternalPSFiles == 0))
    embedPSAction->setEnabled(false);
  else
    embedPSAction->setEnabled(true);
}


bool KDVIMultiPage::slotSave(const QString &fileName)
{
  // Paranoid safety checks
  if (DVIRenderer.dviFile == 0)
    return false;
  if (DVIRenderer.dviFile->dvi_Data() == 0)
    return false;
  if (fileName.isEmpty())
    return false;

  bool r = DVIRenderer.dviFile->saveAs(fileName);
  if (r == false)
    KMessageBox::error(parentWdg, i18n("<qt>Error saving the document to the file <strong>%1</strong>. The document is <strong>not</strong> saved.</qt>", fileName),
                       i18n("Error saving document"));
  return r;
}


void KDVIMultiPage::setFile(bool r)
{
  enableActions(r);
}


QStringList KDVIMultiPage::fileFormats() const
{
  QStringList r;
  r << i18n("*.dvi *.DVI|TeX Device Independent Files (*.dvi)");
  return r;
}


void KDVIMultiPage::addConfigDialogs(KConfigDialog* configDialog)
{
  static optionDialogFontsWidget* fontConfigWidget = 0;

  fontConfigWidget = new optionDialogFontsWidget(parentWdg);
  optionDialogSpecialWidget* specialConfigWidget = new optionDialogSpecialWidget(parentWdg);

  configDialog->addPage(fontConfigWidget, Prefs::self(), i18n("TeX Fonts"), "fonts");
  configDialog->addPage(specialConfigWidget, Prefs::self(), i18n("DVI Specials"), "dvi");
  configDialog->setHelp("preferences", "kdvi");
}


void KDVIMultiPage::preferencesChanged()
{
  // Call method from parent class
  KMultiPage::preferencesChanged();
#ifdef  KDVI_MULTIPAGE_DEBUG
  kDebug(kvs::dvi) << "preferencesChanged" << endl;
#endif

  bool showPS = Prefs::showPS();
  bool useFontHints = Prefs::useFontHints();

  DVIRenderer.setPrefs( showPS, Prefs::editorCommand(), useFontHints);
}


void KDVIMultiPage::print()
{
  // Obtain a fully initialized KPrinter structure, and disable all
  // entries in the "Page Size & Placement" tab of the printer dialog.
  KPrinter *printer = getPrinter(false);
  // Abort with an error message if no KPrinter could be initialized
  if (printer == 0) {
    kError(kvs::dvi) << "KPrinter not available" << endl;
    return;
  }

  // Show the printer options dialog. Return immediately if the user
  // aborts.
  if (!printer->setup(parentWdg, i18n("Print %1", m_file.section('/', -1)) ))
    return;

  // This funny method call is necessary for the KPrinter to return
  // proper results in printer->orientation() below. It seems that
  // KPrinter does some options parsing in that method.
  ((KDVIPrinterWrapper *)printer)->doPreparePrinting();
  if (printer->pageList().isEmpty()) {
    KMessageBox::error( parentWdg,
            i18n("The list of pages you selected was empty.\n"
                 "Maybe you made an error in selecting the pages, "
                 "e.g. by giving an invalid range like '7-2'.") );
    return;
  }

  // Turn the results of the options requestor into a list arguments
  // which are used by dvips.
  QStringList dvips_options;
  // Print in reverse order.
  if ( printer->pageOrder() == KPrinter::LastPageFirst )
    dvips_options << "-r";
  // Print only odd pages.
  if ( printer->pageSet() == KPrinter::OddPages )
    dvips_options << "-A";
  // Print only even pages.
  if ( printer->pageSet() == KPrinter::EvenPages )
    dvips_options << "-B";
  // We use the printer->pageSize() method to find the printer page
  // size, and pass that information on to dvips. Unfortunately, dvips
  // does not understand all of these; what exactly dvips understands,
  // depends on its configuration files. Consequence: expect problems
  // with unusual paper sizes.
  switch( printer->pageSize() ) {
  case KPrinter::A4:
    dvips_options << "-t" << "a4";
    break;
  case KPrinter::B5:
    dvips_options << "-t" << "b5";
    break;
    case KPrinter::Letter:
      dvips_options << "-t" << "letter";
      break;
  case KPrinter::Legal:
    dvips_options << "-t" << "legal";
    break;
    case KPrinter::Executive:
      dvips_options << "-t" << "executive";
      break;
  case KPrinter::A0:
    dvips_options << "-t" << "a0";
    break;
  case KPrinter::A1:
    dvips_options << "-t" << "a1";
    break;
  case KPrinter::A2:
    dvips_options << "-t" << "a2";
    break;
  case KPrinter::A3:
    dvips_options << "-t" << "a3";
    break;
  case KPrinter::A5:
    dvips_options << "-t" << "a5";
    break;
  case KPrinter::A6:
    dvips_options << "-t" << "a6";
    break;
  case KPrinter::A7:
    dvips_options << "-t" << "a7";
    break;
  case KPrinter::A8:
    dvips_options << "-t" << "a8";
      break;
  case KPrinter::A9:
    dvips_options << "-t" << "a9";
    break;
  case KPrinter::B0:
    dvips_options << "-t" << "b0";
    break;
  case KPrinter::B1:
    dvips_options << "-t" << "b1";
    break;
  case KPrinter::B10:
    dvips_options << "-t" << "b10";
    break;
  case KPrinter::B2:
    dvips_options << "-t" << "b2";
    break;
  case KPrinter::B3:
    dvips_options << "-t" << "b3";
    break;
  case KPrinter::B4:
    dvips_options << "-t" << "b4";
    break;
  case KPrinter::B6:
    dvips_options << "-t" << "b6";
    break;
  case KPrinter::B7:
    dvips_options << "-t" << "b7";
    break;
  case KPrinter::B8:
    dvips_options << "-t" << "b8";
    break;
  case KPrinter::B9:
    dvips_options << "-t" << "b9";
    break;
  case KPrinter::C5E:
    dvips_options << "-t" << "c5e";
    break;
  case KPrinter::Comm10E:
    dvips_options << "-t" << "comm10e";
    break;
  case KPrinter::DLE:
    dvips_options << "-t" << "dle";
    break;
  case KPrinter::Folio:
    dvips_options << "-t" << "folio";
    break;
  case KPrinter::Ledger:
    dvips_options << "-t" << "ledger";
    break;
  case KPrinter::Tabloid:
    dvips_options << "-t" << "tabloid";
    break;
  default:
    break;
  }
  // Orientation
  if ( printer->orientation() == KPrinter::Landscape )
    dvips_options << "-t" << "landscape";


  // List of pages to print.
  QList<int> pageList = printer->pageList();
  QString pages;
  int commaflag = 0;
  for( QList<int>::ConstIterator it = pageList.begin(); it != pageList.end(); ++it ) {
    if (commaflag == 1)
      pages +=  QString(",");
    else
      commaflag = 1;
    pages += QString("%1").arg(*it);
  }
  if (!pages.isEmpty())
    dvips_options << "-pp" << pages;

  // Now print. For that, export the DVI-File to PostScript. Note that
  // dvips will run concurrently to keep the GUI responsive, keep log
  // of dvips and allow abort. Giving a non-zero printer argument
  // means that the dvi-widget will print the file when dvips
  // terminates, and then delete the output file.
  QTemporaryFile tf;
  // Must open the QTemporaryFile to access the file name.
  tf.setAutoRemove(false);
  tf.open();
  DVIRenderer.exportPS(tf.fileName(), dvips_options, printer);

  // "True" may be a bit euphemistic. However, since dvips runs
  // concurrently, there is no way of telling the result of the
  // printing command at this stage.
  return;
}


void KDVIMultiPage::enableActions(bool b)
{
  KMultiPage::enableActions(b);

  docInfoAction->setEnabled(b);
  exportPSAction->setEnabled(b);
  exportPDFAction->setEnabled(b);

  setEmbedPostScriptAction();
}


void KDVIMultiPage::doEnableWarnings()
{
  KMessageBox::information (parentWdg, i18n("All messages and warnings will now be shown."));
  KMessageBox::enableAllMessages();
  KTipDialog::setShowOnStart(true);
}


void KDVIMultiPage::showTip()
{
  KTipDialog::showTip(parentWdg, "kdvi/tips", true);
}


void KDVIMultiPage::showTipOnStart()
{
  KTipDialog::showTip(parentWdg, "kdvi/tips");
}


DocumentWidget* KDVIMultiPage::createDocumentWidget(PageView *parent, DocumentPageCache *cache)
{
  DVIWidget* documentWidget = new DVIWidget(parent, cache, "singlePageWidget");

  // Handle source links
  connect(documentWidget, SIGNAL(SRCLink(const QString&, const QPoint&, DocumentWidget*)), getRenderer(),
          SLOT(handleSRCLink(const QString& , const QPoint&, DocumentWidget*)));

  return documentWidget;
}


void KDVIMultiPage::initializePageCache()
{
#warning this method is no longer needed, I guess
  //  pageCache = new DVIPageCache();
}


void KDVIMultiPage::showFindTextDialog()
{
#warning this method is no longer needed, I guess
  /*
  if ((getRenderer().isNull()) || (getRenderer()->supportsTextSearch() == false))
    return;

  if (!searchUsed)
  {
    // WARNING: This text appears several times in the code. Change
    // everywhere, or nowhere!
    if (KMessageBox::warningContinueCancel( parentWdg,
                                            i18n("<qt>This function searches the DVI file for plain text. Unfortunately, this version of "
                                                 "KDVI treats only plain ASCII characters properly. Symbols, ligatures, mathematical "
                                                 "formulae, accented characters, and non-English text, such as Russian or Korean, will "
                                                 "most likely be messed up completely. Continue anyway?</qt>"),
                                            i18n("Function May Not Work as Expected"),
                                            KStdGuiItem::cont(),
                                            "warning_search_text_may_not_work") == KMessageBox::Cancel)
      return;

    // Remember that we don't need to show the warning message again.
    searchUsed = true;
  }

  // Now really show the search widget
  KMultiPage::showFindTextDialog();
  */
}

#include "kdvi_multipage.moc"
