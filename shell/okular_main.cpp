/*
    SPDX-FileCopyrightText: 2002 Wilco Greven <greven@kde.org>
    SPDX-FileCopyrightText: 2003 Christophe Devriese <Christophe.Devriese@student.kuleuven.ac.be>
    SPDX-FileCopyrightText: 2003 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2003-2007 Albert Astals Cid <aacid@kde.org>
    SPDX-FileCopyrightText: 2004 Andy Goossens <andygoossens@telenet.be>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "okular_main.h"

#include "aboutdata.h"
#include "shell.h"
#include "shellutils.h"
#include <KLocalizedString>
#include <KWindowSystem>
#include <QApplication>
#include <QDBusInterface>
#include <QTextStream>

static bool attachUniqueInstance(const QStringList &paths, const QString &serializedOptions)
{
    if (!ShellUtils::unique(serializedOptions) || paths.count() != 1)
        return false;

    QDBusInterface iface(QStringLiteral("org.kde.okular"), QStringLiteral("/okularshell"), QStringLiteral("org.kde.okular"));
    if (!iface.isValid())
        return false;

    const QString page = ShellUtils::page(serializedOptions);
    iface.call(QStringLiteral("openDocument"), ShellUtils::urlFromArg(paths[0], ShellUtils::qfileExistFunc(), page).url(), serializedOptions);
    if (!ShellUtils::noRaise(serializedOptions)) {
        iface.call(QStringLiteral("tryRaise"));
    }

    return true;
}

// Ask an existing non-unique instance to open new tabs
static bool attachExistingInstance(const QStringList &paths, const QString &serializedOptions)
{
    if (paths.count() < 1)
        return false;

    // Don't try to attach to an existing instance with --print-and-exit because that would mean
    // we're going to exit that other instance and that's just rude
    if (ShellUtils::showPrintDialogAndExit(serializedOptions))
        return false;

    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();

    // Don't match the service without trailing "-" (unique instance)
    const QString pattern = QStringLiteral("org.kde.okular-");
    const QString myPid = QString::number(qApp->applicationPid());
    QScopedPointer<QDBusInterface> bestService;
    const int desktop = KWindowSystem::currentDesktop();

    // Select the first instance that isn't us (metric may change in future)
    for (const QString &service : services) {
        if (service.startsWith(pattern) && !service.endsWith(myPid)) {
            bestService.reset(new QDBusInterface(service, QStringLiteral("/okularshell"), QStringLiteral("org.kde.okular")));

            // Find a window that can handle our documents
            const QDBusReply<bool> reply = bestService->call(QStringLiteral("canOpenDocs"), paths.count(), desktop);
            if (reply.isValid() && reply.value())
                break;

            bestService.reset();
        }
    }

    if (!bestService)
        return false;

    for (const QString &arg : paths) {
        // Copy stdin to temporary file which can be opened by the existing
        // window. The temp file is automatically deleted after it has been
        // opened. Not sure if this behavior is safe on all platforms.
        QScopedPointer<QTemporaryFile> tempFile;
        QString path;
        if (arg == QLatin1String("-")) {
            tempFile.reset(new QTemporaryFile);
            QFile stdinFile;
            if (!tempFile->open() || !stdinFile.open(stdin, QIODevice::ReadOnly))
                return false;

            const size_t bufSize = 1024 * 1024;
            QScopedPointer<char, QScopedPointerArrayDeleter<char>> buf(new char[bufSize]);
            size_t bytes;
            do {
                bytes = stdinFile.read(buf.data(), bufSize);
                tempFile->write(buf.data(), bytes);
            } while (bytes != 0);

            path = tempFile->fileName();
        } else {
            // Page only makes sense if we are opening one file
            const QString page = ShellUtils::page(serializedOptions);
            path = ShellUtils::urlFromArg(arg, ShellUtils::qfileExistFunc(), page).url();
        }

        // Returns false if it can't fit another document
        const QDBusReply<bool> reply = bestService->call(QStringLiteral("openDocument"), path, serializedOptions);
        if (!reply.isValid() || !reply.value())
            return false;
    }

    bestService->call(QStringLiteral("tryRaise"));

    return true;
}

namespace Okular
{
Status main(const QStringList &paths, const QString &serializedOptions)
{
    if (ShellUtils::unique(serializedOptions) && paths.count() > 1) {
        QTextStream stream(stderr);
        stream << i18n("Error: Can't open more than one document with the --unique switch") << endl;
        return Error;
    }

    if (ShellUtils::startInPresentation(serializedOptions) && paths.count() > 1) {
        QTextStream stream(stderr);
        stream << i18n("Error: Can't open more than one document with the --presentation switch") << endl;
        return Error;
    }

    if (ShellUtils::showPrintDialog(serializedOptions) && paths.count() > 1) {
        QTextStream stream(stderr);
        stream << i18n("Error: Can't open more than one document with the --print switch") << endl;
        return Error;
    }

    if (!ShellUtils::page(serializedOptions).isEmpty() && paths.count() > 1) {
        QTextStream stream(stderr);
        stream << i18n("Error: Can't open more than one document with the --page switch") << endl;
        return Error;
    }

    if (!ShellUtils::find(serializedOptions).isEmpty() && paths.count() > 1) {
        QTextStream stream(stderr);
        stream << i18n("Error: Can't open more than one document with the --find switch") << endl;
        return Error;
    }
    // try to attach to existing session, unique or not
    if (attachUniqueInstance(paths, serializedOptions) || attachExistingInstance(paths, serializedOptions)) {
        return AttachedOtherProcess;
    }

    Shell *shell = new Shell(serializedOptions);
    if (!shell->isValid()) {
        return Error;
    }

    shell->show();
    for (int i = 0; i < paths.count();) {
        // Page only makes sense if we are opening one file
        const QString page = ShellUtils::page(serializedOptions);
        const QUrl url = ShellUtils::urlFromArg(paths[i], ShellUtils::qfileExistFunc(), page);
        if (shell->openDocument(url, serializedOptions)) {
            ++i;
        } else {
            shell = new Shell(serializedOptions);
            if (!shell->isValid()) {
                return Error;
            }
            shell->show();
        }
    }

    return Success;
}

}

/* kate: replace-tabs on; indent-width 4; */
