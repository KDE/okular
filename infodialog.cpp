// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; c-brace-offset: 0; -*-
// infodialog.cpp
//
// (C) 2001-2003 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include "infodialog.h"
#include "dviFile.h"
#include "fontpool.h"

#include <kio/global.h>
#include <klocale.h>

#include <QFile>
#include <QFrame>
#include <QRegExp>
#include <QTextEdit>
#include <QToolTip>
#include <QVBoxLayout>


infoDialog::infoDialog( QWidget* parent )
  : KPageDialog( parent),
    TextLabel1(0),
    TextLabel2(0),
    TextLabel3(0),
    MFOutputReceived(false),
    headline(QString::null),
    pool(QString::null)
{
  setCaption( i18n( "Document Info" ) );
  setButtons( Ok );
  setModal( false );
  setFaceType( KPageDialog::Tabbed );
  QFrame *page1 = new QFrame( this );
  addPage( page1, i18n("DVI File") );
  QVBoxLayout *topLayout1 = new QVBoxLayout( page1 );
  topLayout1->setSpacing( 6 );
  topLayout1->setMargin( 0 );
  TextLabel1 = new QTextEdit(page1);
  TextLabel1->setReadOnly(true);
  TextLabel1->setToolTip( i18n("Information on the currently loaded DVI-file.") );
  topLayout1->addWidget( TextLabel1 );


  QFrame *page2 = new QFrame( this );
  addPage( page2, i18n("Fonts") );
  QVBoxLayout *topLayout2 = new QVBoxLayout( page2 );
  topLayout2->setSpacing( 6 );
  topLayout2->setMargin( 0 );
  TextLabel2 = new QTextEdit(page2);
  TextLabel2->setReadOnly(true);
  TextLabel2->setMinimumWidth(fontMetrics().maxWidth()*40);
  TextLabel2->setMinimumHeight(fontMetrics().height()*10);
  TextLabel2->setToolTip( i18n("Information on currently loaded fonts.") );
  TextLabel2->setWhatsThis( i18n("This text field shows detailed information about the currently loaded fonts. "
                                    "This is useful for experts who want to locate problems in the setup of TeX or KDVI.") );
  topLayout2->addWidget( TextLabel2 );

  QFrame *page3 = new QFrame( this );
  addPage( page3, i18n("External Programs") );
  QVBoxLayout *topLayout3 = new QVBoxLayout( page3 );
  topLayout3->setSpacing( 6 );
  topLayout3->setMargin( 0 );
  TextLabel3 = new QTextEdit(page3);
  TextLabel3->setReadOnly(true);
  TextLabel3->setPlainText( i18n("No output from any external program received.") );
  TextLabel3->setToolTip( i18n("Output of external programs.") );
  TextLabel3->setWhatsThis( i18n("KDVI uses external programs, such as MetaFont, dvipdfm or dvips. "
                                    "This text field shows the output of these programs. "
                                    "That is useful for experts who want to find problems in the setup of TeX or KDVI.") );
  topLayout3->addWidget( TextLabel3 );
}


void infoDialog::setDVIData(dvifile *dviFile)
{
  QString text = "";

  if (dviFile == NULL)
    text = i18n("There is no DVI file loaded at the moment.");
  else {
    text.append("<table WIDTH=\"100%\" NOSAVE >");
    text.append(QString("<tr><td><b>%1</b></td> <td>%2</td></tr>").arg(i18n("Filename")).arg(dviFile->filename));

    QFile file(dviFile->filename);
    if (file.exists())
      text.append(QString("<tr><td><b>%1</b></td> <td>%2</td></tr>").arg(i18n("File Size")).arg(KIO::convertSize(file.size())));
    else
      text.append(QString("<tr><td><b> </b></td> <td>%1</td></tr>").arg(i18n("The file does no longer exist.")));

    text.append(QString("<tr><td><b>  </b></td> <td>  </td></tr>"));
    text.append(QString("<tr><td><b>%1</b></td> <td>%2</td></tr>").arg(i18n("#Pages")).arg(dviFile->total_pages));
    text.append(QString("<tr><td><b>%1</b></td> <td>%2</td></tr>").arg(i18n("Generator/Date")).arg(dviFile->generatorString));
  } // else (dviFile == NULL)

  TextLabel1->setHtml( text );
}


void infoDialog::setFontInfo(fontPool *fp)
{
  TextLabel2->setPlainText(fp->status());
}


void infoDialog::outputReceiver(const QString& _op)
{
  QString op = _op;
  op = op.replace( QRegExp("<"), "&lt;" );

  if (MFOutputReceived == false) {
    TextLabel3->setHtml("<b>"+headline+"</b><br>");
    headline = QString::null;
  }

  // It seems that the QTextView wants that we append only full lines.
  // We see to that.
  pool = pool+op;
  int idx = pool.lastIndexOf("\n");

  while(idx != -1) {
    QString line = pool.left(idx);
    pool = pool.mid(idx+1);

    // If the Output of the kpsewhich program contains a line starting
    // with "kpathsea:", this means that a new MetaFont-run has been
    // started. We filter these lines out and print them in boldface.
    int startlineindex = line.indexOf("kpathsea:");
    if (startlineindex != -1) {
      int endstartline  = line.indexOf("\n",startlineindex);
      QString startLine = line.mid(startlineindex,endstartline-startlineindex);
      if (MFOutputReceived)
        TextLabel3->append("<hr>\n<b>"+startLine+"</b>");
      else
        TextLabel3->append("<b>"+startLine+"</b>");
    TextLabel3->append(line.mid(endstartline));
    } else
      TextLabel3->append(line);
    idx = pool.lastIndexOf("\n");
  }

  MFOutputReceived = true;
}


void infoDialog::clear(const QString& op)
{
  headline         = op;
  pool             = QString::null;
  MFOutputReceived = false;
}

#include "infodialog.moc"
