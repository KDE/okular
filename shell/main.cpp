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
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

static const char description[] =
I18N_NOOP("okular, a Universal document viewer");

static const char version[] = "0.5";

static KCmdLineOptions options[] =
{
    { "+[URL]", I18N_NOOP("Document to open"), 0 },
    KCmdLineLastOption
};

int main(int argc, char** argv)
{
    KAboutData about(
        "okular",
        I18N_NOOP("okular"),
        version,
        description,
        KAboutData::License_GPL,
        "(C) 2002 Wilco Greven, Christophe Devriese\n(C) 2004-2005 Albert Astals Cid, Enrico Ros\n(C) 2005 Piotr Szymanski");

    about.addAuthor("Albert Astals Cid", I18N_NOOP("Current mantainer"), "astals11@terra.es");
    about.addAuthor("Piotr Szymanski", I18N_NOOP("Created okular from KPDF codebase"), "djurban@pld-dc.org");
    about.addAuthor("Enrico Ros", 0, "eros.kde@email.it");
    about.addAuthor("Wilco Greven", 0, "greven@kde.org");
    about.addAuthor("Christophe Devriese", 0, "oelewapperke@oelewapperke.org");
    about.addAuthor("Laurent Montel", 0, "montel@kde.org");

    about.addCredit("Marco Martin", I18N_NOOP("Icon"), 0, "m4rt@libero.it");

    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;

    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        RESTORE(okular::Shell);
    } else {
        // no session.. just start up normally
        KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

        if (args->count() == 0)
        {
            okular::Shell* widget = new okular::Shell;
            widget->show();
        }
        else
        {
            for (int i = 0; i < args->count(); ++i)
            {
                okular::Shell* widget = new okular::Shell(args->url(i));
                widget->show();
            }
        }
        args->clear();
    }

    return app.exec();
}

// vim:ts=2:sw=2:tw=78:et
