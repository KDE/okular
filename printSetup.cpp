/**********************************************************************

	--- Dlgedit generated file ---

	File: printSetup.cpp
	Last generated: Fri Oct 3 01:51:37 1997

 *********************************************************************/

#include "printSetup.h"
#include <kapp.h>
#include <kconfig.h>
#define Inherited printSetupData

printSetup::printSetup
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	setCaption( i18n( "Print Setup" ) );
	internal->setEnabled( FALSE );
	okButton->setDefault( TRUE );
	spoolerLabel->setBuddy( spoolerCommand );
	readConfig();
}


printSetup::~printSetup()
{
}

void printSetup::addPrinter()
{
	printers->insertItem( newPrinter->text() );
}

void printSetup::removePrinter()
{
	int i = printers->currentItem();
	if ( i < 2 )
		return;
	printers->removeItem( i );
}

void printSetup::okPressed()
{
	KConfig *config = kapp->getConfig();

	config->setGroup( "Printing" );
	for ( int i = printers->count(); i > 2; i-- )
	{
		QString p;
		p = "Printer" + p.setNum( i - 2 );
		config->writeEntry( p, printers->text( i - 1 ) );
	}
	config->writeEntry( "PrinterCount", printers->count() - 2 );
	config->writeEntry( "NupProgram", psnup->isOn() ? "psnup" : "mpage" );
	config->writeEntry( "PrintMethod", internal->isOn() ? "Internal" : dvips->isOn() ? "dvips" : "dvilj4" );
	config->writeEntry( "SpoolerCommand", spoolerCommand->text() );
	config->setGroup( "kdvi" );
	config->sync();
	accept();
}

void printSetup::readConfig()
{
	KConfig *config = kapp->getConfig();

	printers->clear();
	printers->insertItem( i18n( "Default Printer" ) );
	printers->insertItem( i18n( "File" ) );
	config->setGroup( "Printing" );
	int n = config->readNumEntry( "PrinterCount" );
	if ( n > 0 )
		for ( int i = 1; i <= n ; i++ )
		{
			QString p;
			p = config->readEntry( "Printer"+p.setNum( i ) );
			printers->insertItem( p );
		}
	QString nupProgram = config->readEntry( "NupProgram", "psnup" );
	if ( nupProgram == "psnup" ) psnup->setChecked( TRUE );
	if ( nupProgram == "mpage" ) mpage->setChecked( TRUE );
	QString printMethod = config->readEntry( "PrintMethod", "dvips" );
	if ( printMethod == "dvips" ) dvips->setChecked( TRUE );
	if ( printMethod == "dvilj4" ) dvilj4->setChecked( TRUE );
	spoolerCommand->setText( config->readEntry( "SpoolerCommand", "lpr" ) );
	
	config->setGroup( "kdvi" );
}
