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

#include <KLocalizedString>
#include <QtDBus/qdbusinterface.h>
#include <QTextStream>
#include <kwindowsystem.h>
#include <QApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "aboutdata.h"
#include "okular_main.h"
#include "shellutils.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    KAboutData aboutData = okularAboutData();

    app.setApplicationName(aboutData.applicationData().componentName());
    app.setApplicationDisplayName(aboutData.applicationData().displayName());
    app.setApplicationVersion(aboutData.version());
    app.setOrganizationDomain("kde.org");

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("p") << QLatin1String("page"), i18n("Page of the document to be shown"), QLatin1String("number")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("presentation"), i18n("Start the document in presentation mode")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("print"), i18n("Start with print dialog")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("unique"), i18n("\"Unique instance\" control")));
    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("noraise"), i18n("Not raise window")));
    parser.addPositionalArgument(QStringLiteral("urls"), i18n("Documents to open. Specify '-' to read from stdin."));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        RESTORE(Shell);
    }
    else
    {
        // no session.. just start up normally
        QStringList paths;
        for ( int i = 0; i < parser.positionalArguments().count(); ++i )
            paths << parser.positionalArguments().at(i);
        Okular::Status status = Okular::main(paths, ShellUtils::serializeOptions(parser));
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
