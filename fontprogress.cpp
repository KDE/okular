// fontprogress.cpp
//
// (C) 2001 Stefan Kebekus
// Distributed under the GPL

#include "fontprogress.h"

#include <kdebug.h>
#include <klocale.h>
#include <kprogress.h>
#include <kpushbutton.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <qvbox.h>

/* 
 *  Constructs a fontProgressDialog which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
fontProgressDialog::fontProgressDialog( QWidget* parent,  const QString &name )
  : KDialogBase( parent, "Font Generation Progress Dialog", true, name, Cancel, Cancel, true )
{
  setCursor( QCursor( 3 ) );
  //  setCaption( i18n( "Please wait..." ) );

  setButtonCancelText( i18n("Abort"), i18n("Aborts the font generation. Don't do this."));

  setHelp("fontgen", "kdvi");
  setHelpLinkText( i18n( "What's going on here?") ); 
  enableLinkedHelp(true);

  QVBox *page = makeVBoxMainWidget();

  QString tip = i18n( "KDVI is currently generating bitmap fonts which are needed to display your document. For this, KDVI uses a number of external programs, such as MetaFont. You can find the output of these programs later in the document info dialog." );
  QString ttip = i18n( "KDVI is generating fonts. Please wait." );
  
  TextLabel1   = new QLabel( i18n( "KDVI is currently generating bitmap fonts..." ), page, "TextLabel1" );
  TextLabel1->setAlignment( int( QLabel::AlignCenter ) );
  QWhatsThis::add( TextLabel1, tip );
  QToolTip::add( TextLabel1, ttip );

  ProgressBar1 = new KProgress( page, "ProgressBar1" );
  QWhatsThis::add( ProgressBar1, tip );
  QToolTip::add( ProgressBar1, ttip );

  TextLabel2   = new QLabel( "", page, "TextLabel2" );
  TextLabel2->setAlignment( int( QLabel::AlignCenter ) );
  QWhatsThis::add( TextLabel2, tip );
  QToolTip::add( TextLabel2, ttip );
}


/*  
 *  Destroys the object and frees any allocated resources
 */

fontProgressDialog::~fontProgressDialog()
{
    // no need to delete child widgets, Qt does it all for us
}


void fontProgressDialog::outputReceiver(const QString op)
{
  // If the Output of the kpsewhich program contains a line starting
  // with "kpathsea:", this means that a new MetaFont-run has been
  // started. We filter these lines out and print them in boldface.
  int startlineindex = op.find("kpathsea:");
  if (startlineindex != -1) {
    ProgressBar1->setValue(progress++);

    int endstartline  = op.find("\n",startlineindex);
    QString startLine = op.mid(startlineindex,endstartline-startlineindex);

    // The last word in the startline is the name of the font which we
    // are generating. The second-to-last word is the resolution in
    // dots per inch. Display this info in the text label below the
    // progress bar.
    int lastblank     = startLine.findRev(' ');
    QString fontName  = startLine.mid(lastblank+1);
    int secondblank   = startLine.findRev(' ',lastblank-1);
    QString dpi       = startLine.mid(secondblank+1,lastblank-secondblank-1);
    TextLabel2->setText( i18n("Currently generating %1 at %2 dpi").arg(fontName).arg(dpi) );
  }
}

void fontProgressDialog::hideDialog(void)
{
  hide();
}

void fontProgressDialog::setTotalSteps(int steps)
{
  ProgressBar1->setRange(0,steps);
  ProgressBar1->setValue(0);
  progress = 0;
}

void fontProgressDialog::show(void)
{
  KDialogBase::show();
}

