//
// Class: dviWindow
//
// Widget for displaying TeX DVI files.
// Part of KDVI- A previewer for TeX DVI files.
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qcheckbox.h>
#include <qclipboard.h>
#include <qcursor.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qurl.h>
#include <qvbox.h>

#include <kapplication.h>
#include <kmessagebox.h>
#include <kmimemagic.h>
#include <kglobal.h>
#include <kdebug.h>
#include <keditcl.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kprinter.h>
#include <kprocess.h>
#include <kprogress.h>
#include <kstandarddirs.h>
#include <kstringhandler.h>
#include <kstdguiitem.h>

#include "documentWidget.h"
#include "dviwin.h"
#include "fontpool.h"
#include "fontprogress.h"
#include "kdvi_multipage.h"
#include "optiondialog.h"
#include "performanceMeasurement.h"
#include "xdvi.h"
#include "zoomlimits.h"

//#define DEBUG_DVIWIN

QPainter foreGroundPaint; // QPainter used for text

//------ now comes the dviWindow class implementation ----------

dviWindow::dviWindow(QWidget *par)
  : info(new infoDialog(par))
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow( parent=" << par << " )" << endl;
#endif

  // initialize the dvi machinery
  dviFile                = 0;

  connect(&font_pool, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
  connect(&font_pool, SIGNAL(fonts_have_been_loaded(fontPool *)), this, SLOT(all_fonts_loaded(fontPool *)));
  connect(&font_pool, SIGNAL(MFOutput(QString)), info, SLOT(outputReceiver(QString)));
  connect(&font_pool, SIGNAL(fonts_have_been_loaded(fontPool *)), info, SLOT(setFontInfo(fontPool *)));
  connect(&font_pool, SIGNAL(new_kpsewhich_run(QString)), info, SLOT(clear(QString)));

  parentWidget = par;
  shrinkfactor = 3;
  current_page = 0;

  connect( &clearStatusBarTimer, SIGNAL(timeout()), this, SLOT(clearStatusBar()) );

  currentlyDrawnPage = 0;
  editorCommand         = "";

  // Calculate the horizontal resolution of the display device.  @@@
  // We assume implicitly that the horizontal and vertical resolutions
  // agree. This is probably not a safe assumption.
  xres                   = QPaintDevice::x11AppDpiX ();
  // Just to make sure that we are never dividing by zero.
  if ((xres < 10)||(xres > 1000))
    xres = 75.0;

  _zoom                  = 1.0;
  paper_width_in_cm           = 21.0; // set A4 paper as default
  paper_height_in_cm          = 29.7;

  PostScriptOutPutString = NULL;
  HTML_href              = NULL;
  _postscript            = 0;
  _showHyperLinks        = true;
  reference              = QString::null;

  // Storage used for dvips and friends, i.e. for the "export" functions.
  proc                   = 0;
  progress               = 0;
  export_printer         = 0;
  export_fileName        = "";
  export_tmpFileName     = "";
  export_errorString     = "";

  PS_interface           = new ghostscript_interface(0.0, 0, 0);
  // pass status bar messages through
  connect(PS_interface, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
}


dviWindow::~dviWindow()
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "~dviWindow" << endl;
#endif

  delete PS_interface;
  delete proc;
  delete dviFile;
  // Don't delete the export printer. This is owned by the
  // kdvi_multipage.
  export_printer = 0;
}


void dviWindow::setPrefs(bool flag_showPS, bool flag_showHyperLinks, const QString &str_editorCommand, 
			 unsigned int MetaFontMode, bool makePK, bool useType1Fonts, bool useFontHints )
{
  _postscript = flag_showPS;
  _showHyperLinks = flag_showHyperLinks;
  editorCommand = str_editorCommand;
  font_pool.setParameters(MetaFontMode, makePK, useType1Fonts, useFontHints );
  emit(needsRepainting());
}


void dviWindow::showInfo(void)
{
  info->setDVIData(dviFile);
  // Call check_if_fonts_filenames_are_looked_up() to make sure that
  // the fonts_info is emitted. That way, the infoDialog will know
  // about the fonts and their status.
  font_pool.check_if_fonts_filenames_are_looked_up();
  info->show();
}


