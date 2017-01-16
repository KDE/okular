/***************************************************************************
 *   Copyright (C) 2013 Jaydeep Solanki <jaydp17@gmail.com>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef URL_UTILS_H
#define URL_UTILS_H

#include <QRegExp>

namespace UrlUtils
{
    QString getUrl( QString txt )
    {
        // match the url
        QRegExp reg( QStringLiteral( "\\b((https?|ftp)://(www\\d{0,3}[.])?[\\S]+)|((www\\d{0,3}[.])[\\S]+)" ) );
        // blocks from detecting invalid urls
        QRegExp reg1( QStringLiteral( "[\\w'\"\\(\\)]+https?://|[\\w'\"\\(\\)]+ftp://|[\\w'\"\\(\\)]+www\\d{0,3}[.]" ) );
        txt = txt.remove( QStringLiteral("\n") );
        if( reg1.indexIn( txt ) < 0 && reg.indexIn( txt ) >= 0 && QUrl( reg.cap() ).isValid() )
        {
            QString url = reg.cap();
            if( url.startsWith( QLatin1String("www") ) )
                url.prepend( QLatin1String("http://") );
            return url;
        }
        else
            return QString();
    }
}

#endif
