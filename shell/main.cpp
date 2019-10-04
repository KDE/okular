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
#include <QDBusInterface>
#include <QTextStream>
#include <kwindowsystem.h>
#include <QApplication>
#include <QtGlobal>
#include <KAboutData>
#include <KCrash>
#include <KMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "aboutdata.h"
#include "okular_main.h"
#include "shellutils.h"

int main(int argc, char** argv)
{
    /**
     * enable high dpi support
     */
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents);
#endif

    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("okular");

    KAboutData aboutData = okularAboutData();
    KAboutData::setApplicationData(aboutData);
    // set icon for shells which do not use desktop file metadata
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("okular")));

    KCrash::initialize();

    QCommandLineParser parser;
    // The KDE4 version accepted flags such as -unique with a single dash -> preserve compatibility
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    aboutData.setupCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("p") << QStringLiteral("page"), i18n("Page of the document to be shown"), QStringLiteral("number")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("presentation"), i18n("Start the document in presentation mode")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("print"), i18n("Start with print dialog")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("print-and-exit"), i18n("Start with print dialog and exit after printing")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("unique"), i18n("\"Unique instance\" control")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("noraise"), i18n("Not raise window")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("find"), i18n("Find a string on the text"), QStringLiteral("string")));
    parser.addPositionalArgument(QStringLiteral("urls"), i18n("Documents to open. Specify '-' to read from stdin."));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // see if we are starting with session management
    if (app.isSessionRestored())
    {
        kRestoreMainWindows<Shell>();
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