void dviWindow::setPaper(double width_in_cm, double height_in_cm)
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow::setPaper( width_in_cm=" << width_in_cm << ", height_in_cm=" << height_in_cm << " )" << endl;
#endif

  paper_width_in_cm      = width_in_cm;
  paper_height_in_cm     = height_in_cm;
  changePageSize();
}


//------ this function calls the dvi interpreter ----------


void dviWindow::drawPage(documentPage *page)
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow::drawPage(documentPage *) called" << endl;
#endif

  // Paranoid safety checks
  if (page == 0) {
    kdError(4300) << "dviWindow::drawPage(documentPage *) called with argument == 0" << endl; 
    return;
  }
  if (page->getPageNumber() == 0) {
    kdError(4300) << "dviWindow::drawPage(documentPage *) called for a documentPage with page number 0" << endl;
    return;
  }
  if ( dviFile == 0 ) {
    kdError(4300) << "dviWindow::drawPage(documentPage *) called, but no dviFile class allocated." << endl;
    page->clear();
    return;
  }
  if (page->getPageNumber() > dviFile->total_pages) {
    kdError(4300) << "dviWindow::drawPage(documentPage *) called for a documentPage with page number " << page->getPageNumber() 
		  << " but the current dviFile has only " << dviFile->total_pages << " pages." << endl;
    return;
  }
  if ( dviFile->dvi_Data == 0 ) {
    kdError(4300) << "dviWindow::drawPage(documentPage *) called, but no dviFile is loaded yet." << endl;
    page->clear();
    return;
  }

  currentlyDrawnPage     = page;;
  shrinkfactor           = MFResolutions[font_pool.getMetafontMode()]/(xres*_zoom);
  current_page           = page->getPageNumber()-1;

  if ( currentlyDrawnPixmap.isNull() ) {
    currentlyDrawnPage = 0;
    return;
  }


  if ( !currentlyDrawnPixmap.paintingActive() ) {
    // Reset colors
    colorStack.clear();
    globalColor = Qt::black;

    // Check if all the fonts are loaded. If that is not the case, we
    // return and do not draw anything. The font_pool will later emit
    // the signal "fonts_are_loaded" and thus trigger a redraw of the
    // page.
    if (font_pool.check_if_fonts_filenames_are_looked_up() == true) {
      foreGroundPaint.begin( &(currentlyDrawnPixmap) );
      QApplication::setOverrideCursor( waitCursor );
      errorMsg = QString::null;
      draw_page();
      foreGroundPaint.drawRect(0, 0, currentlyDrawnPixmap.width(), currentlyDrawnPixmap.height());
      foreGroundPaint.end();
      QApplication::restoreOverrideCursor();
      page->isEmpty = false;
      if (errorMsg.isEmpty() != true) {
	KMessageBox::detailedError(parentWidget,
				   i18n("<qt><strong>File corruption!</strong> KDVI had trouble interpreting your DVI file. Most "
					"likely this means that the DVI file is broken.</qt>"),
				   errorMsg, i18n("DVI File Error"));
	errorMsg = QString::null;
	currentlyDrawnPage = 0;
	return;
      }
    } else {
      foreGroundPaint.begin( &(currentlyDrawnPixmap) );
      errorMsg = QString::null;
      foreGroundPaint.drawRect(0, 0, currentlyDrawnPixmap.width(), currentlyDrawnPixmap.height());
      foreGroundPaint.end();
    }
    
    // Tell the user (once) if the DVI file contains source specials
    // ... wo don't want our great feature to go unnoticed. In
    // principle, we should use a KMessagebox here, but we want to add
    // a button "Explain in more detail..." which opens the
    // Helpcenter. Thus, we practically re-implement the KMessagebox
    // here. Most of the code is stolen from there.
    if ((dviFile->sourceSpecialMarker == true) && (currentlyDrawnPage->sourceHyperLinkList.size() > 0)) {
      dviFile->sourceSpecialMarker = false;
      // Check if the 'Don't show again' feature was used
      KConfig *config = kapp->config();
      KConfigGroupSaver saver( config, "Notification Messages" );
      bool showMsg = config->readBoolEntry( "KDVI-info_on_source_specials", true);
      
      if (showMsg) {
	KDialogBase *dialog= new KDialogBase(i18n("KDVI: Information"), KDialogBase::Yes, KDialogBase::Yes, KDialogBase::Yes,
					     parentWidget, "information", true, true,KStdGuiItem::ok() );
	
	QVBox *topcontents = new QVBox (dialog);
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
	dialog->setHelpLinkText(i18n("Explain in more detail..."));
	dialog->setHelp("inverse-search", "kdvi");
	dialog->enableLinkedHelp(true);
	dialog->setMainWidget(topcontents);
	dialog->enableButtonSeparator(false);
	dialog->incInitialSize( extraSize );
	dialog->exec();
	delete dialog;
	
	showMsg = !checkbox->isChecked();
	if (!showMsg) {
	  KConfigGroupSaver saver( config, "Notification Messages" );
	  config->writeEntry( "KDVI-info_on_source_specials", showMsg);
	}
	config->sync();
      }
    }
  }
  
  page->setPixmap(currentlyDrawnPixmap);
  currentlyDrawnPage = 0;
}


