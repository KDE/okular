
//
// Class: dviWindow
//
// Previewer for TeX DVI files.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#include <qbitmap.h> 
#include <qfileinfo.h>
#include <qimage.h>
#include <qkeycode.h>
#include <qlabel.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qurl.h>

#include <kapp.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <klocale.h>
#include <kprocess.h>

#include "dviwin.h"
#include "fontpool.h"
#include "fontprogress.h"
#include "infodialog.h"
#include "optiondialog.h"


//------ some definitions from xdvi ----------


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#define	MAXDIM		32767
#define    DVI_BUFFER_LEN  512     
extern struct frame        *current_frame;     
struct WindowRec mane	= {(Window) 0, 3, 0, 0, 0, 0, MAXDIM, 0, MAXDIM, 0};
struct WindowRec currwin = {(Window) 0, 3, 0, 0, 0, 0, MAXDIM, 0, MAXDIM, 0};
extern	struct WindowRec alt;
extern unsigned char       dvi_buffer[DVI_BUFFER_LEN];  
struct drawinf	currinf;


const char *dvi_oops_msg;	/* error message */
double	dimconv;
extern struct frame        frame0; /* dummy head of list */  

jmp_buf	dvi_env;	/* mechanism to communicate dvi file errors */


QIntDict<font> tn_table;


int	_pixels_per_inch; //@@@
_Xconst char	*_paper;



// The following are really used
long	        magnification;
unsigned int	page_w;
unsigned int	page_h;
// end of "really used"

extern unsigned int	page_w, page_h;
Window                  mainwin;

void 	draw_page(void);

#include <setjmp.h>
extern	jmp_buf	dvi_env;	/* mechanism to communicate dvi file errors */
double xres;
QPainter foreGroundPaint; // QPainter used for text


//------ now comes the dviWindow class implementation ----------

dviWindow::dviWindow(double zoom, int mkpk, QWidget *parent, const char *name ) 
  : QWidget( parent, name )
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "dviWindow" << endl;
#endif

  setBackgroundMode(NoBackground);

  setFocusPolicy(QWidget::StrongFocus);
  setFocus();
  
  // initialize the dvi machinery

  dviFile                = 0;

  font_pool              = new fontPool();
  if (font_pool == NULL) {
    kdError(4300) << "Could not allocate memory for the font pool." << endl;
    exit(-1);
  }
  qApp->connect(font_pool, SIGNAL(fonts_have_been_loaded()), this, SLOT(drawPage()));

  info                   = new infoDialog(this);
  if (info == 0) {
    // The info dialog is not vital. Therefore we don't abort if
    // something goes wrong here.
    kdError(4300) << "Could not allocate memory for the info dialog." << endl;
  } else {
    qApp->connect(font_pool, SIGNAL(MFOutput(QString)), info, SLOT(outputReceiver(QString)));
    qApp->connect(font_pool, SIGNAL(fonts_info(class fontPool *)), info, SLOT(setFontInfo(class fontPool *)));
    qApp->connect(font_pool, SIGNAL(new_kpsewhich_run(QString)), info, SLOT(clear(QString)));
  }


  setMakePK( mkpk );
  setMetafontMode( DefaultMFMode ); // that also sets the basedpi
  unshrunk_paper_w       = int( 21.0 * basedpi/2.54 + 0.5 ); // set A4 paper as default
  unshrunk_paper_h       = int( 27.9 * basedpi/2.54 + 0.5 ); 
  PostScriptOutPutString = NULL;
  HTML_href              = NULL;
  mainwin                = handle();
  mane                   = currwin;
  _postscript            = 0;
  pixmap                 = 0;

  // Storage used for dvips and friends.
  proc                   = 0;
  progress               = 0;

  // Calculate the horizontal resolution of the display device.  @@@
  // We assume implicitly that the horizontal and vertical resolutions
  // agree. This is probably not a safe assumption.
  Display *DISP          = x11Display();
  xres                   = ((double)(DisplayWidth(DISP,(int)DefaultScreen(DISP)) *25.4) /
			    DisplayWidthMM(DISP,(int)DefaultScreen(DISP)) );
  // Just to make sure that we are never dividing by zero.
  if ((xres < 10)||(xres > 1000))
    xres = 75.0;

  // In principle, this method should never be called with illegal
  // values for zoom. In principle.
  if (zoom < KViewPart::minZoom/1000.0)
    zoom = KViewPart::minZoom/1000.0;
  if (zoom > KViewPart::maxZoom/1000.0)
    zoom = KViewPart::maxZoom/1000.0;
  mane.shrinkfactor      = currwin.shrinkfactor = (double)basedpi/(xres*zoom);
  _zoom                  = zoom;

  PS_interface           = new ghostscript_interface(0.0, 0, 0);
  is_current_page_drawn  = 0;  
  resize(0,0);
  //@@@  setMouseTracking(true);
}

