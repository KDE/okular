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
#include <qmsgbox.h>
#include <kapp.h>
#include <kdebug.h>
#include <klocale.h>

class DVIFile 
{
	Q_OBJECT
public:
	DVIFile(){}
	~DVIFile(){};
	void dviCopy(QString ifile, QString ofile, QStrList *pagelist,
			int first = 0, int last = 999999 );

};

print::print
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


print::~print()
{
}

void print::setFile( QString _file )
{
	ifile = ofile = _file.copy();
	setCaption( QString(i18n(i18n("Print "))) + ifile );
	QString of( _file );
	if ( of.right(4) == ".dvi" )
		of = of.left( of.length()-4 );
	of.append( printMethod == "dvilj4" ? ".lj" : ".ps" );
	printFileName->setText(of);
}

void print::setCurrentPage( int _page, int _totalpages )
{
	curpage = _page;
	totalpages = _totalpages;
}

void print::setMarkList( QStrList *_marklist )
{
	marklist = _marklist;
	if ( !marklist || marklist->isEmpty() )
		return;
	printMarked->setEnabled( TRUE );
	printMarked->setChecked( TRUE );
	printAll->setChecked( FALSE );		
	printRange->setChecked( FALSE );		
	printCurrent->setChecked( FALSE );		
}

void print::rangeToggled( bool on )
{
	if ( on )
	{
		rangeFrom->setEnabled( TRUE );
		rangeTo->setEnabled( TRUE );
		rangeFrom->setFocus();
	}
	else
	{
		rangeFrom->setEnabled( FALSE );
		rangeTo->setEnabled( FALSE );
	}
}

#define get2(f) ((f.getch()<<8) + f.getch())
#define get4(f) ((get2(f)<<16) + get2(f))
#define put2(f,i) (f.putch(((i)>>8)&0xff), f.putch((i)&0xff))
#define put4(f,i) (put2(f,i>>16), put2(f,(i)))

void DVIFile::dviCopy(QString ifile, QString ofile, QStrList *pagelist,
	int first, int last)
{
	QFile in(ifile);
	QFile out(ofile);
	char buf[1024];
/*	these would be needed for better handling the used fonts
	int texfont[256], fontdeflen[256], fontseen[256];
	for (int i=0; i<256; i++ )
		texfont[i] = fontdeflen[i] = fontseen[i] = 0;
*/
	if (!in.open(IO_ReadOnly))
		QMessageBox::message( i18n("Warning"),
			i18n("Cannot open dvi file!"),
			i18n("OK") );
	if (!out.open(IO_WriteOnly))
		QMessageBox::message( i18n("Warning"),
			i18n("Cannot open output dvi file!"),
			i18n("OK") );;
	in.readBlock( buf, 14 );
	out.writeBlock( buf, 14 );
	out.writeBlock( "\013kdvi output", 12 );
	
	int c, p = in.size()-3, n, tot, totout = 0, o = -1;
	in.at(p);
	while ( ( c = in.getch() ) == 223 ) // trailer
		in.at( --p );
	if ( c != 2 )
		QMessageBox::message( i18n("Warning"),
			i18n("Cannot handle this dvi version!"),
			i18n("OK") );;
	int post_post = p - 5;
	in.at( post_post + 1 );
	int post =  get4( in );
	in.at( post + 27 );
	tot = n = get2( in );
	int fntdefslen = post_post - in.at();
	int *pg = new int[tot + 1];
	pg[ tot ] = post;
	p = post - 40;
	while (n--)
	{
		in.at( p + 41 ),
		pg[n] = p = get4( in );
	}
/*
	for ( p = post + 29; p < post_post; p++ )
	{
		in.at( p );
		c = in.getch();
		if ( c == 138 ) )	// nop
			continue;
		if ( c == 243 )		// fnt_def1
		{
			texfont[ n = in.getch() ] = p;
			in.at( p + 14 );
			fontdeflen[ n ] = in.getch() + in.getch();
			p += 16 + fontdeflen[ n ];
		}
		else	debug("DVI file format error!");
	}
*/

	int defsdone = 0;
	for ( n=0; n < tot; n++ )	// copy pages to out
	{		
		if ( pagelist &&
			pagelist->find( QString().sprintf( "%4d", n + 1 ) ) < 0 )
			continue;
		if ( n + 1 < first || n + 1 > last )
			continue;
		in.at( pg[n] );
		in.readBlock( buf, 41 );
		out.writeBlock( buf, 41 );
		put4( out, o );
		o = out.at() - 45;
		if ( !defsdone ) // cannot copy fnt defs before page 1 due to bug in dvips
		{
			in.at( post + 29 );
			// copy font defs from postamble (bug: all not needed)
			for ( int i = fntdefslen; i>0; i-- )
				out.putch( in.getch() );
			defsdone = 1;
		}		
		in.at( pg[n] + 45 );
		for ( int i = pg[n+1] - pg[n] - 45; i > 0; i-- )
			out.putch( in.getch() );
		totout++;
	}
	out.putch( 248 );	// post
	put4( out, o );
	o = out.at() - 5;
	in.at( post + 5 );
	in.readBlock( buf, 22  );
	out.writeBlock( buf, 22 );
	put2( out, totout );
	in.at( post + 29 );
	for ( int i = post_post - ( post + 29 ); i > 0; i-- )
		out.putch( in.getch() );
	out.putch( 249 );	// post_post
	put4( out, o );
	out.putch( 2 );		// dvi file version id
	put4( out, 0xdfdfdfdf );// trailers
	while ( out.at() & 3 )
		out.putch( 0xdf );
	in.close();
	out.close();
	delete pg;
}

