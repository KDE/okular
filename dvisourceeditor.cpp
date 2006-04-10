// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
/**
 * \file dvisourceeditor.cpp
 * Distributed under the GNU GPL version 2 or (at your option)
 * any later version. See accompanying file COPYING or copy at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * \author Angus Leeming
 * \author Stefan Kebekus
 */

#include <config.h>

#include "dvisourceeditor.h"

#include "documentWidget.h"
#include "dviFile.h"
#include "dviRenderer.h"
#include "dvisourcesplitter.h"
#include "kvs_debug.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>


//#define DEBUG_SPECIAL

DVISourceEditor::DVISourceEditor(dviRenderer& parent,
                                 QWidget* parentWidget,
                                 const QString& linkText,
                                 const QPoint& point,
                                 DocumentWidget* win)
  : started_(false),
    process_(0),
    parent_(&parent),
    parent_widget_(parentWidget)
{
#ifdef DEBUG_SPECIAL
  const RenderedDviPagePixmap* currentDVIPage =
    dynamic_cast<const RenderedDviPagePixmap*> parent->currentlyDrawnPage;
  if (currentDVIPage)
  {
    kDebug(kvs::dvi) << "Source hyperlink to " << currentDVIPage->sourceHyperLinkList[i].linkText << endl;
  }
#endif

  const DVI_SourceFileSplitter splitter(linkText, parent.dviFile->filename);
  const QString TeXfile = splitter.filePath();
  if (!splitter.fileExists())
  {
      KMessageBox::sorry(parentWidget, QString("<qt>") +
                         i18n("The DVI-file refers to the TeX-file "
                              "<strong>%1</strong> which could not be found.", KProcess::quote(TeXfile)) +
                         QString("</qt>"),
                         i18n( "Could Not Find File" ));
      return;
  }

  QString command = parent.editorCommand;
  if (command.isEmpty() == true) {
    const int result =
      KMessageBox::warningContinueCancel(parentWidget, QString("<qt>") +
                                         i18n("You have not yet specified an editor for inverse search. "
                                              "Please choose your favorite editor in the "
                                              "<strong>DVI options dialog</strong> "
                                              "which you will find in the <strong>Settings</strong>-menu.") +
                                         QString("</qt>"),
                                         i18n("Need to Specify Editor"),
                                         i18n("Use KDE's Editor Kate for Now"));
    if (result == KMessageBox::Continue)
      command = "kate %f";
    else
      return;
  }
  command = command.replace( "%l", QString::number(splitter.line()) ).replace( "%f", KProcess::quote(TeXfile) );

#ifdef DEBUG_SPECIAL
  kDebug(kvs::dvi) << "Calling program: " << command << endl;
#endif

  // Set up a shell process with the editor command.
  process_ = new KShellProcess;
  if (process_ == 0) {
    kError(kvs::dvi) << "Could not allocate ShellProcess for the editor command." << endl;
    return;
  }
  connect(process_, SIGNAL(receivedStderr(KProcess *, char *, int)), this, SLOT(output_receiver(KProcess *, char *, int)));
  connect(process_, SIGNAL(receivedStdout(KProcess *, char *, int)), this, SLOT(output_receiver(KProcess *, char *, int)));
  connect(process_, SIGNAL(processExited(KProcess *)), this, SLOT(finished(KProcess *)));

  // Merge the editor-specific editor message here.
  error_message_ = i18n("<qt>The external program<br><br><tt><strong>%1</strong></tt><br/><br/>which was used to call the editor "
                        "for inverse search, reported an error. You might wish to look at the <strong>document info "
                        "dialog</strong> which you will find in the File-Menu for a precise error report. The "
                        "manual for KDVI contains a detailed explanation how to set up your editor for use with KDVI, "
                        "and a list of common problems.</qt>", command);

  parent.update_info_dialog(i18n("Starting the editor..."), true);

  // Heuristic correction. Looks better.
  win->flash(point.y());

  *process_ << command;
  process_->closeStdin();
  if (!process_->start(KProcess::NotifyOnExit, KProcess::AllOutput))
    kError(kvs::dvi) << "Editor failed to start" << endl;
  else
    started_ = true;
}


DVISourceEditor::~DVISourceEditor()
{
  delete process_;
}


void DVISourceEditor::output_receiver(KProcess*, char* buffer, int buflen)
{
  if (buflen > 0) {
    parent_->update_info_dialog(QString::fromLocal8Bit(buffer, buflen));
  }
}


void DVISourceEditor::finished(KProcess* process)
{
  if (!process_ || process != process_)
    return;

  if (process->normalExit() && process->exitStatus() != 0)
    KMessageBox::error(parent_widget_, error_message_);
  // Remove this from the store of all export processes.
  parent_->editor_finished(this);
}


#include "dvisourceeditor.moc"
