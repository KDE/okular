/*
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SHELLUTILS_H
#define OKULAR_SHELLUTILS_H

#include "config-okular.h"

#include <QString>

#if HAVE_DBUS
#include <QLatin1StringView>
#endif

class QUrl;

class QCommandLineParser;

namespace ShellUtils
{
typedef bool (*FileExistFunc)(const QString &fileName);

FileExistFunc qfileExistFunc();
QUrl urlFromArg(const QString &_arg, FileExistFunc exist_func, const QString &pageArg = QString());
QString serializeOptions(const QCommandLineParser &args);
QString serializeOptions(bool startInPresentation, bool showPrintDialog, bool showPrintDialogAndExit, bool unique, bool noRaise, const QString &page, const QString &find, const QString &editorCmd);
bool unique(const QString &serializedOptions);
bool noRaise(const QString &serializedOptions);
bool startInPresentation(const QString &serializedOptions);
bool showPrintDialog(const QString &serializedOptions);
bool showPrintDialogAndExit(const QString &serializedOptions);
QString page(const QString &serializedOptions);
QString find(const QString &serializedOptions);
QString editorCmd(const QString &serializedOptions);

#if HAVE_DBUS
// Must be a subname of "org.kde.okular" due to Flatpak not supporting wildcard D-Bus permissions.
inline constexpr QLatin1StringView kPerProcessDbusPrefix("org.kde.okular.Instance_");

QString currentProcessDbusName();
#endif // HAVE_DBUS
}

#endif
