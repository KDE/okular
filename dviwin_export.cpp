//
// Class: dviWindow
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// Please report bugs or improvements, etc. via the "Report bug"-Menu
// of kdvi.


#include <kapplication.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kprinter.h>
#include <kprocess.h>
#include <qlabel.h>
#include <qpainter.h>


#include "dviwin.h"
#include "fontprogress.h"
#include "infodialog.h"
#include "kdvi_multipage.h"

extern QPainter foreGroundPaint; // QPainter used for text



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
  if (_parentMPage->dviFile == NULL)
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
  QString suggestedName = _parentMPage->dviFile->filename;
  suggestedName = suggestedName.left(suggestedName.find(".")) + ".pdf";

  QString fileName = KFileDialog::getSaveFileName(suggestedName, i18n("*.pdf|Portable Document Format (*.pdf)"), _parentMPage->scrollView(), i18n("Export File As"));
  if (fileName.isEmpty())
    return;
  QFileInfo finfo(fileName);
  if (finfo.exists()) {
    int r = KMessageBox::warningYesNo (_parentMPage->scrollView(), i18n("The file %1\nexists. Do you want to overwrite that file?").arg(fileName),
				       i18n("Overwrite File"));
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
				     _parentMPage->scrollView(), "dvipdfm progress dialog", false );
  if (progress != 0) {
    progress->TextLabel2->setText( i18n("Please be patient") );
    progress->setTotalSteps( _parentMPage->dviFile->total_pages );
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


  if (_parentMPage->info)
    _parentMPage->info->clear(i18n("Export: %1 to PDF").arg(KShellProcess::quote(_parentMPage->dviFile->filename)));

  proc->clearArguments();
  finfo.setFile(_parentMPage->dviFile->filename);
  *proc << QString("cd %1; dvipdfm").arg(KShellProcess::quote(finfo.dirPath(true)));
  *proc << QString("-o %1").arg(KShellProcess::quote(fileName));
  *proc << KShellProcess::quote(_parentMPage->dviFile->filename);
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
  if (_parentMPage->dviFile->page_offset == 0)
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
  if (_parentMPage->dviFile == NULL)
    return;

  QString fileName;
  if (fname.isEmpty()) {
    // Generate a suggestion for a reasonable file name
    QString suggestedName = _parentMPage->dviFile->filename;
    suggestedName = suggestedName.left(suggestedName.find(".")) + ".ps";

    fileName = KFileDialog::getSaveFileName(suggestedName, i18n("*.ps|PostScript (*.ps)"), _parentMPage->scrollView(), i18n("Export File As"));
    if (fileName.isEmpty())
      return;
    QFileInfo finfo(fileName);
    if (finfo.exists()) {
      int r = KMessageBox::warningYesNo (_parentMPage->scrollView(), i18n("The file %1\nexists. Do you want to overwrite that file?").arg(fileName),
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
				     _parentMPage->scrollView(), "dvips progress dialog", false );
  if (progress != 0) {
    progress->TextLabel2->setText( i18n("Please be patient") );
    progress->setTotalSteps( _parentMPage->dviFile->total_pages );
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
  QString sourceFileName = _parentMPage->dviFile->filename;
  if (options.isEmpty() == false) {
    // Get a name for a temporary file.
    KTempFile export_tmpFile;
    export_tmpFileName = export_tmpFile.name();
    export_tmpFile.unlink();

    sourceFileName     = export_tmpFileName;
    if (KIO::NetAccess::copy(KURL::fromPathOrURL( _parentMPage->dviFile->filename ), KURL::fromPathOrURL( sourceFileName ))) {
      int wordSize;
      bool bigEndian;
      qSysInfo (&wordSize, &bigEndian);
      // Proper error handling? We don't care.
      FILE *f = fopen(QFile::encodeName(sourceFileName),"r+");
      for(Q_UINT32 i=1; i<=_parentMPage->dviFile->total_pages; i++) {
	fseek(f,_parentMPage->dviFile->page_offset[i-1]+1, SEEK_SET);
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
      KMessageBox::error(_parentMPage->scrollView(), i18n("<qt>Failed to copy the DVI-file <strong>%1</strong> to the temporary file <strong>%2</strong>. "
				    "The export or print command is aborted.</qt>").arg(_parentMPage->dviFile->filename).arg(sourceFileName));
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
  export_errorString = i18n("<qt>The external program 'dvips', which was used to export the file, reported an error. "
			    "You might wish to look at the <strong>document info dialog</strong> which you will "
			    "find in the File-Menu for a precise error report.</qt>") ;
  if (_parentMPage->info)
    _parentMPage->info->clear(i18n("Export: %1 to PostScript").arg(KShellProcess::quote(_parentMPage->dviFile->filename)));

  proc->clearArguments();
  QFileInfo finfo(_parentMPage->dviFile->filename);
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

  if (_parentMPage->info != 0)
    _parentMPage->info->outputReceiver(op);
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
    KMessageBox::error( _parentMPage->scrollView(), export_errorString );

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
    KMessageBox::error( _parentMPage->scrollView(), export_errorString );

  // Let's hope that this is not all too nasty... killing a
  // KShellProcess from a slot that was called from the KShellProcess
  // itself. Until now, there weren't any problems.

  // Perhaps it was a bad idea, after all.
  //@@@@  delete sproc;
}



void dviWindow::abortExternalProgramm(void)
{
    delete proc; // Deleting the KProcess kills the child.
    proc = 0;

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