dviWindow::~dviWindow()
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "~dviWindow" << endl;
#endif

  if (info)
    delete info;

  delete PS_interface;

  if (dviFile)
    delete dviFile;
}

void dviWindow::showInfo(void)
{
  if (info == 0) 
    return;

  info->setDVIData(dviFile);
  // Call check_if_fonts_are_loaded() to make sure that the fonts_info
  // is emitted. That way, the infoDialog will know about the fonts
  // and their status.
  font_pool->check_if_fonts_are_loaded();
  info->show();
}


void dviWindow::exportPDF(void)
{
  // Should not happen since the progressDialog is modal... but who
  // knows?
  if (proc != 0) {
    KMessageBox::sorry(0, i18n("Another export command is currently running"));
    return;
  }

  // That sould also not happen.
  if (dviFile == NULL)
    return;

  QString fileName = KFileDialog::getSaveFileName(QString::null, "*.pdf|Portable Document Format (*.pdf)", this, i18n("Export File As"));
  if (fileName.isEmpty())
    return;
  QFileInfo finfo(fileName);
  if (finfo.exists()) {
    int r = KMessageBox::warningYesNo (this, QString(i18n("The file %1\nexists. Shall I overwrite that file?")).arg(fileName), 
				       i18n("Overwrite file"));
    if (r == KMessageBox::No)
      return;
  }
  
  // Initialize the progress dialog
  progress = new fontProgressDialog( QString::null, 
				     i18n("Using dvipdfm to export the file to PDF"), 
				     QString::null, 
				     i18n("KDVI is currently using the external program 'dvipdfm' to "
					  "convert your DVI-file to PDF. Sometimes that can take "
					  "a while because dvipdfm needs to generate it's own bitmap fonts "
					  "Please be patient."),
				     i18n("Waiting for dvipdf to finish..."),
				     this, "dvipdf progress dialog", false );
  if (progress != 0) {
    progress->TextLabel2->setText( i18n("Please be patient") );
    progress->setTotalSteps( dviFile->total_pages );
    qApp->connect(progress, SIGNAL(finished(void)), this, SLOT(abortExternalProgramm(void)));
  }

  proc = new KShellProcess();
  if (proc == 0) {
    kdError(4300) << "Could not allocate ShellProcess for the dvipdfm command." << endl;
    return;
  }
  
  qApp->connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)), this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)), this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(processExited(KProcess *)), this, SLOT(dvips_terminated(KProcess *)));

  if (info)
    info->clear(QString(i18n("Export: %1 to PDF")).arg(KShellProcess::quote(dviFile->filename)));

  proc->clearArguments();
  finfo.setFile(dviFile->filename);
  *proc << QString("cd %1; dvipdfm").arg(KShellProcess::quote(finfo.dirPath(true)));
  *proc << QString("-o %1").arg(KShellProcess::quote(fileName));
  *proc << KShellProcess::quote(dviFile->filename);
  proc->closeStdin();
  if (proc->start(KProcess::NotifyOnExit, KProcess::AllOutput) == false) {
    kdError(4300) << "dvipdfm failed to start" << endl;
    return;
  }
  return;
}


