#include <version.h>
#include <kapp.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kdvi.h>
#include <kconfig.h>
#include <qfile.h>

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
	KCmdLineArgs::init( argc, argv, "kdvi", description, KDVI_VERSION);

	KCmdLineArgs::addCmdLineOptions( options );

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
