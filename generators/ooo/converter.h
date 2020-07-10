/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OOO_CONVERTER_H
#define OOO_CONVERTER_H

#include <QDomDocument>
#include <QTextCharFormat>

#include <core/textdocumentgenerator.h>

#include "styleinformation.h"

class QDomElement;
class QDomText;

namespace OOO
{
class Converter : public Okular::TextDocumentConverter
{
    Q_OBJECT

public:
    Converter();
    ~Converter() override;

    Okular::Document::OpenResult convertWithPassword(const QString &fileName, const QString &password) override;

private:
    bool convertBody(const QDomElement &element);
    bool convertText(const QDomElement &element);
    bool convertHeader(QTextCursor *cursor, const QDomElement &element);
    bool convertParagraph(QTextCursor *cursor, const QDomElement &element, const QTextBlockFormat &format = QTextBlockFormat(), bool merge = false);
    bool convertTextNode(QTextCursor *cursor, const QDomText &element, const QTextCharFormat &format);
    bool convertSpan(QTextCursor *cursor, const QDomElement &element, const QTextCharFormat &format);
    bool convertLink(QTextCursor *cursor, const QDomElement &element, const QTextCharFormat &format);
    bool convertList(QTextCursor *cursor, const QDomElement &element);
    bool convertTable(const QDomElement &element);
    bool convertFrame(const QDomElement &element);
    bool convertAnnotation(QTextCursor *cursor, const QDomElement &element);

    QTextDocument *mTextDocument;
    QTextCursor *mCursor;

    StyleInformation *mStyleInformation;
};

}

#endif
