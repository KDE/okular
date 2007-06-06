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
#include <klocale.h>
#include "aboutdata.h"

static KCmdLineOptions options[] =
{
    { "p", 0, 0 },
    { "page <number>", I18N_NOOP("Page of the document to be shown"), 0 },
    { "presentation", I18N_NOOP("Start the document in presentation mode"), 0 },
    { "+[URL]", I18N_NOOP("Document to open"), 0 },
    KCmdLineLastOption
};

int main(int argc, char** argv)
{
    KAboutData * about = okularAboutData( "okular", I18N_NOOP( "okular" ) );

    KCmdLineArgs::init(argc, argv, about);
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;

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
        args->clear();
    }
    int ret = app.exec();
    delete about;
    return ret;
}

// vim:ts=2:sw=2:tw=78:et
