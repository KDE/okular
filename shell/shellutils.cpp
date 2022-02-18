/*
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "shellutils.h"

// qt/kde includes
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace ShellUtils
{
namespace detail
{
bool qfileExistFunc(const QString &fileName)
{
    return QFile::exists(fileName);
}

}

FileExistFunc qfileExistFunc()
{
    return detail::qfileExistFunc;
}

QUrl urlFromArg(const QString &_arg, FileExistFunc exist_func, const QString &pageArg)
{
    QUrl url = QUrl::fromUserInput(_arg, QDir::currentPath(), QUrl::AssumeLocalFile);
    if (url.isLocalFile()) {
        // make sure something like /tmp/foo#bar.pdf is treated as a path name (default)
        // but something like /tmp/foo.pdf#bar is foo.pdf plus an anchor "bar"
        const QString path = url.path();
        int hashIndex = path.lastIndexOf(QLatin1Char('#'));
        if (hashIndex != -1 && !exist_func(path)) {
            url.setPath(path.left(hashIndex));
            url.setFragment(path.mid(hashIndex + 1));
        }
    }
    if (!pageArg.isEmpty()) {
        url.setFragment(pageArg);
    }
    return url;
}

QString serializeOptions(const QCommandLineParser &args)
{
    const bool startInPresentation = args.isSet(QStringLiteral("presentation"));
    const bool showPrintDialog = args.isSet(QStringLiteral("print"));
    const bool showPrintDialogAndExit = args.isSet(QStringLiteral("print-and-exit"));
    const bool unique = args.isSet(QStringLiteral("unique")) && args.positionalArguments().count() <= 1;
    const bool noRaise = args.isSet(QStringLiteral("noraise"));
    const QString page = args.value(QStringLiteral("page"));
    const QString find = args.value(QStringLiteral("find"));
    const QString editorCmd = args.value(QStringLiteral("editor-cmd"));

    return serializeOptions(startInPresentation, showPrintDialog, showPrintDialogAndExit, unique, noRaise, page, find, editorCmd);
}

QString serializeOptions(bool startInPresentation, bool showPrintDialog, bool showPrintDialogAndExit, bool unique, bool noRaise, const QString &page, const QString &find, const QString &editorCmd)
{
    return QStringLiteral("%1:%2:%3:%4:%5:%6:%7:%8")
        .arg(startInPresentation)
        .arg(showPrintDialog)
        .arg(showPrintDialogAndExit)
        .arg(unique)
        .arg(noRaise)
        .arg(page, QString::fromLatin1(find.toUtf8().toBase64()), QString::fromLatin1(editorCmd.toUtf8().toBase64()));
}

static bool unserializeOptions(const QString &serializedOptions, bool *presentation, bool *print, bool *print_and_exit, bool *unique, bool *noraise, QString *page, QString *find, QString *editorCmd)
{
    const QStringList args = serializedOptions.split(QStringLiteral(":"));

    if (args.count() >= 8) {
        *presentation = args[0] == QLatin1String("1");
        *print = args[1] == QLatin1String("1");
        *print_and_exit = args[2] == QLatin1String("1");
        *unique = args[3] == QLatin1String("1");
        *noraise = args[4] == QLatin1String("1");
        *page = args[5];
        *find = args[6];
        *editorCmd = args[7];
        return true;
    }
    return false;
}

bool startInPresentation(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &result, &dummy, &dummy, &dummy, &dummy, &dummyString, &dummyString, &dummyString) && result;
}

bool showPrintDialog(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &dummy, &result, &dummy, &dummy, &dummy, &dummyString, &dummyString, &dummyString) && result;
}

bool showPrintDialogAndExit(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &dummy, &dummy, &result, &dummy, &dummy, &dummyString, &dummyString, &dummyString) && result;
}

bool unique(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &dummy, &dummy, &dummy, &result, &dummy, &dummyString, &dummyString, &dummyString) && result;
}

bool noRaise(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &dummy, &dummy, &dummy, &dummy, &result, &dummyString, &dummyString, &dummyString) && result;
}

QString page(const QString &serializedOptions)
{
    QString result, dummyString;
    bool dummy;
    unserializeOptions(serializedOptions, &dummy, &dummy, &dummy, &dummy, &dummy, &result, &dummyString, &dummyString);
    return result;
}

QString find(const QString &serializedOptions)
{
    QString result, dummyString;
    bool dummy;
    unserializeOptions(serializedOptions, &dummy, &dummy, &dummy, &dummy, &dummy, &dummyString, &result, &dummyString);
    return QString::fromUtf8(QByteArray::fromBase64(result.toLatin1()));
}

QString editorCmd(const QString &serializedOptions)
{
    QString result, dummyString;
    bool dummy;
    unserializeOptions(serializedOptions, &dummy, &dummy, &dummy, &dummy, &dummy, &dummyString, &dummyString, &result);
    return QString::fromUtf8(QByteArray::fromBase64(result.toLatin1()));
}
}
