#include <kapp.h>
#include <kdvi.h>
#include <kconfig.h>

static void getOptions( int& ac, char** av )
{
	if ( ac > 2 && av[1] == QString("-paper") )
		av[1] = av[3], ac -= 2;
}

int main( int argc, char **argv )
{
	KApplication a( argc, argv, "kdvi" );
	kdvi *k;

	getOptions( argc, argv );
	
	if ( a.isRestored() && KTMainWindow::canBeRestored( 1 ) )
	{
		k = new kdvi();
		k->restore( 1 );
	}
	else
		k = argc==2 ? new kdvi(argv[1]) : new kdvi();

	a.setMainWidget( k );
	k->show();
	
	kapp->config()->setGroup("kdvi");

	return a.exec();
}