void dviWindow::exportPS(void)
{
  // Should not happen since the progressDialog is modal... but who
  // knows?
  if (proc != 0) {
    KMessageBox::sorry(0, i18n("Another export command is currently running"));
    return;
  }

  // That sould also not happen.
  if (dviFile == NULL)
    return;

  QString fileName = KFileDialog::getSaveFileName(QString::null, "*.ps|PostScript (*.ps)", this, i18n("Export File As"));
  if (fileName.isEmpty())
    return;
  QFileInfo finfo(fileName);
  if (finfo.exists()) {
    int r = KMessageBox::warningYesNo (this, QString(i18n("The file %1\nexists. Shall I overwrite that file?")).arg(fileName), 
				       i18n("Overwrite file"));
    if (r == KMessageBox::No)
      return;
  }
  
  // Initialize the progress dialog
  progress = new fontProgressDialog( QString::null, 
				     i18n("Using dvips to export the file to PostScript"), 
				     QString::null, 
				     i18n("KDVI is currently using the external program 'dvips' to "
					  "convert your DVI-file to PostScript. Sometimes that can take "
					  "a while because dvips needs to generate it's own bitmap fonts "
					  "Please be patient."),
				     i18n("Waiting for dvips to finish..."),
				     this, "dvips progress dialog", false );
  if (progress != 0) {
    progress->TextLabel2->setText( i18n("Please be patient") );
    progress->setTotalSteps( dviFile->total_pages );
    qApp->connect(progress, SIGNAL(finished(void)), this, SLOT(abortExternalProgramm(void)));
  }

  proc = new KShellProcess();
  if (proc == 0) {
    kdError(4300) << "Could not allocate ShellProcess for the dvips command." << endl;
    return;
  }
  
  qApp->connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)),
		this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(processExited(KProcess *)),
		this, SLOT(dvips_terminated(KProcess *)));

  if (info)
    info->clear(QString(i18n("Export: %1 to PostScript")).arg(KShellProcess::quote(dviFile->filename)));

  proc->clearArguments();
  finfo.setFile(dviFile->filename);
  *proc << QString("cd %1; dvips").arg(KShellProcess::quote(finfo.dirPath(true)));
  *proc << QString("%1").arg(KShellProcess::quote(dviFile->filename));
  *proc << QString("-o %1").arg(KShellProcess::quote(fileName));
  proc->closeStdin();
  if (proc->start(KProcess::NotifyOnExit, KProcess::Stderr) == false) {
    kdError(4300) << "dvips failed to start" << endl;
    return;
  }
  return;
}


void dviWindow::abortExternalProgramm(void)
{
  if (proc != 0) {
    delete proc; // Deleting the KProcess kills the child.
    proc = 0;
  }

  if (progress != 0) {
    progress->hideDialog();
    delete progress;
    progress = 0;
  }
}

void dviWindow::dvips_output_receiver(KProcess *, char *buffer, int buflen)
{
  // Paranoia.
  if (buflen < 0)
    return;
  QString op = QString::fromLocal8Bit(buffer, buflen);

  if (info != 0)
    info->outputReceiver(op);
  if (progress != 0)
    progress->show();
}

void dviWindow::dvips_terminated(KProcess *)
{
  if ((proc->normalExit() == true) && (proc->exitStatus() != 0)) 
    KMessageBox::error( this,
			i18n("The external program used to export the DVI-file\n"
			     "reported an error. You might wish to look at the\n"
			     "document info dialog for a precise error report.") );
   abortExternalProgramm();
}


void dviWindow::setShowPS( int flag )
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "setShowPS" << endl;
#endif

  if ( _postscript == flag )
    return;
  _postscript = flag;
  drawPage();
}

void dviWindow::setShowHyperLinks( int flag )
{
  if ( _showHyperLinks == flag )
    return;
  _showHyperLinks = flag;

  drawPage();
}

void dviWindow::setMakePK( int flag )
{
  makepk = flag;
  font_pool->setMakePK(makepk);
}

void dviWindow::setMetafontMode( unsigned int mode )
{
  if ((dviFile != NULL) && (mode != font_pool->getMetafontMode()))
    KMessageBox::sorry( this,
			i18n("The change in Metafont mode will be effective\n"
			     "only after you start kdvi again!") );
  
  MetafontMode     = font_pool->setMetafontMode(mode);
  basedpi          = MFResolutions[MetafontMode];
  _pixels_per_inch = MFResolutions[MetafontMode];  //@@@
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "basedpi " << basedpi << endl;
#endif
}


