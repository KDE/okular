/*
    SPDX-FileCopyrightText: 2020 Albert Astals Cid <aacid@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "xmlgui_helper.h"

#include "kxmlgui_version.h"

#if KXMLGUI_VERSION >= QT_VERSION_CHECK(5, 73, 0)
#include <KXMLGUIClient>
#endif

#include <QDebug>
#include <QFile>

#if KXMLGUI_VERSION < QT_VERSION_CHECK(5, 73, 0)

// Copied from KXmlGuiVersionHandler::findVersionNumber :/
static QString findVersionNumber(const QString &xml)
{
    enum { ST_START, ST_AFTER_OPEN, ST_AFTER_GUI, ST_EXPECT_VERSION, ST_VERSION_NUM } state = ST_START;
    const int length = xml.length();
    for (int pos = 0; pos < length; pos++) {
        switch (state) {
        case ST_START:
            if (xml[pos] == QLatin1Char('<')) {
                state = ST_AFTER_OPEN;
            }
            break;
        case ST_AFTER_OPEN: {
            // Jump to gui..
            const int guipos = xml.indexOf(QLatin1String("gui"), pos, Qt::CaseInsensitive);
            if (guipos == -1) {
                return QString(); // Reject
            }

            pos = guipos + 2; // Position at i, so we're moved ahead to the next character by the ++;
            state = ST_AFTER_GUI;
            break;
        }
        case ST_AFTER_GUI:
            state = ST_EXPECT_VERSION;
            break;
        case ST_EXPECT_VERSION: {
            const int verpos = xml.indexOf(QLatin1String("version"), pos, Qt::CaseInsensitive);
            if (verpos == -1) {
                return QString(); // Reject
            }
            pos = verpos + 7; // strlen("version") is 7
            while (xml.at(pos).isSpace()) {
                ++pos;
            }
            if (xml.at(pos++) != QLatin1Char('=')) {
                return QString(); // Reject
            }
            while (xml.at(pos).isSpace()) {
                ++pos;
            }

            state = ST_VERSION_NUM;
            break;
        }
        case ST_VERSION_NUM: {
            int endpos;
            for (endpos = pos; endpos < length; endpos++) {
                const ushort ch = xml[endpos].unicode();
                if (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) {
                    continue; // Number..
                }
                if (ch == QLatin1Char('"')) { // End of parameter
                    break;
                } else { // This shouldn't be here..
                    endpos = length;
                }
            }

            if (endpos != pos && endpos < length) {
                const QString matchCandidate = xml.mid(pos, endpos - pos); // Don't include " ".
                return matchCandidate;
            }

            state = ST_EXPECT_VERSION; // Try to match a well-formed version..
            break;
        } // case..
        } // switch
    }     // for

    return QString();
}

#endif

namespace Okular
{
void removeRCFileIfVersionSmallerThan(const QString &filePath, int version)
{
    QFile f(filePath);
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray contents = f.readAll();
        f.close();
#if KXMLGUI_VERSION < QT_VERSION_CHECK(5, 73, 0)
        const QString fileVersion = findVersionNumber(contents);
#else
        const QString fileVersion = KXMLGUIClient::findVersionNumber(contents);
#endif
        if (fileVersion.toInt() < version) {
            QFile::remove(filePath);
        }
    }
}

}
