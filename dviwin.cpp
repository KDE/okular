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
#include <qfileinfo.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qprogressdialog.h>
#include <qregexp.h>
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
#include <kstandarddirs.h>
#include <kstringhandler.h>

#include "dviwin.h"
#include "fontpool.h"
#include "fontprogress.h"
#include "infodialog.h"
#include "optiondialog.h"
#include "performanceMeasurement.h"
#include "xdvi.h"
#include "zoomlimits.h"


QPainter foreGroundPaint; // QPainter used for text

//------ now comes the dviWindow class implementation ----------

dviWindow::dviWindow(double zoom, QWidget *parent, const char *name )
  : QWidget( parent, name )
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow( zoom=" << zoom << ", mkpk=" << mkpk << ", parent=" << parent << ", name=" << name << " )" << endl;
#endif

  shrinkfactor = 3;

  setBackgroundMode(NoBackground);
  setFocusPolicy(QWidget::StrongFocus);
  setFocus();

  connect( &clearStatusBarTimer, SIGNAL(timeout()), this, SLOT(clearStatusBar()) );

  currentlyDrawnPage.sourceHyperLinkList.reserve(200);
  currentlyDrawnPage.textLinkList.reserve(250);
 
  // initialize the dvi machinery
  dviFile                = 0;

  font_pool              = new fontPool();
  if (font_pool == NULL) {
    kdError(4300) << "Could not allocate memory for the font pool." << endl;
    exit(-1);
  }
  connect(font_pool, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
  connect(font_pool, SIGNAL(fonts_have_been_loaded(fontPool *)), this, SLOT(all_fonts_loaded(fontPool *)));

  info                   = new infoDialog(this);
  if (info == 0) {
    // The info dialog is not vital. Therefore we don't abort if
    // something goes wrong here.
    kdError(4300) << "Could not allocate memory for the info dialog." << endl;
  } else {
    qApp->connect(font_pool, SIGNAL(MFOutput(QString)), info, SLOT(outputReceiver(QString)));
    qApp->connect(font_pool, SIGNAL(fonts_have_been_loaded(fontPool *)), info, SLOT(setFontInfo(fontPool *)));
    qApp->connect(font_pool, SIGNAL(new_kpsewhich_run(QString)), info, SLOT(clear(QString)));
  }

  editorCommand         = "";

  // Calculate the horizontal resolution of the display device.  @@@
  // We assume implicitly that the horizontal and vertical resolutions
  // agree. This is probably not a safe assumption.
  xres                   = QPaintDevice::x11AppDpiX ();
  // Just to make sure that we are never dividing by zero.
  if ((xres < 10)||(xres > 1000))
    xres = 75.0;

  // In principle, this method should never be called with illegal
  // values for zoom. In principle.
  if (zoom < ZoomLimits::MinZoom/1000.0)
    zoom = ZoomLimits::MinZoom/1000.0;
  if (zoom > ZoomLimits::MaxZoom/1000.0)
    zoom = ZoomLimits::MaxZoom/1000.0;
  _zoom                  = zoom;

  paper_width_in_cm           = 21.0; // set A4 paper as default
  paper_height_in_cm          = 27.9;

  PostScriptOutPutString = NULL;
  HTML_href              = NULL;
  _postscript            = 0;
  _showHyperLinks        = true;
  currentlyDrawnPage.pixmap                 = 0;
  findDialog             = 0;
  findNextAction         = 0;
  findPrevAction         = 0;
  DVIselection.clear();
  reference              = QString::null;
  searchText             = QString::null;

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
  is_current_page_drawn  = 0;

  // Variables used in animation.
  animationCounter = 0;
  timerIdent       = 0;

  resize(0,0);
}


dviWindow::~dviWindow()
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "~dviWindow" << endl;
#endif

  if (info)
    delete info;
  if (currentlyDrawnPage.pixmap)
    delete currentlyDrawnPage.pixmap;
  delete PS_interface;
  if (dviFile)
    delete dviFile;
  if (font_pool)
    delete font_pool;

  // Don't delete the export printer. This is owned by the
  // kdvi_multipage.
  export_printer = 0;
}


void dviWindow::showInfo(void)
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


void dviWindow::selectAll(void)
{
  QString selectedText("");
  for(unsigned int i = 0; i < currentlyDrawnPage.textLinkList.size(); i++) {
    selectedText += currentlyDrawnPage.textLinkList[i].linkText;
    selectedText += "\n";
  }
  DVIselection.set(0, currentlyDrawnPage.textLinkList.size()-1, selectedText);
  update();
}


