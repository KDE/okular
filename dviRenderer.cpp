// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
//
// Class: dviRenderer
//
// Class for rendering TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2001-2005 Stefan Kebekus
// Distributed under the GPL
//

#include <config.h>

#include "dviRenderer.h"
#include "dviFile.h"
#include "dvisourcesplitter.h"
#include "hyperlink.h"
#include "infodialog.h"
#include "kvs_debug.h"
#include "prebookmark.h"
#include "psgs.h"
#include "renderedDviPagePixmap.h"

#include <kconfig.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kprogress.h>
#include <kstandarddirs.h>

#include <qapplication.h>
#include <qcheckbox.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qptrstack.h>
#include <qregexp.h>
#include <qvbox.h>

//#define DEBUG_DVIRENDERER

QPainter *foreGroundPainter; // QPainter used for text


//------ now comes the dviRenderer class implementation ----------

dviRenderer::dviRenderer(QWidget *par)
  : DocumentRenderer(par),
    dviFile(0),
    info(new infoDialog(par)),
    resolutionInDPI(0),
    embedPS_progress(0),
    embedPS_numOfProgressedFiles(0),
    shrinkfactor(3),
    source_href(0),
    HTML_href(0),
    editorCommand(""),
    PostScriptOutPutString(0),
    PS_interface(new ghostscript_interface),
    _postscript(false),
    line_boundary_encountered(false),
    word_boundary_encountered(false),
    current_page(0),
    penWidth_in_mInch(0),
    number_of_elements_in_path(0),
    currentlyDrawnPage(0)
{
#ifdef DEBUG_DVIRENDERER
  kdDebug(kvs::dvi) << "dviRenderer( parent=" << par << " )" << endl;
#endif

  connect(&font_pool, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
  connect( &clearStatusBarTimer, SIGNAL(timeout()), this, SLOT(clearStatusBar()) );
  // pass status bar messages through
  connect(PS_interface, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
}


dviRenderer::~dviRenderer()
{
#ifdef DEBUG_DVIRENDERER
  kdDebug(kvs::dvi) << "~dviRenderer" << endl;
#endif

  mutex.lock();
  mutex.unlock();

  delete PS_interface;
  delete dviFile;
}


void dviRenderer::setPrefs(bool flag_showPS, const QString &str_editorCommand, bool useFontHints )
{
  //QMutexLocker locker(&mutex);
  _postscript = flag_showPS;
  editorCommand = str_editorCommand;
  font_pool.setParameters( useFontHints );
  emit(documentIsChanged());
}


void dviRenderer::showInfo()
{
  mutex.lock();
  info->setDVIData(dviFile);
  info->show();
  mutex.unlock();
}


//------ this function calls the dvi interpreter ----------


void dviRenderer::drawPage(double resolution, RenderedDocumentPagePixmap* page)
{
#ifdef DEBUG_DVIRENDERER
  kdDebug(kvs::dvi) << "dviRenderer::drawPage(documentPage *) called, page number " << page->getPageNumber() << endl;
#endif

  // Paranoid safety checks
  if (page == 0) {
    kdError(kvs::dvi) << "dviRenderer::drawPage(documentPage *) called with argument == 0" << endl;
    return;
  }
  if (page->getPageNumber() == 0) {
    kdError(kvs::dvi) << "dviRenderer::drawPage(documentPage *) called for a documentPage with page number 0" << endl;
    return;
  }

  mutex.lock();
  if ( dviFile == 0 ) {
    kdError(kvs::dvi) << "dviRenderer::drawPage(documentPage *) called, but no dviFile class allocated." << endl;
    page->clear();
    mutex.unlock();
    return;
  }
  if (page->getPageNumber() > dviFile->total_pages) {
    kdError(kvs::dvi) << "dviRenderer::drawPage(documentPage *) called for a documentPage with page number " << page->getPageNumber()
                  << " but the current dviFile has only " << dviFile->total_pages << " pages." << endl;
    mutex.unlock();
    return;
  }
  if ( dviFile->dvi_Data() == 0 ) {
    kdError(kvs::dvi) << "dviRenderer::drawPage(documentPage *) called, but no dviFile is loaded yet." << endl;
    page->clear();
    mutex.unlock();
    return;
  }

  if (resolution != resolutionInDPI)
    setResolution(resolution);

  SimplePageSize ps = sizeOfPage(page->getPageNumber());
  int pageHeight = ps.sizeInPixel(resolution).height();
  int pageWidth = ps.sizeInPixel(resolution).width();
  page->resize(pageWidth, pageHeight);

  currentlyDrawnPage     = page;
  shrinkfactor           = 1200/resolutionInDPI;
  current_page           = page->getPageNumber()-1;


  // Reset colors
  colorStack.clear();
  globalColor = Qt::black;

  QApplication::setOverrideCursor( waitCursor );
  foreGroundPainter = page->getPainter();
  if (foreGroundPainter != 0) {
    errorMsg = QString::null;
    draw_page();
    page->returnPainter(foreGroundPainter);
  }
  QApplication::restoreOverrideCursor();
  page->isEmpty = false;
  if (errorMsg.isEmpty() != true) {
    KMessageBox::detailedError(parentWidget,
                               i18n("<qt><strong>File corruption!</strong> KDVI had trouble interpreting your DVI file. Most "
                                    "likely this means that the DVI file is broken.</qt>"),
                               errorMsg, i18n("DVI File Error"));
    errorMsg = QString::null;
    currentlyDrawnPage = 0;
    mutex.unlock();
    return;
  }

  // Tell the user (once) if the DVI file contains source specials
  // ... we don't want our great feature to go unnoticed.
  RenderedDviPagePixmap* currentDVIPage = dynamic_cast<RenderedDviPagePixmap*>(currentlyDrawnPage);
  if (currentDVIPage)
  {
    if ((dviFile->sourceSpecialMarker == true) && (currentDVIPage->sourceHyperLinkList.size() > 0)) {
      dviFile->sourceSpecialMarker = false;
      // Show the dialog as soon as event processing is finished, and
      // the program is idle
      QTimer::singleShot( 0, this, SLOT(showThatSourceInformationIsPresent()) );
    }
  }

  currentlyDrawnPage = 0;
  mutex.unlock();
}


void dviRenderer::getText(RenderedDocumentPagePixmap* page)
{
  bool postscriptBackup = _postscript;
  // Disable postscript-specials temporarely to speed up text extraction.
  _postscript = false;

  drawPage(100.0, page);

  _postscript = postscriptBackup;
}


void dviRenderer::showThatSourceInformationIsPresent()
{
  // In principle, we should use a KMessagebox here, but we want to
  // add a button "Explain in more detail..." which opens the
  // Helpcenter. Thus, we practically re-implement the KMessagebox
  // here. Most of the code is stolen from there.

  // Check if the 'Don't show again' feature was used
  KConfig *config = KGlobal::config();
  KConfigGroupSaver saver( config, "Notification Messages" );
  bool showMsg = config->readBoolEntry( "KDVI-info_on_source_specials", true);

  if (showMsg) {
    KDialogBase dialog(i18n("KDVI: Information"), KDialogBase::Yes, KDialogBase::Yes, KDialogBase::Yes,
                                         parentWidget, "information", true, true,KStdGuiItem::ok() );

    QVBox *topcontents = new QVBox (&dialog);
    topcontents->setSpacing(KDialog::spacingHint()*2);
    topcontents->setMargin(KDialog::marginHint()*2);

    QWidget *contents = new QWidget(topcontents);
    QHBoxLayout * lay = new QHBoxLayout(contents);
    lay->setSpacing(KDialog::spacingHint()*2);

    lay->addStretch(1);
    QLabel *label1 = new QLabel( contents);
    label1->setPixmap(QMessageBox::standardIcon(QMessageBox::Information));
    lay->add( label1 );
    QLabel *label2 = new QLabel( i18n("<qt>This DVI file contains source file information. You may click into the text with the "
                                      "middle mouse button, and an editor will open the TeX-source file immediately.</qt>"),
                                 contents);
    label2->setMinimumSize(label2->sizeHint());
    lay->add( label2 );
    lay->addStretch(1);
    QSize extraSize = QSize(50,30);
    QCheckBox *checkbox = new QCheckBox(i18n("Do not show this message again"), topcontents);
    extraSize = QSize(50,0);
    dialog.setHelpLinkText(i18n("Explain in more detail..."));
    dialog.setHelp("inverse-search", "kdvi");
    dialog.enableLinkedHelp(true);
    dialog.setMainWidget(topcontents);
    dialog.enableButtonSeparator(false);
    dialog.incInitialSize( extraSize );
    dialog.exec();

    showMsg = !checkbox->isChecked();
    if (!showMsg) {
      KConfigGroupSaver saver( config, "Notification Messages" );
      config->writeEntry( "KDVI-info_on_source_specials", showMsg);
    }
    config->sync();
  }
}


void dviRenderer::embedPostScript()
{
#ifdef DEBUG_DVIRENDERER
  kdDebug(kvs::dvi) << "dviRenderer::embedPostScript()" << endl;
#endif

  if (!dviFile)
    return;

  embedPS_progress = new KProgressDialog(parentWidget, "embedPSProgressDialog",
                                         i18n("Embedding PostScript Files"), QString::null, true);
  if (!embedPS_progress)
    return;
  embedPS_progress->setAllowCancel(false);
  embedPS_progress->showCancelButton(false);
  embedPS_progress->setMinimumDuration(400);
  embedPS_progress->progressBar()->setTotalSteps(dviFile->numberOfExternalPSFiles);
  embedPS_progress->progressBar()->setProgress(0);
  embedPS_numOfProgressedFiles = 0;


  Q_UINT16 currPageSav = current_page;
  errorMsg = QString::null;
  for(current_page=0; current_page < dviFile->total_pages; current_page++) {
    if (current_page < dviFile->total_pages) {
      command_pointer = dviFile->dvi_Data() + dviFile->page_offset[current_page];
      end_pointer     = dviFile->dvi_Data() + dviFile->page_offset[current_page+1];
    } else
      command_pointer = end_pointer = 0;

    memset((char *) &currinf.data, 0, sizeof(currinf.data));
    currinf.fonttable = &(dviFile->tn_table);
    currinf._virtual  = NULL;
    prescan(&dviRenderer::prescan_embedPS);
  }

  delete embedPS_progress;
  embedPS_progress = 0;

  if (!errorMsg.isEmpty()) {
    errorMsg = "<qt>" + errorMsg + "</qt>";
    KMessageBox::detailedError(parentWidget, "<qt>" + i18n("Not all PostScript files could be embedded into your document.") + "</qt>", errorMsg);
    errorMsg = QString::null;
  } else
    KMessageBox::information(parentWidget, "<qt>" + i18n("All external PostScript files were embedded into your document. You "
                                                 "will probably want to save the DVI file now.") + "</qt>",
                             QString::null, "embeddingDone");

  // Prescan phase starts here
#ifdef PERFORMANCE_MEASUREMENT
  kdDebug(kvs::dvi) << "Time elapsed till prescan phase starts " << performanceTimer.elapsed() << "ms" << endl;
  QTime preScanTimer;
  preScanTimer.start();
#endif
  dviFile->numberOfExternalPSFiles = 0;
  prebookmarks.clear();
  for(current_page=0; current_page < dviFile->total_pages; current_page++) {
    PostScriptOutPutString = new QString();

    if (current_page < dviFile->total_pages) {
      command_pointer = dviFile->dvi_Data() + dviFile->page_offset[current_page];
      end_pointer     = dviFile->dvi_Data() + dviFile->page_offset[current_page+1];
    } else
      command_pointer = end_pointer = 0;

    memset((char *) &currinf.data, 0, sizeof(currinf.data));
    currinf.fonttable = &(dviFile->tn_table);
    currinf._virtual  = NULL;

    prescan(&dviRenderer::prescan_parseSpecials);

    if (!PostScriptOutPutString->isEmpty())
      PS_interface->setPostScript(current_page, *PostScriptOutPutString);
    delete PostScriptOutPutString;
  }
  PostScriptOutPutString = NULL;


#ifdef PERFORMANCE_MEASUREMENT
  kdDebug(kvs::dvi) << "Time required for prescan phase: " << preScanTimer.restart() << "ms" << endl;
#endif
  current_page = currPageSav;
  _isModified = true;
}


bool dviRenderer::isValidFile(const QString& filename) const
{
  QFile f(filename);
  if (!f.open(IO_ReadOnly))
    return false;

  unsigned char test[4];
  if ( f.readBlock( (char *)test,2)<2 || test[0] != 247 || test[1] != 2 )
    return false;

  int n = f.size();
  if ( n < 134 )        // Too short for a dvi file
    return false;
  f.at( n-4 );

  unsigned char trailer[4] = { 0xdf,0xdf,0xdf,0xdf };

  if ( f.readBlock( (char *)test, 4 )<4 || strncmp( (char *)test, (char *) trailer, 4 ) )
    return false;
  // We suppose now that the dvi file is complete and OK
  return true;
}


bool dviRenderer::setFile(const QString &fname, const KURL &base)
{
#ifdef DEBUG_DVIRENDERER
  kdDebug(kvs::dvi) << "dviRenderer::setFile( fname='" << fname << "', ref='" << ref << "', sourceMarker=" << sourceMarker << " )" << endl;
#endif

  //QMutexLocker lock(&mutex);

  QFileInfo fi(fname);
  QString   filename = fi.absFilePath();

  // If fname is the empty string, then this means: "close". Delete
  // the dvifile and the pixmap.
  if (fname.isEmpty()) {
    // Delete DVI file
    info->setDVIData(0);
    delete dviFile;
    dviFile = 0;
    return true;
  }


  // Make sure the file actually exists.
  if (!fi.exists() || fi.isDir()) {
    KMessageBox::error( parentWidget,
                        i18n("<qt><strong>File error.</strong> The specified file '%1' does not exist. "
                             "KDVI already tried to add the ending '.dvi'.</qt>").arg(filename),
                        i18n("File Error!"));
    return false;
  }

  // Check if we are really loading a DVI file, and complain about the
  // mime type, if the file is not DVI. Perhaps we should move the
  // procedure later to the kviewpart, instead of the implementaton in
  // the multipage.
  const QString mimetype = KMimeType::findByFileContent(fname)->name();
  if (mimetype != "application/x-dvi") {
    KMessageBox::sorry( parentWidget,
                        i18n( "<qt>Could not open file <nobr><strong>%1</strong></nobr> which has "
                              "type <strong>%2</strong>. KDVI can only load DVI (.dvi) files.</qt>" )
                        .arg( fname )
                        .arg( mimetype ) );
    return false;
  }

  QApplication::setOverrideCursor( waitCursor );
  dvifile *dviFile_new = new dvifile(filename, &font_pool);

  if ((dviFile == 0) || (dviFile->filename != filename))
    dviFile_new->sourceSpecialMarker = true;
  else
    dviFile_new->sourceSpecialMarker = false;

  if ((dviFile_new->dvi_Data() == NULL)||(dviFile_new->errorMsg.isEmpty() != true)) {
    QApplication::restoreOverrideCursor();
    if (dviFile_new->errorMsg.isEmpty() != true)
      KMessageBox::detailedError(parentWidget,
                                 i18n("<qt>File corruption! KDVI had trouble interpreting your DVI file. Most "
                                      "likely this means that the DVI file is broken.</qt>"),
                                 dviFile_new->errorMsg, i18n("DVI File Error"));
    delete dviFile_new;
    return false;
  }

  delete dviFile;
  dviFile = dviFile_new;
  numPages = dviFile->total_pages;
  info->setDVIData(dviFile);
  _isModified = false;
  baseURL = base;

  font_pool.setExtraSearchPath( fi.dirPath(true) );
  font_pool.setCMperDVIunit( dviFile->getCmPerDVIunit() );

  // Extract PostScript from the DVI file, and store the PostScript
  // specials in PostScriptDirectory, and the headers in the
  // PostScriptHeaderString.
  PS_interface->clear();

  // If the DVI file comes from a remote URL (e.g. downloaded from a
  // web server), we limit the PostScript files that can be accessed
  // by this file to the download directory, in order to limit the
  // possibilities of a denial of service attack.
  QString includePath;
  if (!baseURL.isLocalFile()) {
    includePath = filename;
    includePath.truncate(includePath.findRev('/'));
  }
  PS_interface->setIncludePath(includePath);

  // We will also generate a list of hyperlink-anchors and source-file
  // anchors in the document. So declare the existing lists empty.
  anchorList.clear();
  sourceHyperLinkAnchors.clear();
  bookmarks.clear();
  prebookmarks.clear();

  if (dviFile->page_offset.isEmpty() == true)
    return false;

  // Locate fonts.
  font_pool.locateFonts();

  // Update the list of fonts in the info window
  if (info != 0)
    info->setFontInfo(&font_pool);

  // We should pre-scan the document now (to extract embedded,
  // PostScript, Hyperlinks, ets).

  // PRESCAN STARTS HERE
#ifdef PERFORMANCE_MEASUREMENT
  kdDebug(kvs::dvi) << "Time elapsed till prescan phase starts " << performanceTimer.elapsed() << "ms" << endl;
  QTime preScanTimer;
  preScanTimer.start();
#endif
  dviFile->numberOfExternalPSFiles = 0;
  Q_UINT16 currPageSav = current_page;
  prebookmarks.clear();

  for(current_page=0; current_page < dviFile->total_pages; current_page++) {
    PostScriptOutPutString = new QString();

    if (current_page < dviFile->total_pages) {
      command_pointer = dviFile->dvi_Data() + dviFile->page_offset[current_page];
      end_pointer     = dviFile->dvi_Data() + dviFile->page_offset[current_page+1];
    } else
      command_pointer = end_pointer = 0;

    memset((char *) &currinf.data, 0, sizeof(currinf.data));
    currinf.fonttable = &(dviFile->tn_table);
    currinf._virtual  = NULL;
    prescan(&dviRenderer::prescan_parseSpecials);

    if (!PostScriptOutPutString->isEmpty())
      PS_interface->setPostScript(current_page, *PostScriptOutPutString);
    delete PostScriptOutPutString;
  }
  PostScriptOutPutString = NULL;

  // Generate the list of bookmarks
  bookmarks.clear();
  QPtrStack<Bookmark> stack;
  stack.setAutoDelete (false);
  QValueVector<PreBookmark>::iterator it;
  for( it = prebookmarks.begin(); it != prebookmarks.end(); ++it ) {
    Bookmark *bmk = new Bookmark((*it).title, findAnchor((*it).anchorName));
    if (stack.isEmpty())
      bookmarks.append(bmk);
    else {
      stack.top()->subordinateBookmarks.append(bmk);
      stack.remove();
    }
    for(int i=0; i<(*it).noOfChildren; i++)
      stack.push(bmk);
  }
  prebookmarks.clear();


#ifdef PERFORMANCE_MEASUREMENT
  kdDebug(kvs::dvi) << "Time required for prescan phase: " << preScanTimer.restart() << "ms" << endl;
#endif
  current_page = currPageSav;
  // PRESCAN ENDS HERE


  pageSizes.resize(0);
  if (dviFile->suggestedPageSize != 0) {
    // Fill the vector pageSizes with total_pages identical entries
    pageSizes.resize(dviFile->total_pages, *(dviFile->suggestedPageSize));
  }

  QApplication::restoreOverrideCursor();
  return true;
}


Anchor dviRenderer::parseReference(const QString &reference)
{
  mutex.lock();

#ifdef DEBUG_DVIRENDERER
  kdError(kvs::dvi) << "dviRenderer::parseReference( " << reference << " ) called" << endl;
#endif

  if (dviFile == 0) {
    mutex.unlock();
    return Anchor();
  }

  // case 1: The reference is a number, which we'll interpret as a
  // page number.
  bool ok;
  int page = reference.toInt ( &ok );
  if (ok == true) {
    if (page < 0)
      page = 0;
    if (page > dviFile->total_pages)
      page = dviFile->total_pages;

    mutex.unlock();
    return Anchor(page, Length() );
  }

  // case 2: The reference is of form "src:1111Filename", where "1111"
  // points to line number 1111 in the file "Filename". KDVI then
  // looks for source specials of the form "src:xxxxFilename", and
  // tries to find the special with the biggest xxxx
  if (reference.find("src:",0,false) == 0) {

    // Extract the file name and the numeral part from the reference string
    DVI_SourceFileSplitter splitter(reference, dviFile->filename);
    Q_UINT32 refLineNumber = splitter.line();
    QString  refFileName   = splitter.filePath();

    if (sourceHyperLinkAnchors.isEmpty()) {
      KMessageBox::sorry(parentWidget, i18n("<qt>You have asked KDVI to locate the place in the DVI file which corresponds to "
                                    "line %1 in the TeX-file <strong>%2</strong>. It seems, however, that the DVI file "
                                    "does not contain the necessary source file information. "
                                    "We refer to the manual of KDVI for a detailed explanation on how to include this "
                                    "information. Press the F1 key to open the manual.</qt>").arg(refLineNumber).arg(refFileName),
                         i18n("Could Not Find Reference"));
      mutex.unlock();
      return Anchor();
    }

    // Go through the list of source file anchors, and find the anchor
    // whose line number is the biggest among those that are smaller
    // than the refLineNumber. That way, the position in the DVI file
    // which is highlighted is always a little further up than the
    // position in the editor, e.g. if the DVI file contains
    // positional information at the beginning of every paragraph,
    // KDVI jumps to the beginning of the paragraph that the cursor is
    // in, and never to the next paragraph. If source file anchors for
    // the refFileName can be found, but none of their line numbers is
    // smaller than the refLineNumber, the reason is most likely, that
    // the cursor in the editor stands somewhere in the preamble of
    // the LaTeX file. In that case, we jump to the beginning of the
    // document.
    bool anchorForRefFileFound = false; // Flag that is set if source file anchors for the refFileName could be found at all

    QValueVector<DVI_SourceFileAnchor>::iterator bestMatch = sourceHyperLinkAnchors.end();
    QValueVector<DVI_SourceFileAnchor>::iterator it;
    for( it = sourceHyperLinkAnchors.begin(); it != sourceHyperLinkAnchors.end(); ++it )
      if (refFileName.stripWhiteSpace() == it->fileName.stripWhiteSpace()
      || refFileName.stripWhiteSpace() == it->fileName.stripWhiteSpace() + ".tex"
      ) {
        anchorForRefFileFound = true;

        if ( (it->line <= refLineNumber) &&
             ( (bestMatch == sourceHyperLinkAnchors.end()) || (it->line > bestMatch->line) ) )
          bestMatch = it;
      }

    if (bestMatch != sourceHyperLinkAnchors.end()) {
      mutex.unlock();
      return Anchor(bestMatch->page, bestMatch->distance_from_top);
    } else
      if (anchorForRefFileFound == false)
        KMessageBox::sorry(parentWidget, i18n("<qt>KDVI was not able to locate the place in the DVI file which corresponds to "
                                              "line %1 in the TeX-file <strong>%2</strong>.</qt>").arg(refLineNumber).arg(refFileName),
                           i18n( "Could Not Find Reference" ));
      else {
        mutex.unlock();
        return Anchor();
      }
    mutex.unlock();
    return Anchor();
  }
  mutex.unlock();
  return Anchor();
}


void dviRenderer::setResolution(double resolution_in_DPI)
{
  // Ignore minute changes. The difference to the current value would
  // hardly be visible anyway. That saves a lot of re-painting,
  // e.g. when the user resizes the window, and a flickery mouse
  // changes the window size by 1 pixel all the time.
  if (fabs(resolutionInDPI-resolution_in_DPI) < 1)
    return;

  resolutionInDPI = resolution_in_DPI;

  // Pass the information on to the font pool.
  font_pool.setDisplayResolution( resolutionInDPI );
  shrinkfactor = 1200/resolutionInDPI;
  return;
}


void dviRenderer::clearStatusBar()
{
  emit setStatusBarText( QString::null );
}


void dviRenderer::handleSRCLink(const QString &linkText, const QPoint& point, DocumentWidget *win)
{
  KSharedPtr<DVISourceEditor> editor(new DVISourceEditor(*this, parentWidget, linkText, point, win));
  if (editor->started())
    editor_ = editor;
}


QString dviRenderer::PDFencodingToQString(const QString& _pdfstring)
{
  // This method locates special PDF characters in a string and
  // replaces them by UTF8. See Section 3.2.3 of the PDF reference
  // guide for information.
  QString pdfstring = _pdfstring;
  pdfstring = pdfstring.replace("\\n", "\n");
  pdfstring = pdfstring.replace("\\r", "\n");
  pdfstring = pdfstring.replace("\\t", "\t");
  pdfstring = pdfstring.replace("\\f", "\f");
  pdfstring = pdfstring.replace("\\(", "(");
  pdfstring = pdfstring.replace("\\)", ")");
  pdfstring = pdfstring.replace("\\\\", "\\");

  // Now replace octal character codes with the characters they encode
  int pos;
  QRegExp rx( "(\\\\)(\\d\\d\\d)" );  // matches "\xyz" where x,y,z are numbers
  while((pos = rx.search( pdfstring )) != -1) {
    pdfstring = pdfstring.replace(pos, 4, QChar(rx.cap(2).toInt(0,8)));
  }
  rx.setPattern( "(\\\\)(\\d\\d)" );  // matches "\xy" where x,y are numbers
  while((pos = rx.search( pdfstring )) != -1) {
    pdfstring = pdfstring.replace(pos, 3, QChar(rx.cap(2).toInt(0,8)));
  }
  rx.setPattern( "(\\\\)(\\d)" );  // matches "\x" where x is a number
  while((pos = rx.search( pdfstring )) != -1) {
    pdfstring = pdfstring.replace(pos, 4, QChar(rx.cap(2).toInt(0,8)));
  }
  return pdfstring;
}


void dviRenderer::exportPDF()
{
  KSharedPtr<DVIExport> exporter(new DVIExportToPDF(*this, parentWidget));
  if (exporter->started())
    all_exports_[exporter.data()] = exporter;
}


void dviRenderer::exportPS(const QString& fname, const QStringList& options, KPrinter* printer)
{
  KSharedPtr<DVIExport> exporter(new DVIExportToPS(*this, parentWidget, fname, options, printer));
  if (exporter->started())
    all_exports_[exporter.data()] = exporter;
}


void dviRenderer::update_info_dialog(const QString& text, bool clear)
{
  if (clear)
    info->clear(text);
  else
    info->outputReceiver(text);
}


void dviRenderer::editor_finished(const DVISourceEditor*)
{
  // KSharedPtr will delete existing pointer.
  editor_ = 0;
}


void dviRenderer::export_finished(const DVIExport* key)
{
  typedef QMap<const DVIExport*, KSharedPtr<DVIExport> > ExportMap;
  ExportMap::iterator it = all_exports_.find(key);
  if (it != all_exports_.end())
    all_exports_.erase(key);
}

#include "dviRenderer.moc"
