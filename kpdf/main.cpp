#include "kpdf.h"
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

static const char* description =
    I18N_NOOP("A KDE KPart Application");

static const char* version = "v0.1";

static KCmdLineOptions options[] =
{
    { "+[URL]", I18N_NOOP("Document to open."), 0 },
    { 0, 0, 0 }
};

int main(int argc, char** argv)
{
  KAboutData about("kpdf", I18N_NOOP("KPDF"), version, description, KAboutData::License_GPL, "(C) 2002 Wilco Greven", 0, 0, "greven@kde.org");
  about.addAuthor("Wilco Greven", 0, "greven@kde.org");
  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;

  // see if we are starting with session management
  if (app.isRestored())
    RESTORE(KPDF)
  else
  {
    // no session.. just start up normally
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    if (args->count() == 0)
    {
      KPDF* widget = new KPDF;
      widget->show();
    }
    else
    {
      for (int i = 0; i < args->count(); ++i)
      {
        KPDF* widget = new KPDF;
        widget->show();
        widget->load(args->url(i));
      }
    }
    args->clear();
  }

  return app.exec();
}

// vim:ts=2:sw=2:tw=78:et