void dviWindow::setPaper(double w, double h)
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "setPaper" << endl;
#endif

  unshrunk_paper_w = int( w * basedpi/2.54 + 0.5 );
  unshrunk_paper_h = int( h * basedpi/2.54 + 0.5 ); 
  unshrunk_page_w  = unshrunk_paper_w;
  unshrunk_page_h  = unshrunk_paper_h;
  page_w           = (int)(unshrunk_page_w / mane.shrinkfactor  + 0.5) + 2;
  page_h           = (int)(unshrunk_page_h / mane.shrinkfactor  + 0.5) + 2;
  font_pool->reset_fonts();
  changePageSize();
}


//------ this function calls the dvi interpreter ----------

void dviWindow::drawPage()
{
#ifdef DEBUG_DVIWIN
  kdDebug(4300) << "drawPage" << endl;
#endif

  setCursor(arrowCursor);

  if (dviFile == NULL) {
    resize(0, 0);
    return;
  }
  if ( dviFile->file == NULL) {
    resize(0, 0);
    return;
  }

  if ( !pixmap )
    return;

  if ( !pixmap->paintingActive() ) {

    foreGroundPaint.begin( pixmap );
    QApplication::setOverrideCursor( waitCursor );
    if (setjmp(dvi_env)) {	// dvi_oops called
      QApplication::restoreOverrideCursor();
      foreGroundPaint.end();
      KMessageBox::error( this,
			  i18n("File corruption!\n\n") +
			  QString::fromUtf8(dvi_oops_msg) +
			  i18n("\n\nMost likely this means that the DVI file\nis broken, or that it is not a DVI file."));
      return;
    } else {
      draw_page();
    }
    QApplication::restoreOverrideCursor();
    foreGroundPaint.end();
  }
  resize(pixmap->width(), pixmap->height());
  repaint();
}


bool dviWindow::correctDVI(QString filename)
{
  QFile f(filename);
  if (!f.open(IO_ReadOnly))
    return FALSE;
  int n = f.size();
  if ( n < 134 )	// Too short for a dvi file
    return FALSE;
  f.at( n-4 );

  char test[4];
  unsigned char trailer[4] = { 0xdf,0xdf,0xdf,0xdf };

  if ( f.readBlock( test, 4 )<4 || strncmp( test, (char *) trailer, 4 ) )
    return FALSE;
  // We suppose now that the dvi file is complete	and OK
  return TRUE;
}


void dviWindow::changePageSize()
{
  if ( pixmap && pixmap->paintingActive() )
    return;

  int old_width = 0;
  if (pixmap) {
    old_width = pixmap->width();
    delete pixmap;
  }
  pixmap = new QPixmap( (int)page_w, (int)page_h );
  pixmap->fill( white );

  resize( page_w, page_h );
  currwin.win = mane.win = pixmap->handle();

  PS_interface->setSize( basedpi/mane.shrinkfactor , page_w, page_h);
  drawPage();
}

//------ setup the dvi interpreter (should do more here ?) ----------

bool dviWindow::setFile( const QString & fname )
{
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

    if (pixmap)
      delete pixmap;
    pixmap = 0;
    resize(0, 0);
    return true;
  }

  // Make sure the file actually exists.
  if (!fi.exists() || fi.isDir()) {
    KMessageBox::error( this,
			i18n("File error!\n\n") +
			i18n("The file does not exist\n") + 
			filename);
    return false;
  }

  QApplication::setOverrideCursor( waitCursor );
  if (setjmp(dvi_env)) {	// dvi_oops called
    QApplication::restoreOverrideCursor();
    KMessageBox::error( this,
			i18n("File corruption!\n\n") +
			QString::fromUtf8(dvi_oops_msg) +
			i18n("\n\nMost likely this means that the DVI file\n") + 
			filename +
			i18n("\nis broken, or that it is not a DVI file."));
    return false;
  }

  dvifile *dviFile_new = new dvifile(filename,font_pool);
  if (dviFile_new->file == NULL) {
    delete dviFile_new;
    return false;
  }

  if (dviFile)
    delete dviFile;
  dviFile = dviFile_new;
  if (info != 0)
    info->setDVIData(dviFile);

  page_w = (int)(unshrunk_page_w / mane.shrinkfactor  + 0.5) + 2;
  page_h = (int)(unshrunk_page_h / mane.shrinkfactor  + 0.5) + 2;

  // Extract PostScript from the DVI file, and store the PostScript
  // specials in PostScriptDirectory, and the headers in the
  // PostScriptHeaderString.
  PS_interface->clear();

  // We will also generate a list of hyperlink-anchors in the
  // document. So declare the existing list empty.
  numAnchors = 0;
  
  for(current_page=0; current_page < dviFile->total_pages; current_page++) {
    PostScriptOutPutString = new QString();
    
    (void) lseek(fileno(dviFile->file), dviFile->page_offset[current_page], SEEK_SET);
    memset((char *) &currinf.data, 0, sizeof(currinf.data));
    currinf.fonttable = tn_table;
    currinf.end       = dvi_buffer;
    currinf.pos       = dvi_buffer;
    currinf._virtual  = NULL;
    draw_part(current_frame = &frame0, dimconv);

    if (!PostScriptOutPutString->isEmpty())
      PS_interface->setPostScript(current_page, *PostScriptOutPutString);
    delete PostScriptOutPutString;
  }
  PostScriptOutPutString = NULL;
  is_current_page_drawn  = 0;

  QApplication::restoreOverrideCursor();
  return true;
}



