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

#ifndef HELPER_ENTITYDECODER_H
#define HELPER_ENTITYDECODER_H

#include <QMap>
#include <QString>

//
// This helper class decodes the Unicode HTML entities into the Unicode characters
//
class HelperEntityDecoder
{
public:
    // Initialization with the specific decoder
    HelperEntityDecoder(QTextCodec *encoder = nullptr);

    // Used when the encoding changes
    void changeEncoding(QTextCodec *encoder = nullptr);

    // The decoder function
    QString decode(const QString &entity) const;

private:
    // Map to decode HTML entitles like &acute; based on current encoding, initialized upon the first use
    QMap<QString, QString> m_entityDecodeMap;
};

#endif // HELPER_ENTITYDECODER_H
