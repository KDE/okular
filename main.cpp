#include <version.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kdvi.h>
#include <kconfig.h>
#include <qfile.h>
#include <kaboutdata.h>

static const char *description = 
	I18N_NOOP("KDE DVI viewer");

static KCmdLineOptions options[] =
{
    { "paper <paper_size>", I18N_NOOP("Ignored :-)"), 0 },
    { "+[file]", I18N_NOOP("DVI File to open"), 0 },
    { 0, 0, 0 }
};

int main( int argc, char **argv )
{
	KAboutData aboutData( "kdvi", I18N_NOOP("KDVI"), 
		KDVI_VERSION, description, KAboutData::GPL, 
		"(c) 1999-2000, The Various KDVI and KDE Developers");
	aboutData.addAuthor("Markku Hihnala",0, "mah@ee.oulu.fi");
	aboutData.addAuthor("Bernd Johannes Wuebben",0, "wuebben@math.cornell.edu");
	aboutData.addAuthor("Robert Williams",0, "rwilliams@jrcmaui.com");
	aboutData.addAuthor("Eric Cooper");
	aboutData.addAuthor("Bob Scheifler");
	aboutData.addAuthor("Paal Kvamme");
	aboutData.addAuthor("H\\aa vard Eidnes");
	aboutData.addAuthor("Mark Eichin");
	aboutData.addAuthor("Paul Vojta");
	aboutData.addAuthor("Jeffrey Lee");
	aboutData.addAuthor("Donald Richardson");
	
	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

	KApplication a;
	kdvi *k;

	if ( a.isRestored() && KTMainWindow::canBeRestored( 1 ) )
	{
		k = new kdvi();
		k->restore( 1 );
	}
	else
        {
                KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
		if (args->count())
			k = new kdvi(QFile::decodeName(args->arg(0)));
                else
			k = new kdvi();
		args->clear();
        }

	a.setMainWidget( k );
	k->show();
	
	kapp->config()->setGroup("kdvi");

	return a.exec();
}
