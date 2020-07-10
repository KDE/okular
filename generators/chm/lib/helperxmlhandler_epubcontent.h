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

#ifndef HELPERXMLHANDLER_EPUBCONTENT_H
#define HELPERXMLHANDLER_EPUBCONTENT_H

#include <QMap>
#include <QString>
#include <QXmlDefaultHandler>

class HelperXmlHandler_EpubContent : public QXmlDefaultHandler
{
public:
    HelperXmlHandler_EpubContent();

    // Keep the tag-associated metadata
    QMap<QString, QString> metadata;

    // Manifest storage, id -> href
    QMap<QString, QString> manifest;

    // Spline storage
    QList<QString> spine;

    // TOC (NCX) filename
    QString tocname;

private:
    enum State { STATE_NONE, STATE_IN_METADATA, STATE_IN_MANIFEST, STATE_IN_SPINE };

    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) override;
    bool characters(const QString &ch) override;
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) override;

    // Tracking
    State m_state;
    QString m_tagname;
};

#endif // HELPERXMLHANDLER_EPUBCONTENT_H