//------ handling pages ----------


void dviWindow::gotoPage(int new_page)
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
  drawPage();
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
  if (zoom < KViewPart::minZoom/1000.0)
    zoom = KViewPart::minZoom/1000.0;
  if (zoom > KViewPart::maxZoom/1000.0)
    zoom = KViewPart::maxZoom/1000.0;

  mane.shrinkfactor = currwin.shrinkfactor = basedpi/(xres*zoom);
  _zoom             = zoom;

  page_w = (int)(unshrunk_page_w / mane.shrinkfactor  + 0.5) + 2;
  page_h = (int)(unshrunk_page_h / mane.shrinkfactor  + 0.5) + 2;

  font_pool->reset_fonts();
  changePageSize();
  return _zoom;
}


void dviWindow::paintEvent(QPaintEvent *)
{
  if (pixmap) {
    QPainter p(this);
    p.drawPixmap(QPoint(0, 0), *pixmap);
  }
}

void dviWindow::mouseMoveEvent ( QMouseEvent * e )
{
  // If no mouse button pressed
  if ( e->state() == 0 ) {
    for(int i=0; i<num_of_used_hyperlinks; i++) {
      if (hyperLinkList[i].box.contains(e->pos())) {
	setCursor(pointingHandCursor);
	return;
      }
    }
    setCursor(arrowCursor);
  }
}


void dviWindow::mousePressEvent ( QMouseEvent * e )
{
#ifdef DEBUG_SPECIAL
  kdDebug(4300) << "mouse event" << endl;
#endif

  for(int i=0; i<num_of_used_hyperlinks; i++) {
    if (hyperLinkList[i].box.contains(e->pos())) {
      if (hyperLinkList[i].linkText[0] == '#' ) {
#ifdef DEBUG_SPECIAL
	kdDebug(4300) << "hit: local link to " << hyperLinkList[i].linkText << endl;
#endif
	QString locallink = hyperLinkList[i].linkText.mid(1); // Drop the '#' at the beginning
	for(int j=0; j<numAnchors; j++) {
	  if (locallink.compare(AnchorList_String[j]) == 0) {
#ifdef DEBUG_SPECIAL
	    kdDebug(4300) << "hit: local link to  y=" << AnchorList_Vert[j] << endl;
	    kdDebug(4300) << "hit: local link to sf=" << mane.shrinkfactor << endl;
#endif
	    emit(request_goto_page(AnchorList_Page[j], (int)(AnchorList_Vert[j]/mane.shrinkfactor)));
	    break;
	  }
	}
      } else {
#ifdef DEBUG_SPECIAL
	kdDebug(4300) << "hit: external link to " << hyperLinkList[i].linkText << endl;
#endif
	QUrl DVI_Url(dviFile->filename);
	QUrl Link_Url(DVI_Url, hyperLinkList[i].linkText, TRUE );
	
	KShellProcess proc;
	proc << "kfmclient openURL " << Link_Url.toString();
	proc.start(KProcess::Block);
	//@@@ Set up a warning requester if the command failed?
      }
      break;
    }
  }
}
#include "dviwin.moc"
