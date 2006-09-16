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

#include <QtCore/QStack>
#include <QtGui/QTextCharFormat>
#include <QtXml/QXmlDefaultHandler>

#include "styleinformation.h"

class QDomElement;
class QDomText;

namespace OOO {

class Document;

class Style
{
  public:
    Style( const QTextBlockFormat &blockFormat, const QTextCharFormat &textFormat );

    QTextBlockFormat blockFormat() const;
    QTextCharFormat textFormat() const;

  private:
    QTextBlockFormat mBlockFormat;
    QTextCharFormat mTextFormat;
};

class Converter
{
  public:
    Converter( const Document *document );
    ~Converter();

    bool convert();

    QTextDocument *textDocument() const;
    MetaInformation::List metaInformation() const;

    bool convertBody( const QDomElement &element );
    bool convertText( const QDomElement &element );
    bool convertHeader( QTextCursor *cursor, const QDomElement &element );
    bool convertParagraph( QTextCursor *cursor, const QDomElement &element );
    bool convertTextNode( QTextCursor *cursor, const QDomText &element, const QTextCharFormat &format );
    bool convertSpan( QTextCursor *cursor, const QDomElement &element, const QTextCharFormat &format );
    bool convertList( const QDomElement &element );
    bool convertTable( const QDomElement &element );
    bool convertFrame( const QDomElement &element );

  private:
    const Document *mDocument;
    QTextDocument *mTextDocument;
    QTextCursor *mCursor;
    QTextBlock *mLastTextBlock;

    StyleInformation *mStyleInformation;
};

}

#endif
