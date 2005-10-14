#include <config.h>

#include <dcopclient.h>
#include <dcopref.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kurl.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <qdir.h>

#include <stdlib.h>

#include "kviewshell.h"


static KCmdLineOptions options[] =
{
  { "u", 0, 0 },
  { "unique", I18N_NOOP("Check if the file is loaded in another KDVI.\nIf it is, bring up the other KDVI. Otherwise, load the file."), 0 },
  { "g", 0, 0 },
  { "goto <pagenumber>", I18N_NOOP("Navigate to this page"), 0 },
  { "+file(s)", I18N_NOOP("Files to load"), 0 },
  KCmdLineLastOption
};


static const char description[] = I18N_NOOP("A previewer for Device Independent files (DVI files) produced by the TeX typesetting system.");


int main(int argc, char** argv)
{
  KAboutData about ("kdvi", I18N_NOOP("KDVI"), "1.4",
                    description, KAboutData::License_GPL,
                    "Markku Hinhala, Stephan Kebekus",
                    I18N_NOOP("This program displays Device Independent (DVI) files which are produced by the TeX typesetting system.\n"
                    "This KDVI version is based on original code from KDVI version 0.43 and xdvik."));

  about.addAuthor ("Stefan Kebekus",
                   I18N_NOOP("Current Maintainer."),
                   "kebekus@kde.org",
                   "http://www.mi.uni-koeln.de/~kebekus");

  about.addAuthor ("Markku Hinhala", I18N_NOOP("Author of kdvi 0.4.3"));
  about.addAuthor ("Nicolai Langfeldt", I18N_NOOP("Maintainer of xdvik"));
  about.addAuthor ("Paul Vojta", I18N_NOOP("Author of xdvi"));
  about.addCredit ("Philipp Lehmann", I18N_NOOP("Testing and bug reporting."));
  about.addCredit ("Wilfried Huss", I18N_NOOP("Re-organisation of source code."));

  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions(options);
  KApplication app;

  // see if we are starting with session management
  if (app.isRestored())
  {
    RESTORE(KViewShell);
  }
  else
  {
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    if (args->isSet("unique"))
    {
      // With --unique, we need 2 arguments.
      if (args->count() < 1) 
      {
        args->usage();
        exit(-1);
      }

      // Find the fully qualified file name of the file we are
      // loading. Complain, if we are given a URL which does not point
      // to a local file.
      KURL url(args->url(0));

      if (!url.isValid()) 
      {
        kdError(4300) << QString(I18N_NOOP("The URL %1 is not well-formed.")).arg(args->arg(0)) << endl;
        return -1;
      }

      if (!url.isLocalFile()) 
      {
        kdError(4300) << QString(I18N_NOOP("The URL %1 does not point to a local file. You can only specify local "
             "files if you are using the '--unique' option.")).arg(args->arg(0)) << endl;
        return -1;
      }

      QString qualPath = QFileInfo(url.path()).absFilePath();

      app.dcopClient()->attach();
      // We need to register as "kviewshell" to stay compatible with existing DCOP-skripts.
      QCString id = app.dcopClient()->registerAs("unique-kviewshell");
      if (id.isNull())
        kdError(4300) << "There was an error using dcopClient()->registerAs()." << endl;
      QCStringList apps = app.dcopClient()->registeredApplications();
      for ( QCStringList::Iterator it = apps.begin(); it != apps.end(); ++it ) 
      {
        if ((*it).find("kviewshell") == 0) 
        {
          QByteArray data, replyData;
          QCString replyType;
          QDataStream arg(data, IO_WriteOnly);
          bool result;
          arg << qualPath.stripWhiteSpace();
          if (!app.dcopClient()->call( *it, "kmultipage", "is_file_loaded(QString)", data, replyType, replyData))
            kdError(4300) << "There was an error using DCOP." << endl;
          else 
          {
            QDataStream reply(replyData, IO_ReadOnly);
            if (replyType == "bool") 
            {
              reply >> result;
              if (result == true) 
              {
                if (app.dcopClient()->send( *it, "kmultipage", "jumpToReference(QString)", url.ref()) == true)
                {
                  app.dcopClient()->detach();
                  return 0;
                }
              }
            }
            else
              kdError(4300) << "The DCOP function 'doIt' returned an unexpected type of reply!";
          }
        }
      }
    }

    // We need to register as "kviewshell" to stay compatible with existing DCOP-skripts.
    app.dcopClient()->registerAs("kviewshell");
    KViewShell* shell = new KViewShell("application/x-dvi");
    shell->show();
    app.processEvents();

    if (args->count() > 0)
    {
      KURL url = args->url(0);
      if (!url.hasRef() && args->isSet("goto"))
      {
        // If the url doesn't already has a reference part, add the
        // argument of --goto to the url as reference, to make the
        // KViewShell jump to this page.
        QString reference = args->getOption("goto");
        url.setHTMLRef(reference);
      }
      shell->openURL(url);
    }
  }

  return app.exec();
}
