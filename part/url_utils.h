/*
    SPDX-FileCopyrightText: 2013 Jaydeep Solanki <jaydp17@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
