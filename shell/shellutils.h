/*
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SHELLUTILS_H
#define OKULAR_SHELLUTILS_H

#include <QString>

class QUrl;

class QCommandLineParser;

namespace ShellUtils
{
typedef bool (*FileExistFunc)(const QString &fileName);

FileExistFunc qfileExistFunc();
QUrl urlFromArg(const QString &_arg, FileExistFunc exist_func, const QString &pageArg = QString());
QString serializeOptions(const QCommandLineParser &args);
QString serializeOptions(bool startInPresentation, bool showPrintDialog, bool showPrintDialogAndExit, bool unique, bool noRaise, const QString &page, const QString &find);
bool unserializeOptions(const QString &serializedOptions, bool *presentation, bool *print, bool *print_and_exit, bool *unique, bool *noraise, QString *page);
bool unique(const QString &serializedOptions);
bool noRaise(const QString &serializedOptions);
bool startInPresentation(const QString &serializedOptions);
bool showPrintDialog(const QString &serializedOptions);
bool showPrintDialogAndExit(const QString &serializedOptions);
QString page(const QString &serializedOptions);
QString find(const QString &serializedOptions);

}

#endif
