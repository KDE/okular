//
// Class: dviRenderer
// Author: Stefan Kebekus
//
// (C) 2001-2004, Stefan Kebekus.
//
// Previewer for TeX DVI files.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.
//
// Please report bugs or improvements, etc. via the "Report bug"-Menu
// of kdvi.

#include <config.h>

#include <kapplication.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kprinter.h>
#include <kprocess.h>
#include <ktempfile.h>
#include <qlabel.h>
#include <qpainter.h>

#include "dviRenderer.h"
#include "dviFile.h"
#include "fontprogress.h"
#include "infodialog.h"
#include "kdvi_multipage.h"

extern QPainter foreGroundPaint; // QPainter used for text



void dviRenderer::exportPDF()
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
    KMessageBox::sorry(0, i18n("KDVI could not locate the program 'dvipdfm' on your computer. That program is "
			       "essential for the export function to work. You can, however, convert "
			       "the DVI-file to PDF using the print function of KDVI, but that will often "
			       "produce documents which print ok, but are of inferior quality if viewed in the "
			       "Acrobat Reader. It may be wise to upgrade to a more recent version of your "
			       "TeX distribution which includes the 'dvipdfm' program.\n"
			       "Hint to the perplexed system administrator: KDVI uses the shell's PATH variable "
			       "when looking for programs."));
    return;
  }

  // Generate a suggestion for a reasonable file name
  QString suggestedName = dviFile->filename;
  suggestedName = suggestedName.left(suggestedName.find(".")) + ".pdf";

  QString fileName = KFileDialog::getSaveFileName(suggestedName, i18n("*.pdf|Portable Document Format (*.pdf)"), parentWidget, i18n("Export File As"));
  if (fileName.isEmpty())
    return;
  QFileInfo finfo(fileName);
  if (finfo.exists()) {
    int r = KMessageBox::warningContinueCancel (parentWidget, i18n("The file %1\nexists. Do you want to overwrite that file?").arg(fileName),
				       i18n("Overwrite File"), i18n("Overwrite"));
    if (r == KMessageBox::Cancel)
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
				     parentWidget, i18n("dvipdfm progress dialog"), false );
  if (progress != 0) {
    progress->TextLabel2->setText( i18n("Please be patient") );
    progress->setTotalSteps( dviFile->total_pages );
    qApp->connect(progress, SIGNAL(finished()), this, SLOT(abortExternalProgramm()));
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
  info->clear(i18n("Export: %1 to PDF").arg(KShellProcess::quote(dviFile->filename)));

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


void dviRenderer::exportPS(const QString& fname, const QString& options, KPrinter* printer)
{
  // Safety check.
  if (dviFile->page_offset.isEmpty() == true)
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

  if (dviFile->numberOfExternalNONPSFiles != 0) {
    KMessageBox::sorry( parentWidget, 
			i18n("<qt><P>This DVI file refers to external graphic files which are not in PostScript format, and cannot be handled by the "
			     "<strong>dvips</strong> program that KDVI uses interally to print or to export to PostScript. The functionality that "
			     "you require is therefore unavailable in this version of KDVI.</p>"
			     "<p>As a workaround, you can use the <strong>File/Export As</strong>-Menu to save this file in PDF format, and then use "
			     "a PDF viewer.</p>"
			     "<p>The author of KDVI apologizes for the inconvenience. If enough users complain, the missing functionality might later "
			     "be added.</p></qt>") ,
			i18n("Functionality Unavailable"));
    return;
  }
  
  QString fileName;
  if (fname.isEmpty()) {
    // Generate a suggestion for a reasonable file name
    QString suggestedName = dviFile->filename;
    suggestedName = suggestedName.left(suggestedName.find(".")) + ".ps";

    fileName = KFileDialog::getSaveFileName(suggestedName, i18n("*.ps|PostScript (*.ps)"), parentWidget, i18n("Export File As"));
    if (fileName.isEmpty())
      return;
    QFileInfo finfo(fileName);
    if (finfo.exists()) {
      int r = KMessageBox::warningYesNo (parentWidget, i18n("The file %1\nexists. Do you want to overwrite that file?").arg(fileName),
					 i18n("Overwrite File"));
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
				     parentWidget, i18n("dvips progress dialog"), false );
  if (progress != 0) {
    progress->TextLabel2->setText( i18n("Please be patient") );
    progress->setTotalSteps( dviFile->total_pages );
    qApp->connect(progress, SIGNAL(finished()), this, SLOT(abortExternalProgramm()));
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

  // A similar problem occurs with DVI files that contain page size
  // information. On these files, dvips pointblank refuses to change
  // the page orientation or set another page size. Thus, if the
  // DVI-file does contain page size information, we remove that
  // information first.

  // Sourcefile is the name of the DVI which is used by dvips, either
  // the original file, or a temporary file with a new numbering.
  QString sourceFileName = dviFile->filename;
  if ((options.isEmpty() == false) || (dviFile->suggestedPageSize != 0) ) {
    // Get a name for a temporary file.
    KTempFile export_tmpFile;
    export_tmpFileName = export_tmpFile.name();
    export_tmpFile.unlink();
    
    sourceFileName     = export_tmpFileName;
    
    fontPool fp;
    dvifile newFile(dviFile, &fp);

    // Renumber pages
    newFile.renumber();

    // Remove any page size information from the file
    Q_UINT16 currPageSav = current_page;
    dvifile *dvsav =  dviFile;
    dviFile = &newFile;
    errorMsg = QString::null;

    
    for(current_page=0; current_page < newFile.total_pages; current_page++) {
      if (current_page < newFile.total_pages) {
	command_pointer = dviFile->dvi_Data() + dviFile->page_offset[current_page];
	end_pointer     = dviFile->dvi_Data() + dviFile->page_offset[current_page+1];
      } else
	command_pointer = end_pointer = 0;
      
      memset((char *) &currinf.data, 0, sizeof(currinf.data));
      currinf.fonttable = &(dviFile->tn_table);
      currinf._virtual  = NULL;
      prescan(&dviRenderer::prescan_removePageSizeInfo);
    }
    
    current_page = currPageSav;
    dviFile = dvsav;
    newFile.saveAs(sourceFileName);
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
  export_errorString = i18n("<qt>The external program 'dvips', which was used to export the file, reported an error. "
			    "You might wish to look at the <strong>document info dialog</strong> which you will "
			    "find in the File-Menu for a precise error report.</qt>") ;
  info->clear(i18n("Export: %1 to PostScript").arg(KShellProcess::quote(dviFile->filename)));
  
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


void dviRenderer::dvips_output_receiver(KProcess *, char *buffer, int buflen)
{
  // Paranoia.
  if (buflen < 0)
    return;
  QString op = QString::fromLocal8Bit(buffer, buflen);

  info->outputReceiver(op);
  if (progress != 0)
    progress->show();
}


void dviRenderer::dvips_terminated(KProcess *sproc)
{
  // Give an error message from the message string. However, if the
  // sproc is not the "current external process of interest", i.e. not
  // the LAST external program that was started by the user, then the
  // export_errorString, does not correspond to sproc. In that case,
  // we ingore the return status silently.
  if ((proc == sproc) && (sproc->normalExit() == true) && (sproc->exitStatus() != 0))
    KMessageBox::error( parentWidget, export_errorString );

  if (export_printer != 0)
    export_printer->printFiles( QStringList(export_fileName), true );

  // Kill and delete the remaining process, delete the printer, etc.
  abortExternalProgramm();
}


void dviRenderer::editorCommand_terminated(KProcess *sproc)
{
  // Give an error message from the message string. However, if the
  // sproc is not the "current external process of interest", i.e. not
  // the LAST external program that was started by the user, then the
  // export_errorString, does not correspond to sproc. In that case,
  // we ingore the return status silently.
  if ((proc == sproc) && (sproc->normalExit() == true) && (sproc->exitStatus() != 0))
    KMessageBox::error( parentWidget, export_errorString );

  // Let's hope that this is not all too nasty... killing a
  // KShellProcess from a slot that was called from the KShellProcess
  // itself. Until now, there weren't any problems.

  // Perhaps it was a bad idea, after all.
  //@@@@  delete sproc;
}


void dviRenderer::abortExternalProgramm()
{
  delete proc; // Deleting the KProcess kills the child.
  proc = 0;
  
  if (export_tmpFileName.isEmpty() != true) {
    unlink(QFile::encodeName(export_tmpFileName)); // That should delete the file.
    export_tmpFileName = "";
  }
  
  if (progress != 0) {
    progress->hide();
    delete progress;
    progress = 0;
  }
  
  delete export_printer;
  export_printer  = 0;
  export_fileName = "";
}
