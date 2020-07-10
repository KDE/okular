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

#ifndef HELPERXMLHANDLER_EPUBTOC_H
#define HELPERXMLHANDLER_EPUBTOC_H

#include "ebook_epub.h"
#include <QXmlDefaultHandler>

class HelperXmlHandler_EpubTOC : public QXmlDefaultHandler
{
public:
    HelperXmlHandler_EpubTOC(EBook_EPUB *epub);

    QList<EBookTocEntry> entries;

private:
    // Overridden members
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) override;
    bool characters(const QString &ch) override;
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) override;
    void checkNewTocEntry();

    bool m_inNavMap;
    bool m_inText;
    unsigned int m_indent;
    QString m_lastId;
    QString m_lastTitle;
    EBook_EPUB *m_epub;
};

#endif // HELPERXMLHANDLER_EPUBTOC_H
