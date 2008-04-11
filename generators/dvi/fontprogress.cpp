// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
// fontprogress.cpp
//
// (C) 2001--2004 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include "fontprogress.h"

#include <klocale.h>
#include <kvbox.h>

#include <QApplication>
#include <QLabel>
#include <QProcess>

#include <QProgressBar>


/*
 *  Constructs a fontProgressDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
fontProgressDialog::fontProgressDialog(const QString& helpIndex, const QString& label, const QString& abortTip, const QString& whatsThis, const QString& ttip, QWidget* parent, bool progressbar)
  : KDialog( parent),
    TextLabel2(0),
    TextLabel1(0),
    ProgressBar1(0),
    progress(0),
    process(0)
{
  setCaption( i18n( "Font Generation Progress Dialog" ) );
  setModal( true );
  setButtons( Cancel );
  setDefaultButton( Cancel );
  setCursor(QCursor(Qt::WaitCursor));

  setButtonGuiItem(Cancel, KGuiItem(i18n("Abort"), "process-stop", abortTip));

  if (helpIndex.isEmpty() == false) {
    setHelp(helpIndex, "okular");
    setHelpLinkText( i18n( "What is happening here?") );
    enableLinkedHelp(true);
  } else
    enableLinkedHelp(false);

  KVBox* page = new KVBox( this );
  setMainWidget( page );

  TextLabel1   = new QLabel(label, page);
  TextLabel1->setAlignment(Qt::AlignCenter);
  TextLabel1->setWhatsThis( whatsThis );
  TextLabel1->setToolTip( ttip );

  if (progressbar) {
    ProgressBar1 = new QProgressBar( page );
    ProgressBar1->setFormat(i18n("%v of %m"));
    ProgressBar1->setWhatsThis( whatsThis );
    ProgressBar1->setToolTip( ttip );
  } else
    ProgressBar1 = NULL;

  TextLabel2   = new QLabel("", page);
  TextLabel2->setAlignment(Qt::AlignCenter);
  TextLabel2->setWhatsThis( whatsThis );
  TextLabel2->setToolTip( ttip );

  qApp->connect(this, SIGNAL(finished()), this, SLOT(killProcess()));
}


/*
 *  Destroys the object and frees any allocated resources
 */

fontProgressDialog::~fontProgressDialog()
{
    // no need to delete child widgets, Qt does it all for us
}


void fontProgressDialog::increaseNumSteps(const QString& explanation)
{
  if (ProgressBar1 != 0)
    ProgressBar1->setValue(progress++);
  TextLabel2->setText( explanation );
}


void fontProgressDialog::setTotalSteps(int steps, QProcess* proc)
{
  process = proc;
  if (ProgressBar1 != 0) {
    ProgressBar1->setMaximum(steps);
    ProgressBar1->setValue(0);
  }
  progress = 0;
}


void fontProgressDialog::killProcess()
{
  if (!process.isNull()) {
    process->kill();
    process = 0;
  }
}

#include "fontprogress.moc"
