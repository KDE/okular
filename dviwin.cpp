
//
// Class: dviWindow
//
// Previewer for TeX DVI files.
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
#include <kdebug.h>
#include <keditcl.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kprinter.h>
#include <kprocess.h>



#include "dviwin.h"
#include "fontpool.h"
#include "fontprogress.h"
#include "infodialog.h"
#include "optiondialog.h"


//------ some definitions from xdvi ----------


#define	MAXDIM		32767
struct WindowRec mane	= {(Window) 0, 3, 0, 0, 0, 0, MAXDIM, 0, MAXDIM, 0};
struct WindowRec currwin = {(Window) 0, 3, 0, 0, 0, 0, MAXDIM, 0, MAXDIM, 0};
extern	struct WindowRec alt;
struct drawinf	currinf;

QIntDict<font> tn_table;

int	_pixels_per_inch; //@@@

// The following are really used
unsigned int	page_w;
unsigned int	page_h;
// end of "really used"

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
  connect(font_pool, SIGNAL( setStatusBarText( const QString& ) ), this, SIGNAL( setStatusBarText( const QString& ) ) );
  connect(font_pool, SIGNAL(fonts_have_been_loaded()), this, SLOT(all_fonts_loaded()));

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
  editorCommand         = "";
  setMetafontMode( DefaultMFMode ); // that also sets the basedpi
  paper_width           = 21.0; // set A4 paper as default
  paper_height          = 27.9;
  unshrunk_page_w       = int( 21.0 * basedpi/2.54 + 0.5 );
  unshrunk_page_h       = int( 27.9 * basedpi/2.54 + 0.5 ); 
  PostScriptOutPutString = NULL;
  HTML_href              = NULL;
  mane                   = currwin;
  _postscript            = 0;
  pixmap                 = 0;
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

  // Calculate the horizontal resolution of the display device.  @@@
  // We assume implicitly that the horizontal and vertical resolutions
  // agree. This is probably not a safe assumption.
  xres                   = QPaintDevice::x11AppDpiX ();

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

  delete PS_interface;

  if (dviFile)
    delete dviFile;

  // Don't delete the export printer. This is owned by the
  // kdvi_multipage.
  export_printer = 0;
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


void dviWindow::exportText(void)
{
  // That sould also not happen.
  if (dviFile == NULL)
    return;
  if (dviFile->dvi_Data == 0 ) 
    return;
  if (pixmap->paintingActive())
    return;

  if (KMessageBox::warningContinueCancel( this, 
					  i18n("<qt>This function exports the DVI file to a plain text. Unfortunately, this version of "
					       "KDVI treats only plain ASCII characters properly. Symbols, ligatures, mathematical "
					       "formulae, accented characters, and non-english text, such as Russian or Korean, will "
					       "most likely be messed up completely.</qt>"),
					  i18n("Function may not work as expected"),
					  i18n("Continue anyway"),
					  "warning_export_to_text_may_not_work") == KMessageBox::Cancel)
    return;

  QString fileName = KFileDialog::getSaveFileName(QString::null, "*.txt|Plain Text (Latin 1) (*.txt)", this, i18n("Export File As"));
  if (fileName.isEmpty())
    return;
  QFileInfo finfo(fileName);
  if (finfo.exists()) {
    int r = KMessageBox::warningYesNo (this, QString(i18n("The file %1\nexists. Shall I overwrite that file?")).arg(fileName), 
				       i18n("Overwrite file"));
    if (r == KMessageBox::No)
      return;
  }
  
  QFile textFile(fileName);
  textFile.open( IO_WriteOnly );
  QTextStream stream( &textFile );

  bool _postscript_sav = _postscript;
  int current_page_sav = current_page;
  _postscript = FALSE; // Switch off postscript to speed up things...
  QProgressDialog progress( i18n("Exporting to text..."), i18n("Abort"), dviFile->total_pages, this, "export_text_progress", TRUE );
  progress.setMinimumDuration(300);
  QPixmap pixie(1,1);
  for(current_page=0; current_page < dviFile->total_pages; current_page++) {
    progress.setProgress( current_page );
    // Funny. The manual to QT tells us that we need to call
    // qApp->processEvents() regularly to keep the application from
    // freezing. However, the application crashes immediately if we
    // uncomment the following line and works just fine as it is. Wild
    // guess: Could that be related to the fact that we are linking
    // agains qt-mt?

    // qApp->processEvents();

    if ( progress.wasCancelled() )
      break;
    
    foreGroundPaint.begin( &pixie );
    draw_page(); // We gracefully ingore any errors (bad dvi-file, etc.) which may occur during draw_page()
    foreGroundPaint.end();
    
    for(int i=0; i<num_of_used_textlinks; i++) 
      stream << textLinkList[i].linkText << endl;
  }

  // Switch off the progress dialog, etc.
  progress.setProgress( dviFile->total_pages );
  // Restore the PostScript setting 
  _postscript = _postscript_sav;
  
  // Restore the current page.
  current_page = current_page_sav;
  foreGroundPaint.begin( &pixie );
  draw_page();  // We gracefully ingore any errors (bad dvi-file, etc.) which may occur during draw_page()
  foreGroundPaint.end();

  return;
}


