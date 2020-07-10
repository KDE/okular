/***************************************************************************
 *   Copyright (C) 2004-2007 by Georgy Yunaev, gyunaev@ulduzsoft.com       *
 *   Please do not use email address above for bug reports; see            *
 *   the README file                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QDir>
#include <QRegularExpression>
#include <QString>

namespace LCHMUrlFactory
{
static inline bool isRemoteURL(const QString &url, QString &protocol)
{
    // Check whether the URL is external
    QRegularExpression uriregex(QStringLiteral("^(\\w+):\\/\\/"));
    QRegularExpressionMatch match;

    // mailto: can also have different format, so handle it
    if (url.startsWith(QLatin1String("mailto:"))) {
        protocol = QStringLiteral("mailto");
        return true;
    } else if ((match = uriregex.match(url)).hasMatch()) {
        const QString proto = match.captured(1).toLower();

        // Filter the URLs which need to be opened by a browser
        if (proto == QLatin1String("http") || proto == QLatin1String("ftp") || proto == QLatin1String("mailto") || proto == QLatin1String("news")) {
            protocol = proto;
            return true;
        }
    }

    return false;
}

// Some JS urls start with javascript://
static inline bool isJavascriptURL(const QString &url)
{
    return url.startsWith(QLatin1String("javascript://"));
}

// Parse urls like "ms-its:file name.chm::/topic.htm"
static inline bool isNewChmURL(const QString &url, QString &chmfile, QString &page)
{
    QRegularExpression uriregex(QStringLiteral("^ms-its:(.*)::(.*)$"));
    uriregex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = uriregex.match(url);
    if (match.hasMatch()) {
        chmfile = match.captured(1);
        page = match.captured(2);

        return true;
    }

    return false;
}

static inline QString makeURLabsoluteIfNeeded(const QString &url)
{
    QString p1, p2, newurl = url;

    if (!isRemoteURL(url, p1) && !isJavascriptURL(url) && !isNewChmURL(url, p1, p2)) {
        newurl = QDir::cleanPath(url);

        // Normalize url, so it becomes absolute
        if (newurl[0] != QLatin1Char('/'))
            newurl = QLatin1Char('/') + newurl;
    }

    // qDebug ("makeURLabsolute (%s) -> (%s)", url.ascii(), newurl.ascii());
    return newurl;
}

// Returns a special string, which allows the kio-slave, or viewwindow-browser interaction
// to recognize our own internal urls, which is necessary to show image-only pages.
static inline QString getInternalUriExtension()
{
    return QStringLiteral(".KCHMVIEWER_SPECIAL_HANDLER");
}

static inline bool handleFileType(const QString &link, QString &generated)
{
    QString intext = getInternalUriExtension();

    if (!link.endsWith(intext))
        return false;

    QString filelink = link.left(link.length() - intext.length());
    generated = QStringLiteral("<html><body><img src=\"") + filelink + QStringLiteral("\"></body></html>");
    return true;
}

}
