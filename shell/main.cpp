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
#include <KMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "aboutdata.h"
#include "okular_main.h"
#include "shellutils.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("okular");

    KAboutData aboutData = okularAboutData();

    app.setApplicationName(aboutData.applicationData().componentName());
    app.setApplicationDisplayName(aboutData.applicationData().displayName());
    app.setApplicationVersion(aboutData.version());
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    // The KDE4 version accepted flags such as -unique with a single dash -> preserve compatibility
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("p") << QStringLiteral("page"), i18n("Page of the document to be shown"), QStringLiteral("number")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("presentation"), i18n("Start the document in presentation mode")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("print"), i18n("Start with print dialog")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("unique"), i18n("\"Unique instance\" control")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("noraise"), i18n("Not raise window")));
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

    // Make sure that if the user answered "No" and "Don't ask again"
    // he actually still has an option of answering Yes
    KMessageBox::ButtonCode result = KMessageBox::Yes;
    KMessageBox::shouldBeShownYesNo(QStringLiteral("frameworks_not_official"), result);
    if (result != KMessageBox::Yes) {
        KMessageBox::enableMessage(QStringLiteral("frameworks_not_official"));
    }

    if (KMessageBox::warningYesNo( 0, i18n("This is not an official release of Okular.\nIf you need the best possible Okular experience you should use the kdelibs4-based version instead of the version based on KDE Frameworks 5.\nIf you report bugs make sure you mark them with [frameworks] in the Summary and understand not official release bugs have less priority.\nDo you understand?"), QString(), KStandardGuiItem::yes(), KStandardGuiItem::no(), QStringLiteral("frameworks_not_official") ) != KMessageBox::Yes)
    {
        return 1;
    }

    return app.exec();
}

/* kate: replace-tabs on; indent-width 4; */