void dviWindow::exportPDF(void)
{
  // It could perhaps happen that a kShellProcess, which runs an
  // editor for inverse search, is still running. In that case, we
  // ingore any further output of the editor by detaching the
  // appropriate slots. The sigal "processExited", however, remains
  // attached to the slow "exportCommand_terminated", which is smart
  // enough to ignore the exit status of the editor if another command
  // has been called meanwhile. See also the exportPS method.
  if (proc != 0) {
    // Make sure all further output of the programm is ignored
    qApp->disconnect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)), 0, 0);
    qApp->disconnect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)), 0, 0);
    proc = 0;
  }

  // That sould also not happen.
  if (dviFile == NULL)
    return;

  // Is the dvipdfm-Programm available ??
  QStringList texList = QStringList::split(":", QString::fromLocal8Bit(getenv("PATH")));
  bool found = false;
  for (QStringList::Iterator it=texList.begin(); it!=texList.end(); ++it) {
    QString temp = (*it) + "/" + "dvipdfm";
    if (QFile::exists(temp)) {
      found = true;
      break;
    }
  }
  if (found == false) {
    KMessageBox::sorry(0, i18n("KDVI could not locate the program 'dvipdfm' on your computer. That program is\n"
			       "absolutely needed by the export function. You can, however, convert\n"
			       "the DVI-file to PDF using the print function of KDVI, but that will often\n"
			       "produce which print ok, but are of inferior quality if viewed in the \n"
			       "Acrobat Reader. It may be wise to upgrade to a more recent version of your\n"
			       "TeX distribution which includes the 'dvipdfm' program.\n\n"
			       "Hint to the perplexed system administrator: KDVI uses the shell's PATH variable\n"
			       "when looking for programs."));
    return;
  }

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
					  "a while because dvipdfm needs to generate its own bitmap fonts "
					  "Please be patient."),
				     i18n("Waiting for dvipdfm to finish..."),
				     this, "dvipdfm progress dialog", false );
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
  qApp->disconnect( this, SIGNAL(mySignal()), 0, 0 );

  qApp->connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)), this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)), this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(processExited(KProcess *)), this, SLOT(dvips_terminated(KProcess *)));

  export_errorString = i18n("<qt>The external program 'dvipdf', which was used to export the file, reported an error. "
			    "You might wish to look at the <strong>document info dialog</strong> which you will "
			    "find in the File-Menu for a precise error report.</qt>") ;


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