void dviWindow::embedPostScript(void)
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow::embedPostScript()" << endl;
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
      command_pointer = dviFile->dvi_Data + dviFile->page_offset[current_page];
      end_pointer     = dviFile->dvi_Data + dviFile->page_offset[current_page+1];
    } else
      command_pointer = end_pointer = 0;

    memset((char *) &currinf.data, 0, sizeof(currinf.data));
    currinf.fonttable = &(dviFile->tn_table);
    currinf._virtual  = NULL;
    prescan(&dviWindow::prescan_embedPS);
  }

  delete embedPS_progress;

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
  kdDebug(4300) << "Time elapsed till prescan phase starts " << performanceTimer.elapsed() << "ms" << endl;
  QTime preScanTimer;
  preScanTimer.start();
#endif
  dviFile->numberOfExternalPSFiles = 0;
  for(current_page=0; current_page < dviFile->total_pages; current_page++) {
    PostScriptOutPutString = new QString();

    if (current_page < dviFile->total_pages) {
      command_pointer = dviFile->dvi_Data + dviFile->page_offset[current_page];
      end_pointer     = dviFile->dvi_Data + dviFile->page_offset[current_page+1];
    } else
      command_pointer = end_pointer = 0;

    memset((char *) &currinf.data, 0, sizeof(currinf.data));
    currinf.fonttable = &(dviFile->tn_table);
    currinf._virtual  = NULL;
    prescan(&dviWindow::prescan_parseSpecials);

    if (!PostScriptOutPutString->isEmpty())
      PS_interface->setPostScript(current_page, *PostScriptOutPutString);
    delete PostScriptOutPutString;
  }
  PostScriptOutPutString = NULL;
  emit(prescanDone());
  dviFile->prescan_is_performed = true;

#ifdef PERFORMANCE_MEASUREMENT
  kdDebug(4300) << "Time required for prescan phase: " << preScanTimer.restart() << "ms" << endl;
#endif
  current_page = currPageSav;
}



bool dviWindow::correctDVI(const QString &filename)
{
  QFile f(filename);
  if (!f.open(IO_ReadOnly))
    return FALSE;

  unsigned char test[4];
  if ( f.readBlock( (char *)test,2)<2 || test[0] != 247 || test[1] != 2 )
    return FALSE;

  int n = f.size();
  if ( n < 134 )	// Too short for a dvi file
    return FALSE;
  f.at( n-4 );

  unsigned char trailer[4] = { 0xdf,0xdf,0xdf,0xdf };

  if ( f.readBlock( (char *)test, 4 )<4 || strncmp( (char *)test, (char *) trailer, 4 ) )
    return FALSE;
  // We suppose now that the dvi file is complete and OK
  return TRUE;
}


void dviWindow::changePageSize()
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow::changePageSize()" << endl;
#endif

  if ( currentlyDrawnPixmap.paintingActive() )
    return;

  unsigned int page_width_in_pixel = (unsigned int)(_zoom*paper_width_in_cm/2.54 * xres + 0.5);
  unsigned int page_height_in_pixel = (unsigned int)(_zoom*paper_height_in_cm/2.54 * xres + 0.5);

  currentlyDrawnPixmap.resize( page_width_in_pixel, page_height_in_pixel );
  currentlyDrawnPixmap.fill( white );

  PS_interface->setSize( xres*_zoom, page_width_in_pixel, page_height_in_pixel );
  emit(needsRepainting());
}


