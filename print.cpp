/**********************************************************************

	--- Dlgedit generated file ---

	File: print.cpp
	Last generated: Wed Oct 1 21:53:48 1997

 *********************************************************************/

#include <stdlib.h>
#include "print.h"
#include "printSetup.h"

#define Inherited printData

#include "marklist.h"
#include <qfile.h>
#include <qfileinfo.h>

#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>

Print::Print
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	okButton->setDefault( TRUE );
	setCaption( i18n("Print") );
	readConfig();
	printDestinationChanged( 0 );
	printMarked->setEnabled( FALSE );
	fileNameLabel->setBuddy( printFileName );
	marklist = NULL;
	rangeToggled( FALSE );
	nup = 1;
	printdest = 0;
}


Print::~Print()
{
}

void Print::setFile( QString _file )
{
  ifile = ofile = _file.copy();
  setCaption( i18n("Print ") + ifile );
  QString of( _file );
  if ( of.right(4) == ".dvi" )
    of = of.left( of.length()-4 );
  of.append( ".ps" );
  printFileName->setText(of);
}

void Print::setCurrentPage( int _page, int _totalpages )
{
  curpage = _page;
  totalpages = _totalpages;
}

void Print::setMarkList( const QStringList & _marklist )
{
  marklist = _marklist;
  if ( marklist.isEmpty() )
    return;
  printMarked->setEnabled( TRUE );
  printMarked->setChecked( TRUE );
  printAll->setChecked( FALSE );		
  printRange->setChecked( FALSE );		
  printCurrent->setChecked( FALSE );		
}

void Print::rangeToggled( bool on )
{
  if ( on ) {
    rangeFrom->setEnabled( TRUE );
    rangeTo->setEnabled( TRUE );
    rangeFrom->setFocus();
  } else {
    rangeFrom->setEnabled( FALSE );
    rangeTo->setEnabled( FALSE );
  }
}


void Print::okPressed()
{
  QString cmd;
  QFileInfo finfo(ifile);

  cmd = QString("cd %1; dvips -q -f").arg(KShellProcess::quote(finfo.dirPath(true)));

  if ( printReverse->isOn() )
    cmd += " -r";


  if ( ! printAll->isOn() ) {
    // Print only current page
    if ( printCurrent->isOn() )
      cmd +=  QString(" -pp %1 ").arg(curpage);

    // print range of pages
    if ( printRange->isOn() ) {
      int from = QString(rangeFrom->text()).toInt();
      int to   = QString(rangeTo->text()).toInt();
      if ( from < 1 || from > totalpages || to < from || to > totalpages ) {
	KMessageBox::sorry( 0L, i18n("Invalid page range!"));
	return;
      }
      cmd +=  QString(" -pp %1-%2 ").arg(from).arg(to);
    }

    // print marked pages
    if ( printMarked->isOn() ) {
      if ( marklist.isEmpty() )
	return;

      int commaflag = 0;
      cmd +=  QString(" -pp ");
      for ( QStringList::Iterator it = marklist.begin(); it != marklist.end(); ++it ) {
	if (commaflag == 1) 
	  cmd +=  QString(",");
	else
	  commaflag = 1;

	// Add page numer. Remember that kviewshell starts with page 0
	// while dvips calls the first page "Number 1".
	bool ok;
	int pnr = (*it).toInt( &ok );
	cmd +=  QString("%1").arg(pnr+1);
      }
    }
  }

  cmd += " " + KShellProcess::quote(ifile);

  if ( nup != 1 ) {
    cmd += nupProgram == "mpage" ? " | mpage -" : " | psnup -";
    cmd +=  QString().setNum(nup);
    if ( colOrder->currentItem() == 1 )
      cmd += nupProgram == "mpage" ? " -a" : " -c";
  }
  
  if ( printdest == 1 )
    cmd += QString(" > ") + KShellProcess::quote(printFileName->text());
  else {
    cmd += QString(" | ") + spooler;
    if ( printdest > 1 )
      cmd += QString(" -P") + 
	printer->text(printer->currentItem());
  }

  if ( ifile != ofile )
    cmd += " ; rm " + ofile;

  cmd += " &";

  kdDebug() << "About to run print command: " << cmd << endl;
  system( QFile::encodeName(cmd) );
  accept();
}

void Print::nupPressed(int n)
{
	nup = 1 << n;
}

void Print::printDestinationChanged(int i)
{
	printdest = i;
	if ( printdest == 1 )
	{
		printFileName->setEnabled( TRUE );
		fileNameLabel->setEnabled( TRUE );
	}
	else
	{
		printFileName->setEnabled( FALSE );
		fileNameLabel->setEnabled( FALSE );
	}
}

void Print::setupPressed()
{
	printSetup * ps = new printSetup( this, "ps" );
	ps->exec();
	delete ps;
	readConfig();
}

void Print::cancelPressed()
{
	reject();
}

void Print::readConfig()
{
	KConfig *config = kapp->config();

	printer->clear();
	printer->insertItem( i18n( "Default Printer" ) );
	printer->insertItem( i18n( "File" ) );
	config->setGroup( "Printing" );
	int n = config->readNumEntry( "PrinterCount" );
	if ( n > 0 )
		for ( int i = 1; i <= n; i++ )
		{
			QString p;
			p = config->readEntry( "Printer"+p.setNum( i ) );
			printer->insertItem( p );
		}
	nupProgram = config->readEntry( "NupProgram", "psnup" );
	spooler = config->readEntry( "SpoolerCommand", "lpr" );
	
	config->setGroup( "kdvi" );
}

#undef Inherited
#include "print.moc"
