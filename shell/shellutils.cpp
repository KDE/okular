/***************************************************************************
 *   Copyright (C) 2009 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "shellutils.h"

// qt/kde includes
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QtCore/qcommandlineparser.h>

namespace ShellUtils
{

namespace detail
{

bool qfileExistFunc( const QString& fileName )
{
    return QFile::exists( fileName );
}

}

FileExistFunc qfileExistFunc()
{
    return detail::qfileExistFunc;
}

QUrl urlFromArg( const QString& _arg, FileExistFunc exist_func, const QString& pageArg )
{
    // ## TODO remove exist_func
#if QT_VERSION >= 0x050400
    QUrl url = QUrl::fromUserInput(_arg, QDir::currentPath(), QUrl::AssumeLocalFile);
#else
    // Code from QUrl::fromUserInput(QString, QString)
    QUrl url = QUrl::fromUserInput(_arg);
    QUrl testUrl = QUrl(_arg, QUrl::TolerantMode);
    if (testUrl.isRelative() && !QDir::isAbsolutePath(_arg)) {
        QFileInfo fileInfo(QDir::current(), _arg);
        if (fileInfo.exists())
            url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
    }
#endif
    if ( url.isLocalFile() ) {
        // make sure something like /tmp/foo#bar.pdf is treated as a path name (default)
        // but something like /tmp/foo.pdf#bar is foo.pdf plus an anchor "bar"
        const QString path = url.path();
        int hashIndex = path.lastIndexOf( QLatin1Char ( '#' ) );
        int lastDotIndex = path.lastIndexOf( QLatin1Char ( '.' ) );
        // make sure that we don't change the path if .pdf comes after the #
        if ( hashIndex != -1 && hashIndex > lastDotIndex) {
            url.setPath( path.left( hashIndex ) );
            url.setFragment( path.mid( hashIndex + 1 ) );
            qDebug() << "Added fragment to url:" << url.path() << url.fragment();
        }
    } else if ( !url.fragment().isEmpty() ) {
        // make sure something like http://example.org/foo#bar.pdf is treated as a path name
        // but something like http://example.org/foo.pdf#bar is foo.pdf plus an anchor "bar"
        if ( url.fragment().contains( QLatin1Char( '.' ) ) ) {
            url.setPath( url.path() + QLatin1Char ( '#' ) + url.fragment() );
            url.setFragment( QString() );
        }
    }
    if ( !pageArg.isEmpty() )
    {
      url.setFragment( pageArg );
    }
    return url;
}

QString serializeOptions(const QCommandLineParser &args)
{
    const bool startInPresentation = args.isSet( QStringLiteral("presentation") );
    const bool showPrintDialog = args.isSet( QStringLiteral("print") );
    const bool unique = args.isSet(QStringLiteral("unique")) && args.positionalArguments().count() <= 1;
    const bool noRaise = !args.isSet(QStringLiteral("raise"));
    const QString page = args.value(QStringLiteral("page"));

    return serializeOptions(startInPresentation, showPrintDialog, unique, noRaise, page);
}

QString serializeOptions(bool startInPresentation, bool showPrintDialog, bool unique, bool noRaise, const QString &page)
{
    return QStringLiteral("%1:%2:%3:%4:%5").arg(startInPresentation).arg(showPrintDialog).arg(unique).arg(noRaise).arg(page);
}

bool unserializeOptions(const QString &serializedOptions, bool *presentation, bool *print, bool *unique, bool *noraise, QString *page)
{
    const QStringList args = serializedOptions.split(QStringLiteral(":"));
    if (args.count() == 5)
    {
        *presentation = args[0] == QLatin1String("1");
        *print = args[1] == QLatin1String("1");
        *unique = args[2] == QLatin1String("1");
        *noraise = args[3] == QLatin1String("1");
        *page = args[4];
        return true;
    }
    return false;
}

bool startInPresentation(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &result, &dummy, &dummy, &dummy, &dummyString) && result;
}

bool showPrintDialog(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &dummy, &result, &dummy, &dummy, &dummyString) && result;
}

bool unique(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &dummy, &dummy, &result, &dummy, &dummyString) && result;
}

bool noRaise(const QString &serializedOptions)
{
    bool result, dummy;
    QString dummyString;
    return unserializeOptions(serializedOptions, &dummy, &dummy, &dummy, &result, &dummyString) && result;
}

QString page(const QString &serializedOptions)
{
    QString result;
    bool dummy;
    unserializeOptions(serializedOptions, &dummy, &dummy, &dummy, &dummy, &result);
    return result;
}

}
