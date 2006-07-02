// infodialog.cpp
//
// (C) 2001-2003 Stefan Kebekus
// Distributed under the GPL

#include <config.h>

#include <kdebug.h>
#include <kio/global.h>
#include <klocale.h>
#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qtextview.h>
#include <qtooltip.h>
#include <qvariant.h>
#include <qwhatsthis.h>

#include "dviFile.h"
#include "fontpool.h"
#include "infodialog.h"

infoDialog::infoDialog( QWidget* parent )
  : KDialogBase( Tabbed, i18n("Document Info"), Ok, Ok, parent, "Document Info", false, false)
{
  QFrame *page1 = addPage( i18n("DVI File") );
  QVBoxLayout *topLayout1 = new QVBoxLayout( page1, 0, 6 );
  TextLabel1 = new QTextView( page1, "TextLabel1" );
  QToolTip::add( TextLabel1, i18n("Information on the currently loaded DVI-file.") );
  topLayout1->addWidget( TextLabel1 );

  QFrame *page2 = addPage( i18n("Fonts") );
  QVBoxLayout *topLayout2 = new QVBoxLayout( page2, 0, 6 );
  TextLabel2 = new QTextView( page2, "TextLabel1" );
  TextLabel2->setMinimumWidth(fontMetrics().maxWidth()*40);
  TextLabel2->setMinimumHeight(fontMetrics().height()*10);
  QToolTip::add( TextLabel2, i18n("Information on currently loaded fonts.") );
  QWhatsThis::add( TextLabel2, i18n("This text field shows detailed information about the currently loaded fonts. "
				    "This is useful for experts who want to locate problems in the setup of TeX or KDVI.") );
  topLayout2->addWidget( TextLabel2 );

  QFrame *page3 = addPage( i18n("External Programs") );
  QVBoxLayout *topLayout3 = new QVBoxLayout( page3, 0, 6 );
  TextLabel3 = new QTextView( page3, "TextLabel1" );
  TextLabel3->setText( i18n("No output from any external program received.") );
  QToolTip::add( TextLabel3, i18n("Output of external programs.") );
  QWhatsThis::add( TextLabel3, i18n("KDVI uses external programs, such as MetaFont, dvipdfm or dvips. "
				    "This text field shows the output of these programs. "
				    "That is useful for experts who want to find problems in the setup of TeX or KDVI.") );
  topLayout3->addWidget( TextLabel3 );

  MFOutputReceived = false;
  headline         = QString::null;
  pool             = QString::null;
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

  TextLabel1->setText( text );
}


void infoDialog::setFontInfo(fontPool *fp)
{
  TextLabel2->setText(fp->status());
}

void infoDialog::outputReceiver(const QString& _op)
{
  QString op = _op;
  op = op.replace( QRegExp("<"), "&lt;" );

  if (MFOutputReceived == false) {
    TextLabel3->setText("<b>"+headline+"</b><br>");
    headline = QString::null;
  }

  // It seems that the QTextView wants that we append only full lines.
  // We see to that.
  pool = pool+op;
  int idx = pool.findRev("\n");

  while(idx != -1) {
    QString line = pool.left(idx);
    pool = pool.mid(idx+1);

    // If the Output of the kpsewhich program contains a line starting
    // with "kpathsea:", this means that a new MetaFont-run has been
    // started. We filter these lines out and print them in boldface.
    int startlineindex = line.find("kpathsea:");
    if (startlineindex != -1) {
      int endstartline  = line.find("\n",startlineindex);
      QString startLine = line.mid(startlineindex,endstartline-startlineindex);
      if (MFOutputReceived)
	TextLabel3->append("<hr>\n<b>"+startLine+"</b>");
      else
	TextLabel3->append("<b>"+startLine+"</b>");
    TextLabel3->append(line.mid(endstartline));
    } else
      TextLabel3->append(line);
    idx = pool.findRev("\n");
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
