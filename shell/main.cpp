/***************************************************************************
 *   Copyright (C) 2002 by Wilco Greven <greven@kde.org>                   *
 *   Copyright (C) 2003 by Christophe Devriese                             *
 *                         <Christophe.Devriese@student.kuleuven.ac.be>    *
 *   Copyright (C) 2003 by Laurent Montel <montel@kde.org>                 *
 *   Copyright (C) 2003-2006 by Albert Astals Cid <tsdgeos@terra.es>       *
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
#include <kicon.h>
#include <klocale.h>
#include "aboutdata.h"

int main(int argc, char** argv)
{
    KAboutData about = okularAboutData( "okular", I18N_NOOP( "okular" ) );

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("p");
    options.add("page <number>", ki18n("Page of the document to be shown"));
    options.add("presentation", ki18n("Start the document in presentation mode"));
    options.add("+[URL]", ki18n("Document to open"));
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;
    QApplication::setWindowIcon( KIcon( "graphics-viewer-document" ) );

    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        RESTORE(Shell);
    } else {
        // no session.. just start up normally
        KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

        if (args->count() == 0)
        {
            Shell* widget = new Shell(args);
            widget->show();
        }
        else
        {
            for (int i = 0; i < args->count(); ++i)
            {
                Shell* widget = new Shell(args, args->url(i));
                widget->show();
            }
        }
    }

    return app.exec();
}

// vim:ts=2:sw=2:tw=78:et
