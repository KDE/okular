/*
 *  Kchmviewer - a CHM and EPUB file viewer with broad language support
 *  Copyright (C) 2004-2014 George Yunaev, gyunaev@ulduzsoft.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "helperxmlhandler_epubtoc.h"
#include <QtDebug>

HelperXmlHandler_EpubTOC::HelperXmlHandler_EpubTOC(EBook_EPUB *epub)
{
    m_epub = epub;
    m_inNavMap = false;
    m_inText = false;
    m_indent = 0;
}

bool HelperXmlHandler_EpubTOC::startElement(const QString &, const QString &localName, const QString &, const QXmlAttributes &atts)
{
    //	qDebug() << "startElement " << " " << localName;

    //	for ( int i = 0; i < atts.count(); i++ )
    //		qDebug() << "    " << atts.localName(i) << " " << atts.value(i);

    if (localName == QLatin1String("navMap")) {
        m_inNavMap = true;
        return true;
    }

    if (!m_inNavMap)
        return true;

    if (localName == QLatin1String("navPoint"))
        m_indent++;

    if (localName == QLatin1String("text"))
        m_inText = true;

    if (localName == QLatin1String("content")) {
        int idx = atts.index(QLatin1String("src"));

        if (idx == -1)
            return false;

        m_lastId = atts.value(idx);
        checkNewTocEntry();
    }

    return true;
}

bool HelperXmlHandler_EpubTOC::characters(const QString &ch)
{
    //	qDebug() << "characters" << " " << ch;
    if (m_inText)
        m_lastTitle = ch;

    checkNewTocEntry();
    return true;
}

bool HelperXmlHandler_EpubTOC::endElement(const QString &, const QString &localName, const QString &)
{
    //	qDebug() << "endElement" << " " << qName;

    if (localName == QLatin1String("navMap")) {
        m_inNavMap = false;
        return true;
    }

    if (localName == QLatin1String("navPoint"))
        m_indent--;

    if (localName == QLatin1String("text"))
        m_inText = false;

    return true;
}

void HelperXmlHandler_EpubTOC::checkNewTocEntry()
{
    if (!m_lastId.isEmpty() && !m_lastTitle.isEmpty()) {
        EBookTocEntry entry;
        entry.name = m_lastTitle;
        entry.url = m_epub->pathToUrl(m_lastId);
        entry.iconid = EBookTocEntry::IMAGE_AUTO;
        entry.indent = m_indent - 1;

        entries.push_back(entry);

        // qDebug() << "TOC entry: " << m_lastId << " :" << m_lastTitle << " :" << m_indent - 1;

        m_lastId.clear();
        m_lastTitle.clear();
    }
}
