/***************************************************************************
 *   Copyright (C) 2013 by Azat Khuzhin <a3at.mail@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "converter.h"

#include <QTextFrame>

#include "document.h"

using namespace Txt;

Converter::Converter()
{
}

Converter::~Converter()
{
}

QTextDocument *Converter::convert(const QString &fileName)
{
    Document *textDocument = new Document(fileName);

    textDocument->setPageSize(QSizeF(600, 800));

    QTextFrameFormat frameFormat;
    frameFormat.setMargin(20);

    QTextFrame *rootFrame = textDocument->rootFrame();
    rootFrame->setFrameFormat(frameFormat);

    emit addMetaData(Okular::DocumentInfo::MimeType, QStringLiteral("text/plain"));

    return textDocument;
}
