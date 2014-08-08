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
#include "aboutdata.h"
#include "okular_main.h"
#include "shellutils.h"

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
    }
    else
    {
        // no session.. just start up normally
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
        QStringList paths;
        for ( int i = 0; i < args->count(); ++i )
            paths << args->arg(i);
        Okular::Status status = Okular::main(paths, ShellUtils::serializeOptions(*args));
        switch (status)
        {
            case Okular::Error:
                return -1;
            case Okular::AttachedOtherProcess:
                return 0;
            case Okular::Success:
                // Do nothing
                break;
        }
    }

    return app.exec();
}

/* kate: replace-tabs on; indent-width 4; */
