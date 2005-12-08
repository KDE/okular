// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// fontprogress.cpp
//
// (C) 2001--2004 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include "fontprogress.h"

#include <klocale.h>
#include <kprogress.h>
#include <kvbox.h>

#include <QApplication>
#include <QLabel>
#include <QProcess>
#include <QToolTip>


/*
 *  Constructs a fontProgressDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
fontProgressDialog::fontProgressDialog(const QString& helpIndex, const QString& label, const QString& abortTip, const QString& whatsThis, const QString& ttip, QWidget* parent, const QString& name, bool progressbar)
  : KDialogBase( parent, "Font Generation Progress Dialog", true, name, Cancel, Cancel, true ),
    TextLabel2(0),
    TextLabel1(0),
    ProgressBar1(0),
    progress(0),
    process(0)
{
  setCursor( QCursor( 3 ) );

  setButtonCancel(KGuiItem(i18n("Abort"), "stop", abortTip));

  if (helpIndex.isEmpty() == false) {
    setHelp(helpIndex, "kdvi");
    setHelpLinkText( i18n( "What's going on here?") );
    enableLinkedHelp(true);
  } else
    enableLinkedHelp(false);

  KVBox *page = makeVBoxMainWidget();

  TextLabel1   = new QLabel( label, page, "TextLabel2" );
  TextLabel1->setAlignment( int( Qt::AlignCenter ) );
  TextLabel1->setWhatsThis( whatsThis );
  QToolTip::add( TextLabel1, ttip );

  if (progressbar) {
    ProgressBar1 = new KProgress( page, "ProgressBar1" );
    ProgressBar1->setFormat(i18n("%v of %m"));
    ProgressBar1->setWhatsThis( whatsThis );
    QToolTip::add( ProgressBar1, ttip );
  } else
    ProgressBar1 = NULL;

  TextLabel2   = new QLabel( "", page, "TextLabel2" );
  TextLabel2->setAlignment( int( Qt::AlignCenter ) );
  TextLabel2->setWhatsThis( whatsThis );
  QToolTip::add( TextLabel2, ttip );

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
    ProgressBar1->setProgress(progress++);
  TextLabel2->setText( explanation );
}


void fontProgressDialog::setTotalSteps(int steps, QProcess* proc)
{
  process = proc;
  if (ProgressBar1 != 0) {
    ProgressBar1->setTotalSteps(steps);
    ProgressBar1->setProgress(0);
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
