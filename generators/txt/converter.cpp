/***************************************************************************
 *   Copyright (C) 2013 by Azat Khuzhin <a3at.mail@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include <QtGui/QTextFrame>

#include "converter.h"
#include "document.h"

using namespace Txt;

Converter::Converter() : m_textDocument(NULL)
{
}

Converter::~Converter()
{
}

QTextDocument* Converter::convert( const QString &fileName )
{
    m_textDocument = new Document( fileName );

    m_textDocument->setPageSize(QSizeF( 600, 800 ));

    QTextFrameFormat frameFormat;
    frameFormat.setMargin( 20 );

    QTextFrame *rootFrame = m_textDocument->rootFrame();
    rootFrame->setFrameFormat( frameFormat );

    emit addMetaData( Okular::DocumentInfo::MimeType, "text/plain" );

    return m_textDocument;
}