void dviWindow::exportPS(QString fname, QString options, KPrinter *printer)
{
  // Safety check.
  if (dviFile->page_offset == 0)
    return;

  // It could perhaps happen that a kShellProcess, which runs an
  // editor for inverse search, is still running. In that case, we
  // ingore any further output of the editor by detaching the
  // appropriate slots. The sigal "processExited", however, remains
  // attached to the slow "exportCommand_terminated", which is smart
  // enough to ignore the exit status of the editor if another command
  // has been called meanwhile. See also the exportPDF method.
  if (proc != 0) {
    qApp->disconnect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)), 0, 0);
    qApp->disconnect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)), 0, 0);
    proc = 0;
  }

  // That sould also not happen.
  if (dviFile == NULL)
    return;
  
  QString fileName;
  if (fname.isEmpty()) {
    fileName = KFileDialog::getSaveFileName(QString::null, "*.ps|PostScript (*.ps)", this, i18n("Export File As"));
    if (fileName.isEmpty())
      return;
    QFileInfo finfo(fileName);
    if (finfo.exists()) {
      int r = KMessageBox::warningYesNo (this, QString(i18n("The file %1\nexists. Shall I overwrite that file?")).arg(fileName), 
					 i18n("Overwrite file"));
      if (r == KMessageBox::No)
	return;
    }
  } else
    fileName = fname;
  export_fileName = fileName;
  export_printer  = printer;

  // Initialize the progress dialog
  progress = new fontProgressDialog( QString::null, 
				     i18n("Using dvips to export the file to PostScript"), 
				     QString::null, 
				     i18n("KDVI is currently using the external program 'dvips' to "
					  "convert your DVI-file to PostScript. Sometimes that can take "
					  "a while because dvips needs to generate its own bitmap fonts "
					  "Please be patient."),
				     i18n("Waiting for dvips to finish..."),
				     this, "dvips progress dialog", false );
  if (progress != 0) {
    progress->TextLabel2->setText( i18n("Please be patient") );
    progress->setTotalSteps( dviFile->total_pages );
    qApp->connect(progress, SIGNAL(finished(void)), this, SLOT(abortExternalProgramm(void)));
  }

  // There is a major problem with dvips, at least 5.86 and lower: the
  // arguments of the option "-pp" refer to TeX-pages, not to
  // sequentially numbered pages. For instance "-pp 7" may refer to 3
  // or more pages: one page "VII" in the table of contents, a page
  // "7" in the text body, and any number of pages "7" in various
  // appendices, indices, bibliographies, and so forth. KDVI currently
  // uses the following disgusting workaround: if the "options"
  // variable is used, the DVI-file is copied to a temporary file, and
  // all the page numbers are changed into a sequential ordering
  // (using UNIX files, and taking manually care of CPU byte
  // ordering). Finally, dvips is then called with the new file, and
  // the file is afterwards deleted. Isn't that great?

  // Sourcefile is the name of the DVI which is used by dvips, either
  // the original file, or a temporary file with a new numbering.
  QString sourceFileName = dviFile->filename;
  if (options.isEmpty() == false) {
    // Get a name for a temporary file.
    KTempFile export_tmpFile;
    export_tmpFileName = export_tmpFile.name();
    export_tmpFile.unlink();
    
    sourceFileName     = export_tmpFileName;
    if (KIO::NetAccess::copy(dviFile->filename, sourceFileName)) {
      int wordSize;
      bool bigEndian;
      qSysInfo (&wordSize, &bigEndian);
      // Proper error handling? We don't care.
      FILE *f = fopen(QFile::encodeName(sourceFileName),"r+");
      for(Q_UINT32 i=1; i<=dviFile->total_pages; i++) {
	fseek(f,dviFile->page_offset[i-1]+1, SEEK_SET);
	// Write the page number to the file, taking good care of byte
	// orderings. Hopefully QT will implement random access QFiles
	// soon.
	if (bigEndian) {
	  fwrite(&i, sizeof(Q_INT32), 1, f);
	  fwrite(&i, sizeof(Q_INT32), 1, f);
	  fwrite(&i, sizeof(Q_INT32), 1, f);
	  fwrite(&i, sizeof(Q_INT32), 1, f);
	} else {
	  Q_UINT8  anum[4];
	  Q_UINT8 *bnum = (Q_UINT8 *)&i;
	  anum[0] = bnum[3];
	  anum[1] = bnum[2];
	  anum[2] = bnum[1];
	  anum[3] = bnum[0];
	  fwrite(anum, sizeof(Q_INT32), 1, f);
	  fwrite(anum, sizeof(Q_INT32), 1, f);
	  fwrite(anum, sizeof(Q_INT32), 1, f);
	  fwrite(anum, sizeof(Q_INT32), 1, f);
	}
      }
      fclose(f);
    } else {
      KMessageBox::error(this, i18n("Failed to copy the DVI-file <strong>%1</strong> to the temporary file <strong>%2</strong>. "
				    "The export or print command is aborted.").arg(dviFile->filename).arg(sourceFileName));
      return;
    }
  }

  // Allocate and initialize the shell process.
  proc = new KShellProcess();
  if (proc == 0) {
    kdError(4300) << "Could not allocate ShellProcess for the dvips command." << endl;
    return;
  }
  
  qApp->connect(proc, SIGNAL(receivedStderr(KProcess *, char *, int)), this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(receivedStdout(KProcess *, char *, int)), this, SLOT(dvips_output_receiver(KProcess *, char *, int)));
  qApp->connect(proc, SIGNAL(processExited(KProcess *)), this, SLOT(dvips_terminated(KProcess *)));
  export_errorString = i18n("The external program 'dvips', which was used to export the file, reported an error. "
			    "You might wish to look at the <strong>document info dialog</strong> which you will "
			    "find in the File-Menu for a precise error report.") ;
  if (info)
    info->clear(QString(i18n("Export: %1 to PostScript")).arg(KShellProcess::quote(dviFile->filename)));

  proc->clearArguments();
  QFileInfo finfo(dviFile->filename);
  *proc << QString("cd %1; dvips").arg(KShellProcess::quote(finfo.dirPath(true)));
  if (printer == 0)
    *proc << "-z"; // export Hyperlinks
  if (options.isEmpty() == false)
    *proc << options;
  *proc << QString("%1").arg(KShellProcess::quote(sourceFileName));
  *proc << QString("-o %1").arg(KShellProcess::quote(fileName));
  proc->closeStdin();
  if (proc->start(KProcess::NotifyOnExit, KProcess::Stderr) == false) {
    kdError(4300) << "dvips failed to start" << endl;
    return;
  }
  return;
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

