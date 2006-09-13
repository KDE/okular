/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>

#include "converter.h"
#include "document.h"
#include "styleinformation.h"
#include "styleparser.h"

using namespace OOO;

Converter::Converter( const Document *document )
  : mDocument( document ), mTextDocument( 0 ), mCursor( 0 ),
    mStyleInformation( new StyleInformation ), mInParagraph( false )
{
}

Converter::~Converter()
{
  delete mStyleInformation;
  mStyleInformation = 0;
}

bool Converter::convert()
{
  delete mTextDocument;
  delete mCursor;

  mTextDocument = new QTextDocument;
  mCursor = new QTextCursor( mTextDocument );

  StyleParser styleParser( mDocument, mStyleInformation );
  if ( !styleParser.parse() )
    return false;

  const QString masterLayout = mStyleInformation->masterLayout( "Standard" );
  const PageFormatProperty property = mStyleInformation->pageProperty( masterLayout );
  mTextDocument->setPageSize( QSize( property.width(), property.height() ) );

  QXmlSimpleReader reader;
  reader.setContentHandler( this );

  QXmlInputSource source;
  source.setData( mDocument->content() );

  return reader.parse( &source, true );
}

QTextDocument *Converter::textDocument() const
{
  return mTextDocument;
}

MetaInformation::List Converter::metaInformation() const
{
  return mStyleInformation->metaInformation();
}

bool Converter::characters( const QString &ch )
{
  if ( !mInParagraph )
    return true;

  if ( !ch.isEmpty() )
    mCursor->insertText( ch, mFormat );

  return true;
}

bool Converter::startElement( const QString &namespaceUri, const QString &localName, const QString &qName,
                             const QXmlAttributes &attributes )
{
  if ( localName == QLatin1String( "p" ) ) {
    mInParagraph = true;

    const QString styleName = attributes.value( "text:style-name" );
    const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );

    property.apply( &mFormat );
  }

  if ( mInParagraph && localName == QLatin1String( "span" ) ) {
    mSpanStack.push( mFormat );
    mFormat = QTextCharFormat();

    const QString styleName = attributes.value( "text:style-name" );
    const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );
    property.apply( &mFormat );
  }

  return true;
}

bool Converter::endElement( const QString &namespaceUri, const QString &localName, const QString &qName )
{
  if ( localName == QLatin1String( "p" ) ) {
    mInParagraph = false;
    mCursor->insertText( "\n" );
  }

  if ( mInParagraph && localName == QLatin1String( "span" ) ) {
    mFormat = mSpanStack.pop();
  }

  return true;
}
