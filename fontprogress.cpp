// fontprogress.cpp
//
// (C) 2001--2004 Stefan Kebekus
// Distributed under the GPL

#include "fontprogress.h"

#include <kdebug.h>
#include <klocale.h>
#include <kprocio.h>
#include <kprogress.h>
#include <kpushbutton.h>
#include <qapplication.h>
#include <q3frame.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <q3whatsthis.h>

#include <q3vbox.h>

/* 
 *  Constructs a fontProgressDialog which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
fontProgressDialog::fontProgressDialog( QString helpIndex, QString label, QString abortTip, QString whatsThis, QString ttip, QWidget* parent, const QString &name, bool progressbar )
  : KDialogBase( parent, "Font Generation Progress Dialog", true, name, Cancel, Cancel, true )
{
  setCursor( QCursor( 3 ) );

  setButtonCancel(KGuiItem(i18n("Abort"), abortTip));

  if (helpIndex.isEmpty() == false) {
    setHelp(helpIndex, "kdvi");
    setHelpLinkText( i18n( "What's going on here?") ); 
    enableLinkedHelp(true);
  } else
    enableLinkedHelp(false);

  Q3VBox *page = makeVBoxMainWidget();

  TextLabel1   = new QLabel( label, page, "TextLabel2" );
  TextLabel1->setAlignment( int( Qt::AlignCenter ) );
  Q3WhatsThis::add( TextLabel1, whatsThis );
  QToolTip::add( TextLabel1, ttip );

  if (progressbar) {
    ProgressBar1 = new KProgress( page, "ProgressBar1" );
    ProgressBar1->setFormat(i18n("%v of %m"));
    Q3WhatsThis::add( ProgressBar1, whatsThis );
    QToolTip::add( ProgressBar1, ttip );
  } else 
    ProgressBar1 = NULL;
  
  TextLabel2   = new QLabel( "", page, "TextLabel2" );
  TextLabel2->setAlignment( int( Qt::AlignCenter ) );
  Q3WhatsThis::add( TextLabel2, whatsThis );
  QToolTip::add( TextLabel2, ttip );

  progress = 0;
  procIO = 0;
  qApp->connect(this, SIGNAL(finished(void)), this, SLOT(killProcIO(void)));
}


/*  
 *  Destroys the object and frees any allocated resources
 */

fontProgressDialog::~fontProgressDialog()
{
    // no need to delete child widgets, Qt does it all for us
}


void fontProgressDialog::increaseNumSteps( const QString explanation)
{
  if (ProgressBar1 != 0)
    ProgressBar1->setProgress(progress++);
  TextLabel2->setText( explanation );
}


void fontProgressDialog::setTotalSteps(int steps, KProcIO *proc)
{
  procIO = proc;
  if (ProgressBar1 != 0) {
    ProgressBar1->setTotalSteps(steps);
    ProgressBar1->setProgress(0);
  }
  progress = 0;
}


void fontProgressDialog::killProcIO(void)
{
  if (!procIO.isNull())
    procIO->kill();
}


#include "fontprogress.moc"