void dviWindow::dvips_terminated(KProcess *sproc)
{
  // Give an error message from the message string. However, if the
  // sproc is not the "current external process of interest", i.e. not
  // the LAST external program that was started by the user, then the
  // export_errorString, does not correspond to sproc. In that case,
  // we ingore the return status silently.
  if ((proc == sproc) && (sproc->normalExit() == true) && (sproc->exitStatus() != 0)) 
    KMessageBox::error( this, export_errorString );
  
  if (export_printer != 0)
    export_printer->printFiles( QStringList(export_fileName), true );
  // Kill and delete the remaining process, reset the printer, etc.
  abortExternalProgramm();
}

void dviWindow::editorCommand_terminated(KProcess *sproc)
{
  // Give an error message from the message string. However, if the
  // sproc is not the "current external process of interest", i.e. not
  // the LAST external program that was started by the user, then the
  // export_errorString, does not correspond to sproc. In that case,
  // we ingore the return status silently.
  if ((proc == sproc) && (sproc->normalExit() == true) && (sproc->exitStatus() != 0)) 
    KMessageBox::error( this, export_errorString );
  
  // Let's hope that this is not all too nasty... killing a
  // KShellProcess from a slot that was called from the KShellProcess
  // itself. Until now, there weren't any problems.

  // Perhaps it was a bad idea, after all.
  //@@@@  delete sproc;
}



void dviWindow::abortExternalProgramm(void)
{
  if (proc != 0) {
    delete proc; // Deleting the KProcess kills the child.
    proc = 0;
  }

  if (export_tmpFileName.isEmpty() != true) {
    unlink(QFile::encodeName(export_tmpFileName)); // That should delete the file.
    export_tmpFileName = "";
  }

  if (progress != 0) {
    progress->hideDialog();
    delete progress;
    progress = 0;
  }
  
  export_printer  = 0;
  export_fileName = "";
}

