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

#include <QRegularExpression>

namespace UrlUtils
{
QString getUrl(QString txt)
{
    // match the url
    QRegularExpression reg(QStringLiteral("\\b((https?|ftp)://(www\\d{0,3}[.])?[\\S]+)|((www\\d{0,3}[.])[\\S]+)"));
    // blocks from detecting invalid urls
    QRegularExpression reg1(QStringLiteral("[\\w'\"\\(\\)]+https?://|[\\w'\"\\(\\)]+ftp://|[\\w'\"\\(\\)]+www\\d{0,3}[.]"));
    txt = txt.remove(QLatin1Char('\n'));

    if (reg1.match(txt).hasMatch()) { // return early if there is a match (url is not valid)
        return QString();
    }

    QRegularExpressionMatch match = reg.match(txt);
    QString url = match.captured();
    if (match.hasMatch() && QUrl(url).isValid()) {
        if (url.startsWith(QLatin1String("www"))) {
            url.prepend(QLatin1String("http://"));
        }
        return url;
    }

    return QString();
}
}

#endif
