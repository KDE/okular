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
#include <qfile.h>
#include <qregexp.h>
#include <kcmdlineargs.h>

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

KUrl urlFromArg( const QString& _arg, FileExistFunc exist_func, const QString& pageArg )
{
    /*
     Rationale for the small "cut-and-paste" work being done below:
     KCmdLineArgs::makeURL() (used by ::url() encodes any # into the URL itself,
     so we have to find it manually and build up the URL by taking its ref,
     if any.
     */
    QString arg = _arg;
    arg.replace( QRegExp( "^file:/{1,3}"), "/" );
    if ( arg != _arg )
    {
        arg = QString::fromUtf8( QByteArray::fromPercentEncoding( arg.toUtf8() ) );
    }
    KUrl url = KCmdLineArgs::makeURL( arg.toUtf8() );
    int sharpPos = -1;
    if ( !url.isLocalFile() || !exist_func( url.toLocalFile() ) )
    {
        sharpPos = arg.lastIndexOf( QLatin1Char( '#' ) );
    }
    if ( sharpPos != -1 )
    {
      url = KCmdLineArgs::makeURL( arg.left( sharpPos ).toUtf8() );
      url.setHTMLRef( arg.mid( sharpPos + 1 ) );
    }
    else if ( !pageArg.isEmpty() )
    {
      url.setHTMLRef( pageArg );
    }
    return url;
}

QString serializeOptions(const KCmdLineArgs &args)
{
    const bool startInPresentation = args.isSet( "presentation" );
    const bool showPrintDialog = args.isSet( "print" );
    const bool unique = args.isSet("unique") && args.count() <= 1;
    const bool noRaise = !args.isSet("raise");
    const QString page = args.getOption("page");

    return serializeOptions(startInPresentation, showPrintDialog, unique, noRaise, page);
}

QString serializeOptions(bool startInPresentation, bool showPrintDialog, bool unique, bool noRaise, const QString &page)
{
    return QString("%1:%2:%3:%4:%5").arg(startInPresentation).arg(showPrintDialog).arg(unique).arg(noRaise).arg(page);
}

bool unserializeOptions(const QString &serializedOptions, bool *presentation, bool *print, bool *unique, bool *noraise, QString *page)
{
    const QStringList args = serializedOptions.split(":");
    if (args.count() == 5)
    {
        *presentation = args[0] == "1";
        *print = args[1] == "1";
        *unique = args[2] == "1";
        *noraise = args[3] == "1";
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
