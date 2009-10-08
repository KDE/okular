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

}