void dviWindow::copyText(void)
{
  QApplication::clipboard()->setSelectionMode(false);
  QApplication::clipboard()->setText(DVIselection.selectedText);
}


void dviWindow::setShowPS( bool flag )
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "setShowPS" << endl;
#endif

  if ( _postscript == flag )
    return;
  _postscript = flag;
  drawPage();
}


void dviWindow::setShowHyperLinks( bool flag )
{
  if ( _showHyperLinks == flag )
    return;
  _showHyperLinks = flag;

  drawPage();
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

void dviWindow::drawPage()
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow::drawPage()" << endl;
#endif

#ifdef PERFORMANCE_MEASUREMENT
 start:
#endif

  shrinkfactor = MFResolutions[font_pool->getMetafontMode()]/(xres*_zoom);
  setCursor(arrowCursor);
  
  // Stop any animation which may be in progress
  if (timerIdent != 0) {
    killTimer(timerIdent);
    timerIdent       = 0;
    animationCounter = 0;
  }
  
  // Remove the mouse selection
  DVIselection.clear();
  
  // Stop if there is no dvi-file present
  if ( dviFile == 0 ) {
    resize(0, 0);
    return;
  }
  if ( dviFile->dvi_Data == 0 ) {
    resize(0, 0);
    return;
  }
  if ( !currentlyDrawnPage.pixmap )
    return;

  if ( !currentlyDrawnPage.pixmap->paintingActive() ) {
    // Reset colors
    colorStack.clear();
    globalColor = Qt::black;

    foreGroundPaint.begin( currentlyDrawnPage.pixmap );
    QApplication::setOverrideCursor( waitCursor );
    errorMsg = QString::null;
    draw_page();
    foreGroundPaint.drawRect(0, 0, currentlyDrawnPage.pixmap->width(), currentlyDrawnPage.pixmap->height());
    foreGroundPaint.end();
    QApplication::restoreOverrideCursor();
    if (errorMsg.isEmpty() != true) {
      KMessageBox::detailedError(this,
				 i18n("<qt><strong>File corruption!</strong> KDVI had trouble interpreting your DVI file. Most "
				      "likely this means that the DVI file is broken.</qt>"),
				 errorMsg, i18n("DVI File error"));
      return;
    }

    // Tell the user (once) if the DVI file contains source specials
    // ... wo don't want our great feature to go unnoticed. In
    // principle, we should use a KMessagebox here, but we want to add
    // a button "Explain in more detail..." which opens the
    // Helpcenter. Thus, we practically re-implement the KMessagebox
    // here. Most of the code is stolen from there.
    if ((dviFile->sourceSpecialMarker == true) && (currentlyDrawnPage.sourceHyperLinkList.size() > 0)) {
      dviFile->sourceSpecialMarker = false;
      // Check if the 'Don't show again' feature was used
      KConfig *config = kapp->config();
      KConfigGroupSaver saver( config, "Notification Messages" );
      bool showMsg = config->readBoolEntry( "KDVI-info_on_source_specials", true);

      if (showMsg) {
	KDialogBase *dialog= new KDialogBase(i18n("KDVI: Information"), KDialogBase::Yes, KDialogBase::Yes, KDialogBase::Yes,
					     this, "information", true, true, i18n("&OK"));

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

  // WORK IN PROGRESS

  /*  repaint();
  unsigned int  current_page_sav = current_page;
  qApp->processEvents();
  if (current_page_sav == current_page)
    emit contents_changed();
  */

  repaint();
  //  update();
  emit contents_changed();

#ifdef PERFORMANCE_MEASUREMENT
  if (performanceFlag == 1) {
    qApp->processEvents(30);
    current_page++;
    if (current_page < dviFile->total_pages)
      goto start;
    else {
      kdDebug(4300) << "Time required to draw all pages: " << performanceTimer.restart() << "ms" << endl;
      performanceFlag = 2;
      exit(0);
    }
  }
#endif
}


bool dviWindow::correctDVI(QString filename)
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
  if ( currentlyDrawnPage.pixmap && currentlyDrawnPage.pixmap->paintingActive() )
    return;

  if (currentlyDrawnPage.pixmap)
    delete currentlyDrawnPage.pixmap;

  unsigned int page_width_in_pixel = (unsigned int)(_zoom*paper_width_in_cm/2.54 * xres + 0.5);
  unsigned int page_height_in_pixel = (unsigned int)(_zoom*paper_height_in_cm/2.54 * xres + 0.5);

  currentlyDrawnPage.pixmap = new QPixmap( page_width_in_pixel, page_height_in_pixel );
  if (currentlyDrawnPage.pixmap == 0) {
    kdError(4300) << "dviWindow::changePageSize(), no memory for pixmap, width=" << page_width_in_pixel << ", height=" << page_height_in_pixel << endl;
    exit(0);
  }
  currentlyDrawnPage.pixmap->fill( white );

  resize( page_width_in_pixel, page_height_in_pixel );

  PS_interface->setSize( xres*_zoom, page_width_in_pixel, page_height_in_pixel );
  drawPage();
}

//------ setup the dvi interpreter (should do more here ?) ----------

bool dviWindow::setFile(QString fname, QString ref, bool sourceMarker)
{
  DVIselection.clear();
  reference              = QString::null;
  setMouseTracking(true);

  QFileInfo fi(fname);
  QString   filename = fi.absFilePath();

  // If fname is the empty string, then this means: "close". Delete
  // the dvifile and the pixmap.
  if (fname.isEmpty()) {
    // Delete DVI file
    if (info != 0)
      info->setDVIData(0);
    if (dviFile)
      delete dviFile;
    dviFile = 0;

    if (currentlyDrawnPage.pixmap)
      delete currentlyDrawnPage.pixmap;
    currentlyDrawnPage.pixmap = 0;
    resize(0, 0);
    return true;
  }

  // Make sure the file actually exists.
  if (!fi.exists() || fi.isDir()) {
    KMessageBox::error( this,
			i18n("<qt><strong>File error!</strong> The specified file '%1' does not exist. "
			     "KDVI already tried to add the ending '.dvi'</qt>").arg(filename),
			i18n("File Error!"));
    return false;
  }

  // Check if we are really loading a DVI file, and complain about the
  // mime type, if the file is not DVI. Perhaps we should move the
  // procedure later to the kviewpart, instead of the implementaton in
  // the multipage.
  QString mimetype( KMimeMagic::self()->findFileType( fname )->mimeType() );
  if (mimetype != "application/x-dvi") {
    KMessageBox::sorry( this,
			i18n( "<qt>Could not open file <nobr><strong>%1</strong></nobr> which has "
			      "type <strong>%2</strong>. KDVI can only load DVI (.dvi) files.</qt>" )
			.arg( fname )
			.arg( mimetype ) );
    return false;
  }

  QApplication::setOverrideCursor( waitCursor );
  dvifile *dviFile_new = new dvifile(filename, font_pool, sourceMarker);
  if ((dviFile_new->dvi_Data == NULL)||(dviFile_new->errorMsg.isEmpty() != true)) {
    QApplication::restoreOverrideCursor();
    if (dviFile_new->errorMsg.isEmpty() != true)
      KMessageBox::detailedError(this,
				 i18n("<qt>File corruption! KDVI had trouble interpreting your DVI file. Most "
				      "likely this means that the DVI file is broken.</qt>"),
				 dviFile_new->errorMsg, i18n("DVI File Error"));
    delete dviFile_new;
    return false;
  }


  if (dviFile)
    delete dviFile;
  dviFile = dviFile_new;
  if (info != 0)
    info->setDVIData(dviFile);

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
  if (!filename.startsWith(tmp))
  {
     tmp = KGlobal::dirs()->saveLocation("data", QString::null);
     if (!filename.startsWith(tmp))
        restrictIncludePath = false;
  }
  
  QString includePath;
  if (restrictIncludePath)
  {
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

  // Prescan phase starts here
#ifdef PERFORMANCE_MEASUREMENT
  kdDebug(4300) << "Time elapsed till prescan phase starts " << performanceTimer.elapsed() << "ms" << endl;
  QTime preScanTimer;
  preScanTimer.start();
#endif
  
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
    draw_part(65536.0*fontPixelPerDVIunit(), false);
    
    if (!PostScriptOutPutString->isEmpty())
      PS_interface->setPostScript(current_page, *PostScriptOutPutString);
    delete PostScriptOutPutString;
  }
  PostScriptOutPutString = NULL;
  is_current_page_drawn  = 0;
  
#ifdef PERFORMANCE_MEASUREMENT
  kdDebug(4300) << "Time required for prescan phase: " << preScanTimer.restart() << "ms" << endl;
#endif

  QApplication::restoreOverrideCursor();
  reference              = ref;
  return true;
}

void dviWindow::all_fonts_loaded(fontPool *)
{
  if (dviFile == 0)
    return;

  drawPage();

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
    QString  refFileName   = QFileInfo(ref.mid(i).stripWhiteSpace()).absFilePath();
    
    if (sourceHyperLinkAnchors.isEmpty()) {
      KMessageBox::sorry(this, i18n("<qt>You have asked KDVI to locate the place in the DVI file which corresponds to "
				    "line %1 in the TeX-file <strong>%2</strong>. It seems, however, that the DVI file "
				    "does not contain the necessary source file information. "
				    "We refer to the manual of KDVI for a detailed explanation on how to include this "
				    "information. Press the F1 key to open the manual.</qt>").arg(ref.left(i)).arg(refFileName),
			 i18n( "Could not Find Reference" ));
      return;
    }
    
    Q_INT32 page = 0;
    Q_INT32 y    = -1000;
    QValueVector<DVI_SourceFileAnchor>::iterator it;
    for( it = sourceHyperLinkAnchors.begin(); it != sourceHyperLinkAnchors.end(); ++it )
      if (refFileName.stripWhiteSpace() == it->fileName.stripWhiteSpace()) {
	if (refLineNumber >= it->line) {
	  page = it->page;
	  y    = (Q_INT32)(it->vertical_coordinate/shrinkfactor+0.5);
	} 
      }
    
    reference = QString::null;
    if (y >= 0)
      emit(request_goto_page(page, y));
    if (y < 0)
      KMessageBox::sorry(this, i18n("<qt>KDVI was not able to locate the place in the DVI file which corresponds to "
				    "line %1 in the TeX-file <strong>%2</strong>.</qt>").arg(ref.left(i)).arg(refFileName),
			 i18n( "Could not Find Reference" ));
    return;
  }
  reference = QString::null;
}

//------ handling pages ----------


void dviWindow::gotoPage(unsigned int new_page)
{
  if (dviFile == NULL)
    return;

  if (new_page<1)
    new_page = 1;
  if (new_page > dviFile->total_pages)
    new_page = dviFile->total_pages;
  if ((new_page-1==current_page) &&  !is_current_page_drawn)
    return;
  current_page           = new_page-1;
  is_current_page_drawn  = 0;
  animationCounter       = 0;
  drawPage();
}


void dviWindow::gotoPage(int new_page, int vflashOffset)
{
  gotoPage(new_page);
  animationCounter = 0;
  if (timerIdent != 0)
    killTimer(timerIdent);
  flashOffset      = vflashOffset - currentlyDrawnPage.pixmap->height()/100; // Heuristic correction. Looks better.
  timerIdent       = startTimer(50); // Start the animation. The animation proceeds in 1/10s intervals
}

void dviWindow::timerEvent( QTimerEvent *e )
{
  animationCounter++;
  if (animationCounter >= 10) {
    killTimer(e->timerId());
    timerIdent       = 0;
    animationCounter = 0;
  }
  repaint(0, flashOffset, currentlyDrawnPage.pixmap->width(), currentlyDrawnPage.pixmap->height()/19, false);
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

  shrinkfactor = MFResolutions[font_pool->getMetafontMode()]/(xres*zoom);
  _zoom        = zoom;

  font_pool->setDisplayResolution( xres*zoom );
  changePageSize();
  return _zoom;
}

void dviWindow::paintEvent(QPaintEvent *e)
{
  if (currentlyDrawnPage.pixmap) {
    bitBlt ( this, e->rect().topLeft(), currentlyDrawnPage.pixmap, e->rect(), CopyROP);
    QPainter p(this);
    p.setClipRect(e->rect());
    if (animationCounter > 0 && animationCounter < 10) {
      int wdt = currentlyDrawnPage.pixmap->width()/(10-animationCounter);
      int hgt = currentlyDrawnPage.pixmap->height()/((10-animationCounter)*20);
      p.setPen(QPen(QColor(150,0,0), 3, DashLine));
      p.drawRect((currentlyDrawnPage.pixmap->width()-wdt)/2, flashOffset, wdt, hgt);
    }

    // Mark selected text.
    if (DVIselection.selectedTextStart != -1)
      for(unsigned int i = DVIselection.selectedTextStart; (i <= DVIselection.selectedTextEnd)&&(i < currentlyDrawnPage.textLinkList.size()); i++) {
	p.setPen( NoPen );
	p.setBrush( white );
	p.setRasterOp( Qt::XorROP );
	p.drawRect(currentlyDrawnPage.textLinkList[i].box);
      }
  }
}

void dviWindow::clearStatusBar(void)
{
  emit setStatusBarText( QString::null );
}

void dviWindow::mouseMoveEvent ( QMouseEvent * e )
{
  // If no mouse button pressed
  if ( e->state() == 0 ) {
    // go through hyperlinks
    for(unsigned int i=0; i<currentlyDrawnPage.hyperLinkList.size(); i++) {
      if (currentlyDrawnPage.hyperLinkList[i].box.contains(e->pos())) {
	clearStatusBarTimer.stop();
	setCursor(pointingHandCursor);
	QString link = currentlyDrawnPage.hyperLinkList[i].linkText;
	if ( link.startsWith("#") )
	  link = link.remove(0,1);
	emit setStatusBarText( i18n("Link to %1").arg(link) );
	return;
      }
    }

    // Cursor not over hyperlink? Then let the cursor be the usual arrow
    setCursor(arrowCursor);

    // But maybe the cursor hovers over a sourceHyperlink?
    for(unsigned int i=0; i<currentlyDrawnPage.sourceHyperLinkList.size(); i++) {
      if (currentlyDrawnPage.sourceHyperLinkList[i].box.contains(e->pos())) {
	clearStatusBarTimer.stop();
	KStringHandler kstr;
	QString line = kstr.word(currentlyDrawnPage.sourceHyperLinkList[i].linkText, "0");
	QString file = kstr.word(currentlyDrawnPage.sourceHyperLinkList[i].linkText, "1:");
	emit setStatusBarText( i18n("Link to line %1 of %2").arg(line).arg(file) );
	return;
      }
    }
    if (!clearStatusBarTimer.isActive())
      clearStatusBarTimer.start( 200, TRUE ); // clear the statusbar after 200 msec.
    return;
  }

  // Right mouse button pressed -> Text copy function
  if ((e->state() & RightButton) != 0) {
    if (selectedRectangle.isEmpty()) {
      firstSelectedPoint = e->pos();
      selectedRectangle.setRect(e->pos().x(),e->pos().y(),1,1);
    } else {
      int lx = e->pos().x() < firstSelectedPoint.x() ? e->pos().x() : firstSelectedPoint.x();
      int rx = e->pos().x() > firstSelectedPoint.x() ? e->pos().x() : firstSelectedPoint.x();
      int ty = e->pos().y() < firstSelectedPoint.y() ? e->pos().y() : firstSelectedPoint.y();
      int by = e->pos().y() > firstSelectedPoint.y() ? e->pos().y() : firstSelectedPoint.y();
      selectedRectangle.setCoords(lx,ty,rx,by);
    }

    // Now that we know the rectangle, we have to find out which words
    // intersect it!
    Q_INT32 selectedTextStart = -1;
    Q_INT32 selectedTextEnd   = -1;

    for(unsigned int i=0; i<currentlyDrawnPage.textLinkList.size(); i++)
      if ( selectedRectangle.intersects(currentlyDrawnPage.textLinkList[i].box) ) {
	if (selectedTextStart == -1)
	  selectedTextStart = i;
	selectedTextEnd = i;
      }

    QString selectedText("");
    if (selectedTextStart != -1)
      for(unsigned int i = selectedTextStart; (i <= selectedTextEnd)&&(i < currentlyDrawnPage.textLinkList.size()); i++) {
	selectedText += currentlyDrawnPage.textLinkList[i].linkText;
	selectedText += "\n";
      }

    if ((selectedTextStart != DVIselection.selectedTextStart) || (selectedTextEnd != DVIselection.selectedTextEnd)) {
      if (selectedTextEnd == -1) {
	DVIselection.clear();
	update();
      } else {
	// Find the rectangle that needs to be updated (reduces
	// flicker)
	int a = DVIselection.selectedTextStart;
	int b = DVIselection.selectedTextEnd+1;
	int c = selectedTextStart;
	int d = selectedTextEnd+1;

	int i1 = kMin(a,c);
	int i2 = kMin(kMax(a,c),kMin(b,d));
	int i3 = kMax(kMax(a,c),kMin(b,d));
	int i4 = kMax(b,d);

	QRect box;
	int i=i1;
	while(i<i2) {
	  if (i != -1)
	    box = box.unite(currentlyDrawnPage.textLinkList[i].box);
	  i++;
	}

	for(int i=i3; i<i4; i++)
	  if (i != -1)
	    box = box.unite(currentlyDrawnPage.textLinkList[i].box);
	DVIselection.set(selectedTextStart, selectedTextEnd, selectedText);
	update(box);
      }
    }
  }
}


void dviWindow::mouseReleaseEvent ( QMouseEvent * )
{
  selectedRectangle.setRect(0,0,0,0);
}


void dviWindow::mousePressEvent ( QMouseEvent * e )
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "mouse event" << endl;
#endif

  // Check if the mouse is pressed on a regular hyperlink
  if ((e->button() == LeftButton) && (currentlyDrawnPage.hyperLinkList.size() > 0))
    for(int i=0; i<currentlyDrawnPage.hyperLinkList.size(); i++) {
      if (currentlyDrawnPage.hyperLinkList[i].box.contains(e->pos())) {
	if (currentlyDrawnPage.hyperLinkList[i].linkText[0] == '#' ) {
#ifdef DEBUG_SPECIAL
	  kdDebug(4300) << "hit: local link to " << currentlyDrawnPage.hyperLinkList[i].linkText << endl;
#endif
	  QString locallink = currentlyDrawnPage.hyperLinkList[i].linkText.mid(1); // Drop the '#' at the beginning
	  QMap<QString,DVI_Anchor>::Iterator it = anchorList.find(locallink);
	  if (it != anchorList.end()) {
#ifdef DEBUG_SPECIAL
	    kdDebug(4300) << "hit: local link to  y=" << AnchorList_Vert[j] << endl;
	    kdDebug(4300) << "hit: local link to sf=" << shrinkfactor << endl;
#endif
	    emit(request_goto_page(it.data().page, (int)(it.data().vertical_coordinate/shrinkfactor)));
	  }
	} else {
#ifdef DEBUG_SPECIAL
	  kdDebug(4300) << "hit: external link to " << currentlyDrawnPage.hyperLinkList[i].linkText << endl;
#endif
	  // We could in principle use KIO::Netaccess::run() here, but
	  // it is perhaps not a very good idea to allow a DVI-file to
	  // specify arbitrary commands, such as "rm -rvf /". Using
	  // the kfmclient seems to be MUCH safer.
	  QUrl DVI_Url(dviFile->filename);
	  QUrl Link_Url(DVI_Url, currentlyDrawnPage.hyperLinkList[i].linkText, TRUE );

          QStringList args;
          args << "openURL";
          args << Link_Url.toString();
          kapp->kdeinitExec("kfmclient", args);
	}
	break;
      }
    }

  // Check if the mouse is pressed on a source-hyperlink
  if ((e->button() == MidButton) && (currentlyDrawnPage.sourceHyperLinkList.size() > 0))
    for(unsigned int i=0; i<currentlyDrawnPage.sourceHyperLinkList.size(); i++)
      if (currentlyDrawnPage.sourceHyperLinkList[i].box.contains(e->pos())) {
#ifdef DEBUG_SPECIAL
	kdDebug(4300) << "Source hyperlink to " << currentlyDrawnPage.sourceHyperLinkList[i].linkText << endl;
#endif

	QString cp = currentlyDrawnPage.sourceHyperLinkList[i].linkText;
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
	QString TeXfile;
	if ( fi2.exists() )
	  TeXfile = fi2.absFilePath();
	else {
	  QFileInfo fi3(fi1.dir(),cp.mid(i));
	  TeXfile = fi3.absFilePath();
	  if ( !fi3.exists() ) {
	    KMessageBox::sorry(this, i18n("The DVI-file refers to the TeX-file "
					  "<strong>%1</strong> which could not be found.").arg(KShellProcess::quote(TeXfile)),
			       i18n( "Could not Find File" ));
	    return;
	  }
	}

	QString command = editorCommand;
	if (command.isEmpty() == true) {
	  int r = KMessageBox::warningContinueCancel(this, i18n("You have not yet specified an editor for inverse search. "
								"Please choose your favorite editor in the "
								"<strong>DVI options dialog</strong> "
								"which you will find in the <strong>Settings</strong>-menu."),
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

	if (info)
	  info->clear(i18n("Starting the editor..."));

	animationCounter = 0;
	flashOffset      = e->y(); // Heuristic correction. Looks better.
	timerIdent       = startTimer(50); // Start the animation. The animation proceeds in 1/10s intervals


	proc->clearArguments();
	*proc << command;
	proc->closeStdin();
	if (proc->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false) {
	  kdError(4300) << "Editor failed to start" << endl;
	  return;
	}
      }
}

#include "dviwin.moc"
