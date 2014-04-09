/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003 by Christophe Devriese                             *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2003-2007 by Albert Astals Cid <aacid@kde.org>          *
 *   Copyright (C) 2004 by Andy Goossens <andygoossens@telenet.be>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "shell.h"
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <QtDBus/qdbusinterface.h>
#include <QTextStream>
#include <kwindowsystem.h>
#include "aboutdata.h"
#include "shellutils.h"

static bool attachUniqueInstance(KCmdLineArgs* args)
{
    if (!args->isSet("unique") || args->count() != 1)
        return false;

    QDBusInterface iface("org.kde.okular", "/okular", "org.kde.okular");
    QDBusInterface iface2("org.kde.okular", "/okularshell", "org.kde.okular");
    if (!iface.isValid() || !iface2.isValid())
        return false;

    if (args->isSet("print"))
	iface.call("enableStartWithPrint");
    if (args->isSet("page"))
        iface.call("openDocument", ShellUtils::urlFromArg(args->arg(0), ShellUtils::qfileExistFunc(), args->getOption("page")).url());
    else
        iface.call("openDocument", ShellUtils::urlFromArg(args->arg(0), ShellUtils::qfileExistFunc()).url());
    if (args->isSet("raise")){
	iface2.call("tryRaise");
    }

    return true;
}

// Ask an existing non-unique instance to open new tabs
static bool attachExistingInstance( KCmdLineArgs* args )
{
  if( args->count() < 1 )
    return false;

  const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();

  // Don't match the service without trailing "-" (unique instance)
  const QString pattern = "org.kde.okular-";
  const QString myPid = QString::number( kapp->applicationPid() );
  QScopedPointer<QDBusInterface> bestService;
  const int desktop = KWindowSystem::currentDesktop();

  // Select the first instance that isn't us (metric may change in future)
  foreach( const QString& service, services )
  {
    if( service.startsWith(pattern) && !service.endsWith(myPid) )
    {
      bestService.reset( new QDBusInterface(service,"/okularshell","org.kde.okular") );

      // Find a window that can handle our documents
      const QDBusReply<bool> reply = bestService->call( "canOpenDocs", args->count(), desktop );
      if( reply.isValid() && reply.value() )
        break;

      bestService.reset();
    }
  }

  if( !bestService )
    return false;

  for( int i = 0; i < args->count(); ++i )
  {
    QString arg = args->arg( i );

    // Copy stdin to temporary file which can be opened by the existing
    // window. The temp file is automatically deleted after it has been
    // opened. Not sure if this behavior is safe on all platforms.
    QScopedPointer<QTemporaryFile> tempFile;
    if( arg == "-" )
    {
      tempFile.reset( new QTemporaryFile );
      QFile stdinFile;
      if( !tempFile->open() || !stdinFile.open(stdin,QIODevice::ReadOnly) )
        return false;

      const size_t bufSize = 1024*1024;
      QScopedPointer<char,QScopedPointerArrayDeleter<char> > buf( new char[bufSize] );
      size_t bytes;
      do
      {
        bytes = stdinFile.read( buf.data(), bufSize );
        tempFile->write( buf.data(), bytes );
      } while( bytes != 0 );

      arg = tempFile->fileName();
    }

    // Returns false if it can't fit another document
    const QDBusReply<bool> reply = bestService->call( "openDocument", arg );
    if( !reply.isValid() || !reply.value() )
      return false;
  }

  bestService->call( "tryRaise" );

  return true;
}

int main(int argc, char** argv)
{
    KAboutData about = okularAboutData( "okular", I18N_NOOP( "Okular" ) );

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("p");
    options.add("page <number>", ki18n("Page of the document to be shown"));
    options.add("presentation", ki18n("Start the document in presentation mode"));
    options.add("print", ki18n("Start with print dialog"));
    options.add("unique", ki18n("\"Unique instance\" control"));
    options.add("noraise", ki18n("Not raise window"));
    options.add("+[URL]", ki18n("Document to open. Specify '-' to read from stdin."));
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;

    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        RESTORE(Shell);
    } else {
        // no session.. just start up normally
        KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

        // try to attach to existing session, unique or not
        if (attachUniqueInstance(args) || attachExistingInstance(args))
        {
            args->clear();
            return 0;
        }

        if (args->isSet( "unique" ) && args->count() > 1)
        {
            QTextStream stream(stderr);
            stream << i18n( "Error: Can't open more than one document with the --unique switch" ) << endl;
            return -1;
        }
        else
        {
          Shell* shell = new Shell( args );
          shell->show();
          for( int i = 0; i < args->count(); )
          {
            if( shell->openDocument(args->arg(i)) )
              ++i;
            else
            {
              shell = new Shell( args );
              shell->show();
            }
          }
        }
    }

    return app.exec();
}

// vim:ts=2:sw=2:tw=78:et
