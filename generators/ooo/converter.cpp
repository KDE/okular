/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QUrl>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtGui/QTextFrame>
#include <QtGui/QTextList>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomText>

#include "converter.h"
#include "document.h"
#include "styleinformation.h"
#include "styleparser.h"

using namespace OOO;

Style::Style( const QTextBlockFormat &blockFormat, const QTextCharFormat &textFormat )
  : mBlockFormat( blockFormat ), mTextFormat( textFormat )
{
}

QTextBlockFormat Style::blockFormat() const
{
  return mBlockFormat;
}

QTextCharFormat Style::textFormat() const
{
  return mTextFormat;
}

Converter::Converter( const Document *document )
  : mDocument( document ), mTextDocument( 0 ), mCursor( 0 ),
    mStyleInformation( new StyleInformation )
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

  // add images to resource framework
  const QMap<QString, QByteArray> images = mDocument->images();
  QMapIterator<QString, QByteArray> it( images );
  while ( it.hasNext() ) {
    it.next();

    mTextDocument->addResource( QTextDocument::ImageResource, QUrl( it.key() ), QImage::fromData( it.value() ) );
  }

  const QString masterLayout = mStyleInformation->masterLayout( "Standard" );
  const PageFormatProperty property = mStyleInformation->pageProperty( masterLayout );
  mTextDocument->setPageSize( QSize( qRound( property.width() ), qRound( property.height() ) ) );

  QTextFrameFormat frameFormat;
  frameFormat.setMargin( qRound( property.margin() ) );

  QTextFrame *rootFrame = mTextDocument->rootFrame();
  rootFrame->setFrameFormat( frameFormat );

  QXmlSimpleReader reader;

  QXmlInputSource source;
  source.setData( mDocument->content() );

  QString errorMsg;
  int errorLine, errorCol;

  QDomDocument document;
  if ( !document.setContent( &source, &reader, &errorMsg, &errorLine, &errorCol ) ) {
    qDebug( "%s at (%d,%d)", qPrintable( errorMsg ), errorLine, errorCol );
    return false;
  }

  const QDomElement documentElement = document.documentElement();

  QDomElement element = documentElement.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "body" ) ) {
      if ( !convertBody( element ) )
        return false;
    }

    element = element.nextSiblingElement();
  }

  return true;
}

bool Converter::convertBody( const QDomElement &element )
{
  QDomElement child = element.firstChildElement();
  while ( !child.isNull() ) {
    if ( child.tagName() == QLatin1String( "text" ) ) {
      if ( !convertText( child ) )
        return false;
    }

    child = child.nextSiblingElement();
  }

  return true;
}

bool Converter::convertText( const QDomElement &element )
{
  QDomElement child = element.firstChildElement();
  while ( !child.isNull() ) {
    if ( child.tagName() == QLatin1String( "p" ) ) {
      mCursor->insertBlock();
      if ( !convertParagraph( mCursor, child ) )
        return false;
    } else if ( child.tagName() == QLatin1String( "h" ) ) {
      mCursor->insertBlock();
      if ( !convertHeader( mCursor, child ) )
        return false;
    } else if ( child.tagName() == QLatin1String( "list" ) ) {
      if ( !convertList( child ) )
        return false;
    }

    child = child.nextSiblingElement();
  }

  return true;
}

bool Converter::convertHeader( QTextCursor *cursor, const QDomElement &element )
{
  const QString styleName = element.attribute( "style-name" );
  const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );

  QTextBlockFormat blockFormat;
  QTextCharFormat textFormat;
  property.apply( &blockFormat, &textFormat );

  cursor->setBlockFormat( blockFormat );

  QDomNode child = element.firstChild();
  while ( !child.isNull() ) {
    if ( child.isElement() ) {
      const QDomElement childElement = child.toElement();
      if ( childElement.tagName() == QLatin1String( "span" ) ) {
        if ( !convertSpan( cursor, childElement, textFormat ) )
          return false;
      }
    } else if ( child.isText() ) {
      const QDomText childText = child.toText();
      if ( !convertTextNode( cursor, childText, textFormat ) )
        return false;
    }

    child = child.nextSibling();
  }

  return true;
}

