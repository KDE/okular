/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2003 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2003-2007 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2004 Andy Goossens <andygoossens@telenet.be>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "shell.h"

#include "aboutdata.h"
#include "okular_main.h"
#include "shellutils.h"
#include <KAboutData>
#include <KCrash>
#include <KIconTheme>
#include <KLocalizedString>
#include <KMessageBox>
#include <KWindowSystem>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QStringList>
#include <QTextStream>
#include <QtGlobal>

#define HAVE_STYLE_MANAGER __has_include(<KStyleManager>)
#if HAVE_STYLE_MANAGER
#include <KStyleManager>
#endif

int main(int argc, char **argv)
{
    /**
     * trigger initialisation of proper icon theme
     */
#if KICONTHEMES_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    KIconTheme::initTheme();
#endif

    QCoreApplication::setAttribute(Qt::AA_CompressTabletEvents);

    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("okular");

#if HAVE_STYLE_MANAGER
    /**
     * trigger initialisation of proper application style
     */
    KStyleManager::initStyle();
#else
    /**
     * For Windows and macOS: use Breeze if available
     * Of all tested styles that works the best for us
     */
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    QApplication::setStyle(QStringLiteral("breeze"));
#endif
#endif

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
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("editor-cmd"), i18n("Sets the external editor command"), QStringLiteral("string")));
    parser.addPositionalArgument(QStringLiteral("urls"), i18n("Documents to open. Specify '-' to read from stdin."));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // see if we are starting with session management
    if (app.isSessionRestored()) {
        kRestoreMainWindows<Shell>();
    } else {
        // no session.. just start up normally
        QStringList paths;
        for (int i = 0; i < parser.positionalArguments().count(); ++i) {
            paths << parser.positionalArguments().at(i);
        }
        Okular::Status status = Okular::main(paths, ShellUtils::serializeOptions(parser));
        switch (status) {
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