bool dviWindow::setFile(const QString &fname, const QString &ref, bool sourceMarker)
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow::setFile( fname='" << fname << "', ref='" << ref << "', sourceMarker=" << sourceMarker << " )" << endl;
#endif

  reference              = QString::null;

  QFileInfo fi(fname);
  QString   filename = fi.absFilePath();

  // If fname is the empty string, then this means: "close". Delete
  // the dvifile and the pixmap.
  if (fname.isEmpty()) {
    // Delete DVI file
    info->setDVIData(0);
    delete dviFile;
    dviFile = 0;
    
    currentlyDrawnPixmap.resize(0,0);
    if (currentlyDrawnPage != 0)
      currentlyDrawnPage->setPixmap(currentlyDrawnPixmap);
    emit(prescanDone()); // ####
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
  QString mimetype( KMimeMagic::self()->findFileType( fname )->mimeType() );
  if (mimetype != "application/x-dvi") {
    KMessageBox::sorry( parentWidget,
			i18n( "<qt>Could not open file <nobr><strong>%1</strong></nobr> which has "
			      "type <strong>%2</strong>. KDVI can only load DVI (.dvi) files.</qt>" )
			.arg( fname )
			.arg( mimetype ) );
    return false;
  }

  QApplication::setOverrideCursor( waitCursor );
  dvifile *dviFile_new = new dvifile(filename, &font_pool, sourceMarker);
  if ((dviFile_new->dvi_Data == NULL)||(dviFile_new->errorMsg.isEmpty() != true)) {
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
  info->setDVIData(dviFile);

  font_pool.setExtraSearchPath( fi.dirPath(true) );
  font_pool.setCMperDVIunit( dviFile->getCmPerDVIunit() );

  // Extract PostScript from the DVI file, and store the PostScript
  // specials in PostScriptDirectory, and the headers in the
  // PostScriptHeaderString.
  PS_interface->clear();

  // Files that reside under "tmp" or under the "data" resource are most
  // likely remote files. We limit the files they are able to read to
  // the directory they are in in order to limit the possibilities of a
  // denial of service attack.
  bool restrictIncludePath = true;
  QString tmp = KGlobal::dirs()->saveLocation("tmp", QString::null);
  if (!filename.startsWith(tmp)) {
    tmp = KGlobal::dirs()->saveLocation("data", QString::null);
    if (!filename.startsWith(tmp))
      restrictIncludePath = false;
  }

  QString includePath;
  if (restrictIncludePath) {
    includePath = filename;
    includePath.truncate(includePath.findRev('/'));
  }

  PS_interface->setIncludePath(includePath);

  // We will also generate a list of hyperlink-anchors and source-file
  // anchors in the document. So declare the existing lists empty.
  anchorList.clear();
  sourceHyperLinkAnchors.clear();

  if (dviFile->page_offset == 0)
    return false;

  // If we are re-loading a document, e.g. because the user TeXed his
  // document anew, then all fonts required for this document are
  // probably already there, and we should pre-scan the document now
  // (to extract embedded, PostScript, Hyperlinks, ets). Otherwise,
  // the pre-scan will be carried out in the method 'all_fonts_loaded'
  // after fonts have been loaded, and the fontPool emits the signal
  // fonts_have_been_loaded().
  if (font_pool.check_if_fonts_filenames_are_looked_up() == true) {
    if (dviFile->prescan_is_performed == false) {
      // Prescan phase starts here
#ifdef PERFORMANCE_MEASUREMENT
      kdDebug(4300) << "Time elapsed till prescan phase starts " << performanceTimer.elapsed() << "ms" << endl;
      QTime preScanTimer;
      preScanTimer.start();
#endif
      dviFile->numberOfExternalPSFiles = 0;
      Q_UINT16 currPageSav = current_page;

      for(current_page=0; current_page < dviFile->total_pages; current_page++) {
	PostScriptOutPutString = new QString();

	if (current_page < dviFile->total_pages) {
	  command_pointer = dviFile->dvi_Data + dviFile->page_offset[current_page];
	  end_pointer     = dviFile->dvi_Data + dviFile->page_offset[current_page+1];
	} else
	  command_pointer = end_pointer = 0;

	memset((char *) &currinf.data, 0, sizeof(currinf.data));
	currinf.fonttable = &(dviFile->tn_table);
	currinf._virtual  = NULL;
	prescan(&dviWindow::prescan_parseSpecials);

	if (!PostScriptOutPutString->isEmpty())
	  PS_interface->setPostScript(current_page, *PostScriptOutPutString);
	delete PostScriptOutPutString;
      }
      PostScriptOutPutString = NULL;
      emit(prescanDone());
      dviFile->prescan_is_performed = true;

#ifdef PERFORMANCE_MEASUREMENT
      kdDebug(4300) << "Time required for prescan phase: " << preScanTimer.restart() << "ms" << endl;
#endif
      current_page = currPageSav;
    }
    emit(needsRepainting());
    if (dviFile->suggestedPageSize != 0)
      emit( documentSpecifiedPageSize(*(dviFile->suggestedPageSize)) );
  }
  
  QApplication::restoreOverrideCursor();
  reference              = ref;
  return true;
}


void dviWindow::all_fonts_loaded(fontPool *)
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow::all_fonts_loaded(...) called" << endl;
#endif
  
  if (!dviFile)
    return;
  
  if (dviFile->prescan_is_performed == false) {
    // Prescan phase starts here
#ifdef PERFORMANCE_MEASUREMENT
    kdDebug(4300) << "Time elapsed till prescan phase starts " << performanceTimer.elapsed() << "ms" << endl;
    QTime preScanTimer;
    preScanTimer.start();
#endif
    dviFile->numberOfExternalPSFiles = 0;
    Q_UINT16 currPageSav = current_page;

    for(current_page=0; current_page < dviFile->total_pages; current_page++) {
      PostScriptOutPutString = new QString();

      if (current_page < dviFile->total_pages) {
	command_pointer = dviFile->dvi_Data + dviFile->page_offset[current_page];
	end_pointer     = dviFile->dvi_Data + dviFile->page_offset[current_page+1];
      } else
	command_pointer = end_pointer = 0;

      memset((char *) &currinf.data, 0, sizeof(currinf.data));
      currinf.fonttable = &(dviFile->tn_table);
      currinf._virtual  = NULL;
      prescan(&dviWindow::prescan_parseSpecials);

      if (!PostScriptOutPutString->isEmpty())
	PS_interface->setPostScript(current_page, *PostScriptOutPutString);
      delete PostScriptOutPutString;
    }
    PostScriptOutPutString = NULL;
    emit(prescanDone());
    dviFile->prescan_is_performed = true;

#ifdef PERFORMANCE_MEASUREMENT
    kdDebug(4300) << "Time required for prescan phase: " << preScanTimer.restart() << "ms" << endl;
#endif
    current_page = currPageSav;
  }

  // If the document specifies a paper size, we emit
  // 'documentSpecifiedPageSize' here. Note that emitting
  // 'documentSpecifiedPageSize' may or may not lead to a call of
  // 'drawPage'; that depends on the question if the widget size
  // really changes or not. To be on the safe side, we emit
  // emit(needsRepainting()) independently. For documents that have a
  // specified paper format which is NOT the default format, that
  // means that the document will be drawn twice.
  if (dviFile->suggestedPageSize != 0)
    emit( documentSpecifiedPageSize(*(dviFile->suggestedPageSize)) );
  //@@@@  emit(needsRepainting());


  // case 1: The reference is a number, which we'll interpret as a
  // page number.
  bool ok;
  int page = reference.toInt ( &ok );
  if (ok == true) {
    page--;
    if (page < 0)
      page = 0;
    if (page >= dviFile->total_pages)
      page = dviFile->total_pages-1;
    emit(request_goto_page(page, -1000));
    reference = QString::null;
    return;
  }

  // case 2: The reference is of form "src:1111Filename", where "1111"
  // points to line number 1111 in the file "Filename". KDVI then
  // looks for source specials of the form "src:xxxxFilename", and
  // tries to find the special with the biggest xxxx
  if (reference.find("src:",0,false) == 0) {
    QString ref = reference.mid(4);
    // Extract the file name and the numeral part from the reference string
    Q_UINT32 i;
    for(i=0;i<ref.length();i++)
      if (!ref.at(i).isNumber())
	break;
    Q_UINT32 refLineNumber = ref.left(i).toUInt();

    QFileInfo fi1(dviFile->filename);
    QString  refFileName   = QFileInfo(fi1.dir(), ref.mid(i).stripWhiteSpace()).absFilePath();

    if (sourceHyperLinkAnchors.isEmpty()) {
      KMessageBox::sorry(parentWidget, i18n("<qt>You have asked KDVI to locate the place in the DVI file which corresponds to "
				    "line %1 in the TeX-file <strong>%2</strong>. It seems, however, that the DVI file "
				    "does not contain the necessary source file information. "
				    "We refer to the manual of KDVI for a detailed explanation on how to include this "
				    "information. Press the F1 key to open the manual.</qt>").arg(ref.left(i)).arg(refFileName),
			 i18n("Could Not Find Reference"));
      return;
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
      if (refFileName.stripWhiteSpace() == it->fileName.stripWhiteSpace()) {
	anchorForRefFileFound = true;

	if ( (it->line <= refLineNumber) &&
	     ( (bestMatch == sourceHyperLinkAnchors.end()) || (it->line > bestMatch->line) ) )
	  bestMatch = it;
      }

    reference = QString::null;

    if (bestMatch != sourceHyperLinkAnchors.end())
      emit(request_goto_page(bestMatch->page, (Q_INT32)(bestMatch->vertical_coordinate/shrinkfactor+0.5)));
    else
      if (anchorForRefFileFound == false)
	KMessageBox::sorry(parentWidget, i18n("<qt>KDVI was not able to locate the place in the DVI file which corresponds to "
				      "line %1 in the TeX-file <strong>%2</strong>.</qt>").arg(ref.left(i)).arg(refFileName),
			   i18n( "Could Not Find Reference" ));
      else
	emit(request_goto_page(0, 0));

    return;
  }
  reference = QString::null;
}


int dviWindow::totalPages()
{
  if (dviFile != NULL)
    return dviFile->total_pages;
  else
    return 0;
}


double dviWindow::setZoom(double zoom)
{
  // In principle, this method should never be called with illegal
  // values. In principle.
  if (zoom < ZoomLimits::MinZoom/1000.0)
    zoom = ZoomLimits::MinZoom/1000.0;
  if (zoom > ZoomLimits::MaxZoom/1000.0)
    zoom = ZoomLimits::MaxZoom/1000.0;

  shrinkfactor = MFResolutions[font_pool.getMetafontMode()]/(xres*zoom);
  _zoom        = zoom;

  font_pool.setDisplayResolution( xres*zoom );
  changePageSize();
  return _zoom;
}


void dviWindow::clearStatusBar(void)
{
  emit setStatusBarText( QString::null );
}


void dviWindow::handleLocalLink(const QString &linkText)
{	  
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "hit: local link to " << linkText << endl;
#endif
  QString locallink;
  if (linkText[0] == '#' )
    locallink = linkText.mid(1); // Drop the '#' at the beginning
  else
    locallink = linkText;
  QMap<QString,DVI_Anchor>::Iterator it = anchorList.find(locallink);
  if (it != anchorList.end()) {
#ifdef DEBUG_SPECIAL
    kdDebug(4300) << "hit: local link to  y=" << AnchorList_Vert[j] << endl;
    kdDebug(4300) << "hit: local link to sf=" << shrinkfactor << endl;
#endif
    emit(request_goto_page(it.data().page, (int)(it.data().vertical_coordinate/shrinkfactor)));
  } else {
    if (linkText[0] != '#' ) {
#ifdef DEBUG_SPECIAL
      kdDebug(4300) << "hit: external link to " << currentlyDrawnPage->hyperLinkList[i].linkText << endl;
#endif
      // We could in principle use KIO::Netaccess::run() here, but
      // it is perhaps not a very good idea to allow a DVI-file to
      // specify arbitrary commands, such as "rm -rvf /". Using
      // the kfmclient seems to be MUCH safer.
      QUrl DVI_Url(dviFile->filename);
      QUrl Link_Url(DVI_Url, linkText, TRUE );
      
      QStringList args;
      args << "openURL";
      args << Link_Url.toString();
      kapp->kdeinitExec("kfmclient", args);
    }
  }
}


void dviWindow::handleSRCLink(const QString &linkText, QMouseEvent *e, documentWidget *win)
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "Source hyperlink to " << currentlyDrawnPage->sourceHyperLinkList[i].linkText << endl;
#endif
  
  QString cp = linkText;
  int max = cp.length();
  int i;
  for(i=0; i<max; i++)
    if (cp[i].isDigit() == false)
      break;
  
  // The macro-package srcltx gives a special like "src:99 test.tex"
  // while MikTeX gives "src:99test.tex". KDVI tries
  // to understand both.
  QFileInfo fi1(dviFile->filename);
  QFileInfo fi2(fi1.dir(),cp.mid(i+1));

  //Sometimes the filename is passed without the .tex extension,
  //better add it when necessary.
  if ( !fi2.exists() )
    fi2.setFile(fi2.absFilePath() + ".tex");

  QString TeXfile;
  if ( fi2.exists() )
    TeXfile = fi2.absFilePath();
  else {
    QFileInfo fi3(fi1.dir(),cp.mid(i));
    TeXfile = fi3.absFilePath();
    if ( !fi3.exists() ) {
      KMessageBox::sorry(parentWidget, QString("<qt>") +
			 i18n("The DVI-file refers to the TeX-file "
			      "<strong>%1</strong> which could not be found.").arg(KShellProcess::quote(TeXfile)) +
			 QString("</qt>"),
			 i18n( "Could Not Find File" ));
      return;
    }
  }
  
  QString command = editorCommand;
  if (command.isEmpty() == true) {
    int r = KMessageBox::warningContinueCancel(parentWidget, QString("<qt>") +
					       i18n("You have not yet specified an editor for inverse search. "
						    "Please choose your favorite editor in the "
						    "<strong>DVI options dialog</strong> "
						    "which you will find in the <strong>Settings</strong>-menu.") +
					       QString("</qt>"),
					       i18n("Need to Specify Editor"),
					       i18n("Use KDE's Editor Kate for Now"));
    if (r == KMessageBox::Continue)
      command = "kate %f";
    else
      return;
  }
  command = command.replace( QRegExp("%l"), cp.left(i) ).replace( QRegExp("%f"), KShellProcess::quote(TeXfile) );
  
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "Calling program: " << command << endl;
#endif
  
  // There may still be another program running. Since we don't
  // want to mix the output of several programs, we will
  // henceforth dimiss the output of the older programm. "If it
  // hasn't failed until now, we don't care."
  if (proc != 0) {
    qApp->disconnect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)), 0, 0);
    qApp->disconnect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)), 0, 0);
    proc = 0;
  }
  
  // Set up a shell process with the editor command.
  proc = new KShellProcess();
  if (proc == 0) {
    kdError(4300) << "Could not allocate ShellProcess for the editor command." << endl;
    return;
  }
  qApp->connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)), this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)), this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(processExited(KProcess *)), this, SLOT(editorCommand_terminated(KProcess *)));
  // Merge the editor-specific editor message here.
  export_errorString = i18n("<qt>The external program<br><br><tt><strong>%1</strong></tt><br/><br/>which was used to call the editor "
			    "for inverse search, reported an error. You might wish to look at the <strong>document info "
			    "dialog</strong> which you will find in the File-Menu for a precise error report. The "
			    "manual for KDVI contains a detailed explanation how to set up your editor for use with KDVI, "
			    "and a list of common problems.</qt>").arg(command);
  
  info->clear(i18n("Starting the editor..."));
  
  int flashOffset      = e->y(); // Heuristic correction. Looks better.
  win->flash(flashOffset);
  
  
  proc->clearArguments();
  *proc << command;
  proc->closeStdin();
  if (proc->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false) {
    kdError(4300) << "Editor failed to start" << endl;
    return;
  }
}


#include "dviwin.moc"