bool Converter::convertParagraph( QTextCursor *cursor, const QDomElement &element )
{
  const QString styleName = element.attribute( "style-name" );
  const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );

  QTextBlockFormat blockFormat;
  QTextCharFormat textFormat;
  property.apply( &blockFormat, &textFormat );

  cursor->setBlockFormat( blockFormat );

  QDomNode child = element.firstChild();
  while ( !child.isNull() ) {
    if ( child.isElement() ) {
      const QDomElement childElement = child.toElement();
      if ( childElement.tagName() == QLatin1String( "span" ) ) {
        if ( !convertSpan( cursor, childElement, textFormat ) )
          return false;
      } else if ( childElement.tagName() == QLatin1String( "tab" ) ) {
        mCursor->insertText( "    " );
      } else if ( childElement.tagName() == QLatin1String( "s" ) ) {
        QString spaces;
        spaces.fill( ' ', childElement.attribute( "c" ).toInt() );
        mCursor->insertText( spaces );
      } else if ( childElement.tagName() == QLatin1String( "frame" ) ) {
        if ( !convertFrame( childElement ) )
          return false;
      }
    } else if ( child.isText() ) {
      const QDomText childText = child.toText();
      if ( !convertTextNode( cursor, childText, textFormat ) )
        return false;
    }

    child = child.nextSibling();
  }

  return true;
}

bool Converter::convertTextNode( QTextCursor *cursor, const QDomText &element, const QTextCharFormat &format )
{
  cursor->insertText( element.data(), format );

  return true;
}

bool Converter::convertSpan( QTextCursor *cursor, const QDomElement &element, const QTextCharFormat &format )
{
  const QString styleName = element.attribute( "style-name" );
  const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );

  QTextBlockFormat blockFormat;
  QTextCharFormat textFormat = format;
  property.apply( &blockFormat, &textFormat );

  QDomNode child = element.firstChild();
  while ( !child.isNull() ) {
    if ( child.isText() ) {
      const QDomText childText = child.toText();
      if ( !convertTextNode( cursor, childText, textFormat ) )
        return false;
    }

    child = child.nextSibling();
  }

  return true;
}

bool Converter::convertList( const QDomElement &element )
{
  const QString styleName = element.attribute( "style-name" );
  const ListFormatProperty property = mStyleInformation->listProperty( styleName );

  QTextListFormat format;
  property.apply( &format, 0 );

  QTextList *list = mCursor->insertList( format );

  QDomElement child = element.firstChildElement();
  int loop = 0;
  while ( !child.isNull() ) {
    if ( child.tagName() == QLatin1String( "list-item" ) ) {
      loop++;

      const QDomElement paragraphElement = child.firstChildElement();
      if ( paragraphElement.tagName() != QLatin1String( "p" ) )
        continue;

        // FIXME: as soon as Qt is fixed
//      if ( loop > 1 )
        mCursor->insertBlock();

      if ( !convertParagraph( mCursor, paragraphElement ) )
        return false;

//      if ( loop > 1 )
        list->add( mCursor->block() );
    }

    child = child.nextSiblingElement();
  }

  return true;
}

bool Converter::convertTable( const QDomElement &element )
{
  return true;
}

bool Converter::convertFrame( const QDomElement &element )
{
  QDomElement child = element.firstChildElement();
  while ( !child.isNull() ) {
    if ( child.tagName() == QLatin1String( "image" ) ) {
      const QString href = child.attribute( "href" );
      QTextImageFormat format;
      format.setWidth( StyleParser::convertUnit( element.attribute( "width" ) ) );
      format.setHeight( StyleParser::convertUnit( element.attribute( "height" ) ) );
      format.setName( href );

      mCursor->insertImage( format );
    }

    child = child.nextSiblingElement();
  }

  return true;
}

QTextDocument *Converter::textDocument() const
{
  return mTextDocument;
}

MetaInformation::List Converter::metaInformation() const
{
  return mStyleInformation->metaInformation();
}
