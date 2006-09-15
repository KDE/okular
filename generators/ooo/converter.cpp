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
    mStyleInformation( new StyleInformation ), mInParagraph( false ),
    mInHeader( false )
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
  mTextDocument->setPageSize( QSize( qRound( property.width() ), qRound( property.height() ) ) );

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
  if ( !mInParagraph && !mInHeader )
    return true;

  if ( !ch.isEmpty() )
    mCursor->insertText( ch, mTextFormat );

  return true;
}

bool Converter::startElement( const QString &namespaceUri, const QString &localName, const QString &qName,
                             const QXmlAttributes &attributes )
{
  if ( localName == QLatin1String( "p" ) ) {
    mInParagraph = true;

    const QString styleName = attributes.value( "text:style-name" );
    const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );

    mBlockFormat = QTextBlockFormat();
    mTextFormat = QTextCharFormat();

    property.apply( &mBlockFormat, &mTextFormat );
    mCursor->insertBlock();
    mCursor->setBlockFormat( mBlockFormat );
  }

  if ( localName == QLatin1String( "h" ) ) {
    mInHeader = true;

    const QString styleName = attributes.value( "text:style-name" );
    const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );

    mBlockFormat = QTextBlockFormat();
    mTextFormat = QTextCharFormat();

    property.apply( &mBlockFormat, &mTextFormat );
    mCursor->insertBlock();
    mCursor->setBlockFormat( mBlockFormat );
  }

  if ( mInParagraph && localName == QLatin1String( "span" ) ) {
    mSpanStack.push( QPair<QTextBlockFormat, QTextCharFormat>( mBlockFormat, mTextFormat ) );

    mBlockFormat = QTextBlockFormat();
    mTextFormat = QTextCharFormat();

    const QString styleName = attributes.value( "text:style-name" );
    const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );
    property.apply( &mBlockFormat, &mTextFormat );
  }

  return true;
}

bool Converter::endElement( const QString &namespaceUri, const QString &localName, const QString &qName )
{
  if ( localName == QLatin1String( "p" ) ) {
    mInParagraph = false;
  }

  if ( localName == QLatin1String( "h" ) ) {
    mInHeader = false;
  }

  if ( mInParagraph && localName == QLatin1String( "span" ) ) {
    const QPair<QTextBlockFormat, QTextCharFormat> formats = mSpanStack.pop();
    mBlockFormat = formats.first;
    mTextFormat = formats.second;
  }

  return true;
}