void dviWindow::selectAll(void)
{
  QString selectedText("");
  for(int i = 0; i < num_of_used_textlinks; i++) {
    selectedText += textLinkList[i].linkText;
    selectedText += "\n";
  }
  DVIselection.set(0, num_of_used_textlinks-1, selectedText);
  repaint();
}

void dviWindow::copyText(void)
{
  QApplication::clipboard()->setSelectionMode(false);
  QApplication::clipboard()->setText(DVIselection.selectedText);
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

  paper_width      = w;
  paper_height     = h;
  unshrunk_page_w  = int( w * basedpi/2.54 + 0.5 );
  unshrunk_page_h  = int( h * basedpi/2.54 + 0.5 ); 
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
  if ( !pixmap )
    return;

  if ( !pixmap->paintingActive() ) {
    foreGroundPaint.begin( pixmap );
    QApplication::setOverrideCursor( waitCursor );
    errorMsg = QString::null;
    draw_page();
    foreGroundPaint.drawRect(0,0,pixmap->width(),pixmap->height());
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
    if ((dviFile->sourceSpecialMarker == true) && (num_of_used_source_hyperlinks > 0)) {
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
#if QT_VERSION < 300
	label1->setPixmap(QMessageBox::standardIcon(QMessageBox::Information, kapp->style().guiStyle()));
#else
	label1->setPixmap(QMessageBox::standardIcon(QMessageBox::Information));
#endif
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
  repaint();
  emit contents_changed();
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

  PS_interface->setSize( basedpi/mane.shrinkfactor, page_w, page_h );
  drawPage();
}

//------ setup the dvi interpreter (should do more here ?) ----------

bool dviWindow::setFile(QString fname, QString ref, bool sourceMarker=true)
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

    if (pixmap)
      delete pixmap;
    pixmap = 0;
    resize(0, 0);
    return true;
  }

  // Make sure the file actually exists.
  if (!fi.exists() || fi.isDir()) {
    KMessageBox::error( this,
			i18n("<qt><strong>File error!</strong> The specified file '%1' does not exist. "
			     "KDVI already tried to add the ending '.dvi'</qt>").arg(filename), 
			i18n("File error!"));
    return false;
  }

  // Check if we are really loading a DVI file, and complain about the
  // mime type, if the file is not DVI. Perhaps we should move the
  // procedure later to the kviewpart, instead of the implementaton in
  // the multipage.
  QString mimetype( KMimeMagic::self()->findFileType( fname )->mimeType() );
  if (mimetype != "application/x-dvi") {
    KMessageBox::sorry( this,
			i18n( "Could not open file <nobr><strong>%1</strong></nobr> which has " 
			      "type <strong>%2</strong>. KDVI can only load DVI (.dvi) files." )
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
				 dviFile_new->errorMsg, i18n("DVI File error"));
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

  if (dviFile->page_offset == 0)
    return false;

  for(current_page=0; current_page < dviFile->total_pages; current_page++) {
    PostScriptOutPutString = new QString();
    
    if (current_page < dviFile->total_pages) {
      command_pointer = dviFile->dvi_Data + dviFile->page_offset[current_page];
      end_pointer     = dviFile->dvi_Data + dviFile->page_offset[current_page+1];
    } else
      command_pointer = end_pointer = 0;
    
    memset((char *) &currinf.data, 0, sizeof(currinf.data));
    currinf.fonttable = tn_table;
    currinf._virtual  = NULL;
    draw_part(dviFile->dimconv, false);

    if (!PostScriptOutPutString->isEmpty())
      PS_interface->setPostScript(current_page, *PostScriptOutPutString);
    delete PostScriptOutPutString;
  }
  PostScriptOutPutString = NULL;
  is_current_page_drawn  = 0;

  QApplication::restoreOverrideCursor();
  reference              = ref;
  return true;
}

void dviWindow::all_fonts_loaded(void)
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
    Q_INT32 page = 0;
    Q_INT32 y    = -1000;
    bool    sourceSpecialsFlag = false;
    QString ref = reference.mid(4);
    
    // Extract the file name and the numeral part from the string
    unsigned int i;
    for(i=0;i<ref.length();i++)
      if (!ref.at(i).isNumber())
	break;
    Q_UINT32 lineNumber = ref.left(i).toUInt();
    QString  fileName   = ref.mid(i);

    bool _postscript_sav = _postscript;
    int current_page_sav = current_page;
    _postscript = FALSE; // Switch off postscript to speed up things...
    QProgressDialog progress( i18n("Searching for position corresponding to line %1 in %2.").arg(ref.left(i)).arg(fileName), 
			      i18n("Abort"), dviFile->total_pages, this, "search_reference_progress", TRUE );
    progress.setMinimumDuration(300);
    QPixmap pixie(1,1);

    for(current_page=0; current_page < dviFile->total_pages; current_page++) {
      progress.setProgress( current_page );
      // Funny. The manual to QT tells us that we need to call
      // qApp->processEvents() regularly to keep the application from
      // freezing. However, the application crashes immediately if we
      // uncomment the following line and works just fine as it is. Wild
      // guess: Could that be related to the fact that we are linking
      // agains qt-mt?
      
      // qApp->processEvents();
      
      if ( progress.wasCancelled() )
	break;
      
      foreGroundPaint.begin( &pixie );
      draw_page(); // We gracefully ingore any errors (bad dvi-file, etc.) which may occur during draw_page()
      foreGroundPaint.end();
      if (num_of_used_source_hyperlinks > 0)
	sourceSpecialsFlag = true;
      for(int i=0; i<num_of_used_source_hyperlinks; i++) {
	QString sourceLink = sourceHyperLinkList[i].linkText;
	// Extract the file name and the numeral part from the string
	unsigned int j;
	for(j=0;j<sourceLink.length();j++)
	  if (!sourceLink.at(j).isNumber())
	    break;
	Q_UINT32 sourceLineNumber = sourceLink.left(j).toUInt();
	QString  sourceFileName   = sourceLink.mid(j);
	if ((fileName.stripWhiteSpace() == sourceFileName.stripWhiteSpace())&&(lineNumber >= sourceLineNumber)) {
	  page = current_page;
	  y    = sourceHyperLinkList[i].box.top();
	  kdDebug() << sourceLink << endl;
	}
      }
    }
    progress.setProgress( dviFile->total_pages ); // Switch off the progress dialog, etc.
    _postscript = _postscript_sav; // Restore the PostScript setting 
    
    // Restore the current page.
    current_page = current_page_sav;
    foreGroundPaint.begin( &pixie );
    draw_page();  // We gracefully ingore any errors (bad dvi-file, etc.) which may occur during draw_page()
    foreGroundPaint.end();
    
    reference = QString::null;
    if (sourceSpecialsFlag == false) {
      KMessageBox::sorry(this, i18n("<qt>You have asked KDVI to locate the place in the DVI file which corresponds to "
				    "line %1 in the TeX-file <strong>%2</strong>. It seems, however, that the DVI file "
				    "does not contain the necessary source file information. "
				    "We refer to the manual of KDVI for a detailed explanation on how to include this "
				    "information. Press the F1 key to open the manual.</qt>").arg(ref.left(i)).arg(fileName),
			 i18n( "Could not find reference" ));
      return;
    }
    if (y >= 0)
      emit(request_goto_page(page, y));
    if (y < 0)
      KMessageBox::sorry(this, i18n("<qt>KDVI was not able to locate the place in the DVI file which corresponds to "
				    "line %1 in the TeX-file <strong>%2</strong>.</qt>").arg(ref.left(i)).arg(fileName),
			 i18n( "Could not find reference" ));
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
  flashOffset      = vflashOffset - pixmap->height()/100; // Heuristic correction. Looks better.
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
  repaint();
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
    bitBlt ( this, 0, 0, pixmap, 0, 0, -1, -1, CopyROP, TRUE);
    QPainter p(this);
    if (animationCounter > 0 && animationCounter < 10) {
      int wdt = pixmap->width()/(10-animationCounter);
      int hgt = pixmap->height()/((10-animationCounter)*20);
      p.setPen(QPen(QColor(150,0,0), 3, DashLine));
      p.drawRect((pixmap->width()-wdt)/2, flashOffset, wdt, hgt);
    }

    // Mark selected text.
    if (DVIselection.selectedTextStart != -1)
      for(int i = DVIselection.selectedTextStart; (i <= DVIselection.selectedTextEnd)&&(i < num_of_used_textlinks); i++) {
	p.setPen( NoPen );
	p.setBrush( white );
	p.setRasterOp( Qt::XorROP );
	p.drawRect(textLinkList[i].box);
      }
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
  
  if ((e->state() & RightButton) != 0) {
    if (selectedRectangle.isEmpty())
      selectedRectangle.setRect(e->pos().x(),e->pos().y(),1,1);
    else {
      QRect R(e->pos().x(),e->pos().y(),1,1);
      selectedRectangle = selectedRectangle.unite(R);
    }
    
    // Now that we know the rectangle, we have to find out which words
    // intersect it!
    Q_INT32 selectedTextStart = -1;
    Q_INT32 selectedTextEnd   = -1;

    for(int i=0; i<num_of_used_textlinks; i++) 
      if ( selectedRectangle.intersects(textLinkList[i].box) ) {
	if (selectedTextStart == -1)
	  selectedTextStart = i;
	selectedTextEnd = i;
      }

    QString selectedText("");
    if (selectedTextStart != -1)
      for(int i = selectedTextStart; (i <= selectedTextEnd)&&(i < num_of_used_textlinks); i++) {
	selectedText += textLinkList[i].linkText;
	selectedText += "\n";
      }

    if (selectedTextEnd == -1)
      DVIselection.clear();
    else
      DVIselection.set(selectedTextStart, selectedTextEnd, selectedText);
    repaint();
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
  if ((e->button() == LeftButton) && (num_of_used_hyperlinks > 0))
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
	  // We could in principle use KIO::Netaccess::run() here, but
	  // it is perhaps not a very good idea to allow a DVI-file to
	  // specify arbitrary commands, such as "rm -rvf /*". Using
	  // the kfmclient seems to be MUCH safer.
	  QUrl DVI_Url(dviFile->filename);
	  QUrl Link_Url(DVI_Url, hyperLinkList[i].linkText, TRUE );
	
	  KShellProcess proc;
	  proc << "kfmclient openURL " << Link_Url.toString();
	  proc.start(KProcess::Block);
	}
	break;
      }
    }

  // Check if the mouse is pressed on a source-hyperlink
  if ((e->button() == MidButton) && (num_of_used_source_hyperlinks > 0))
    for(int i=0; i<num_of_used_source_hyperlinks; i++) 
      if (sourceHyperLinkList[i].box.contains(e->pos())) {
#ifdef DEBUG_SPECIAL
	kdDebug(4300) << "Source hyperlink to " << sourceHyperLinkList[i].linkText << endl;
#endif

	QString cp = sourceHyperLinkList[i].linkText;
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
			       i18n( "Could not find file" ));
	    return;
	  }
	}

	QString command = editorCommand;
	if (command.isEmpty() == true) {
	  int r = KMessageBox::warningContinueCancel(this, i18n("You have not yet specified an editor for inverse search. "
								"Please choose your favourite editor in the "
								"<strong>DVI options dialog</strong> "
								"which you will find in the <strong>Settings</strong>-menu."),
						     "Need to specify editor",
						     "Use KDE's editor kate for now");
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
	export_errorString = i18n("<qt>The external program<br/><br/><nobr><tt>%1</strong></tt><br/></br>which was used to call the editor "
				  "for inverse search, reported an error. You might wish to look at the <strong>document info "
				  "dialog</strong> which you will find in the File-Menu for a precise error report. The "
				  "manual of KDVI contains a detailed explanation how to set up your editor for use with KDVI, "
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
