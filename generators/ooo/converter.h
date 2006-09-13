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

namespace OOO {

class Document;

class Converter : public QXmlDefaultHandler
{
  public:
    Converter( const Document *document );
    ~Converter();

    bool convert();

    QTextDocument *textDocument() const;
    MetaInformation::List metaInformation() const;

    virtual bool characters( const QString& );
    virtual bool startElement( const QString&, const QString&, const QString&, const QXmlAttributes& );
    virtual bool endElement( const QString&, const QString&, const QString& );

  private:
    const Document *mDocument;
    QTextDocument *mTextDocument;
    QTextCursor *mCursor;

    StyleInformation *mStyleInformation;

    bool mInParagraph;
    QTextCharFormat mFormat;
    QStack<QTextCharFormat> mSpanStack;
};

}

#endif