void print::okPressed()
{
	QString cmd;
	
	cmd = printMethod == "dvilj4" ? "dvilj4 -q -e-" : "dvips -q -f";

	if ( printReverse->isOn() )
		cmd += " -r";

	if ( ! printAll->isOn() )
	{
		DVIFile dvi;
		ofile = tmpnam(NULL);
		if ( printCurrent->isOn() )
			dvi.dviCopy( ifile, ofile, NULL, curpage, curpage );
		else if ( printMarked->isOn()  )
			dvi.dviCopy( ifile, ofile, marklist );
		else if ( printRange->isOn() )
		{
			int f = QString(rangeFrom->text()).toInt(),
			    t = QString(rangeTo->text()).toInt();
			if ( f < 1 || f > totalpages || t < f || t > totalpages )
			{
				QMessageBox::message( i18n("Warning"),
					i18n("Invalid page range!"),
					i18n("OK") );
				return;
			}
			dvi.dviCopy( ifile, ofile, NULL, f, t );
		}
	}

	cmd += " " + ofile;

	if ( nup != 1 )
	{
		cmd += nupProgram == "mpage" ? " | mpage -" : " | psnup -";
		cmd +=  QString().setNum(nup);
		if ( colOrder->currentItem() == 1 )
			cmd += nupProgram == "mpage" ? " -a" : " -c";

	}

	if ( printdest == 1 )
		cmd += QString(" > ") + printFileName->text();
	else
	{
		cmd += QString(" | ") + spooler;
		if ( printdest > 1 )
			cmd += QString(" -P") + 
				printer->text(printer->currentItem());
	}

	if ( ifile != ofile )
		cmd += " ; rm " + ofile;

	cmd += " &";

	KDEBUG( KDEBUG_INFO, 0, QString( "About to run: " + cmd ) );
	system( cmd );
	accept();
}

void print::nupPressed(int n)
{
	nup = 1 << n;
}

void print::printDestinationChanged(int i)
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

void print::setupPressed()
{
	printSetup * ps = new printSetup( this, "ps" );
	ps->exec();
	delete ps;
	readConfig();
}

void print::cancelPressed()
{
	reject();
}

void print::readConfig()
{
	KConfig *config = kapp->getConfig();

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
	printMethod = config->readEntry( "PrintMethod", "dvips" );
	nupCombo->setEnabled( printMethod == "dvips" );
	colOrder->setEnabled( printMethod == "dvips" );
	spooler = config->readEntry( "SpoolerCommand", "lpr" );
	
	config->setGroup( "kdvi" );
}
