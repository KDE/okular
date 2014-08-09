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


#include <klocale.h>
#include <QtDBus/qdbusinterface.h>
#include <QTextStream>
#include <kwindowsystem.h>
#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "aboutdata.h"
#include "shellutils.h"

#include <kapplication.h>

static bool attachUniqueInstance(QCommandLineParser* args)
{
    if (!args->isSet("unique") || args->positionalArguments().count() != 1)
        return false;

    QDBusInterface iface("org.kde.okular", "/okular", "org.kde.okular");
    QDBusInterface iface2("org.kde.okular", "/okularshell", "org.kde.okular");
    if (!iface.isValid() || !iface2.isValid())
        return false;

    if (args->isSet("print"))
        iface.call("enableStartWithPrint");
    if (args->isSet("page"))
        iface.call("openDocument", ShellUtils::urlFromArg(args->positionalArguments().at(0), ShellUtils::qfileExistFunc(), args->value("page")).url());
    else
        iface.call("openDocument", ShellUtils::urlFromArg(args->positionalArguments().at(0), ShellUtils::qfileExistFunc()).url());
    if (args->isSet("raise")) {
        iface2.call("tryRaise");
    }

    return true;
}

// Ask an existing non-unique instance to open new tabs
static bool attachExistingInstance( QCommandLineParser* args )
{
    if ( args->positionalArguments().count() < 1 )
        return false;

    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();

    // Don't match the service without trailing "-" (unique instance)
    const QString pattern = "org.kde.okular-";
    const QString myPid = QString::number( kapp->applicationPid() );
    QScopedPointer<QDBusInterface> bestService;
    const int desktop = KWindowSystem::currentDesktop();

    // Select the first instance that isn't us (metric may change in future)
    foreach ( const QString& service, services )
    {
        if ( service.startsWith(pattern) && !service.endsWith( myPid ) )
        {
            bestService.reset( new QDBusInterface(service, "/okularshell", "org.kde.okular") );

            // Find a window that can handle our documents
            const QDBusReply<bool> reply = bestService->call( "canOpenDocs", args->positionalArguments().count(), desktop );
            if( reply.isValid() && reply.value() )
                break;

            bestService.reset();
        }
    }

    if ( !bestService )
        return false;

    foreach ( QString arg, args->positionalArguments() )
    {
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

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    //PORTING SCRIPT: adapt aboutdata variable if necessary
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("p") << QLatin1String("page"), i18n("Page of the document to be shown"), QLatin1String("number")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("presentation"), i18n("Start the document in presentation mode")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("print"), i18n("Start with print dialog")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("unique"), i18n("\"Unique instance\" control")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("noraise"), i18n("Not raise window")));
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("+[URL]"), i18n("Document to open. Specify '-' to read from stdin.")));

    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        RESTORE(Shell);
    } else {
        // no session.. just start up normally

        // try to attach to existing session, unique or not
        if (attachUniqueInstance(&parser) || attachExistingInstance(&parser))
        {
            
            return 0;
        }

        if (parser.isSet( "unique" ) && parser.positionalArguments().count() > 1)
        {
            QTextStream stream(stderr);
            stream << i18n( "Error: Can't open more than one document with the --unique switch" ) << endl;
            return -1;
        }
        else
        {
            Shell* shell = new Shell( &parser );
            shell->show();
            for ( int i = 0; i < parser.positionalArguments().count(); )
            {
                if ( shell->openDocument( parser.positionalArguments().at(i)) )
                    ++i;
                else
                {
                    shell = new Shell( &parser );
                    shell->show();
                }
            }
        }
    }

    return app.exec();
}

/* kate: replace-tabs on; indent-width 4; */
