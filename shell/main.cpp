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

        // try to attach the "unique" session: if we succeed, do nothing more and exit
        if (attachUniqueInstance(args))
        {
            args->clear();
            return 0;
        }

        if (args->count() == 0)
        {
            Shell* widget = new Shell(args);
            widget->show();
        }
        else if (args->isSet( "unique" ) && args->count() > 1)
        {
            QTextStream stream(stderr);
            stream << i18n( "Error: Can't open more than one document with the --unique switch" ) << endl;
            return -1;
        }
        else
        {
            for (int i = 0; i < args->count(); ++i)
            {
                Shell* widget = new Shell(args, i);
                widget->show();
            }
        }
    }

    return app.exec();
}

// vim:ts=2:sw=2:tw=78:et
