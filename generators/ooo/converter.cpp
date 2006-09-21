/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QDebug>

#include <QtCore/QUrl>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtGui/QTextFrame>
#include <QtGui/QTextList>
#include <QtGui/QTextTableCell>
#include <QtXml/QDomElement>
#include <QtXml/QDomText>
#include <QtXml/QXmlSimpleReader>

#include "converter.h"
#include "document.h"
#include "styleinformation.h"
#include "styleparser.h"

#include "core/document.h"

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
  mTableOfContents = QDomDocument( "DocumentSynopsis" );
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

  mTableOfContents.clear();
  mHeaderInfos.clear();
  mInternalLinkInfos.clear();
  mLinkInfos.clear();

  mTextDocument = new QTextDocument;
  mCursor = new QTextCursor( mTextDocument );

  /**
   * Read the style properties, so the are available when
   * parsing the content.
   */
  StyleParser styleParser( mDocument, mStyleInformation );
  if ( !styleParser.parse() )
    return false;

  /**
   * Add all images of the document to resource framework
   */
  const QMap<QString, QByteArray> images = mDocument->images();
  QMapIterator<QString, QByteArray> it( images );
  while ( it.hasNext() ) {
    it.next();

    mTextDocument->addResource( QTextDocument::ImageResource, QUrl( it.key() ), QImage::fromData( it.value() ) );
  }

  /**
   * Set the correct page size
   */
  const QString masterLayout = mStyleInformation->masterLayout( "Standard" );
  const PageFormatProperty property = mStyleInformation->pageProperty( masterLayout );
  mTextDocument->setPageSize( QSize( qRound( property.width() ), qRound( property.height() ) ) );

  QTextFrameFormat frameFormat;
  frameFormat.setMargin( qRound( property.margin() ) );

  QTextFrame *rootFrame = mTextDocument->rootFrame();
  rootFrame->setFrameFormat( frameFormat );

  /**
   * Parse the content of the document
   */
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

  /**
   * Create table of contents
   */
  if ( !createTableOfContents() )
    return false;

  /**
   * Create list of links
   */
  if ( !createLinksList() )
    return false;

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
    } else if ( child.tagName() == QLatin1String( "table" ) ) {
      if ( !convertTable( child ) )
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
  property.applyBlock( &blockFormat );
  property.applyText( &textFormat );

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

  HeaderInfo headerInfo;
  headerInfo.block = cursor->block();
  headerInfo.text = element.text();
  headerInfo.level = element.attribute( "outline-level" ).toInt();

  mHeaderInfos.append( headerInfo );

  return true;
}

bool Converter::convertParagraph( QTextCursor *cursor, const QDomElement &element, const QTextBlockFormat &parentFormat )
{
  const QString styleName = element.attribute( "style-name" );
  const StyleFormatProperty property = mStyleInformation->styleProperty( styleName );

  QTextBlockFormat blockFormat( parentFormat );
  QTextCharFormat textFormat;
  property.applyBlock( &blockFormat );
  property.applyText( &textFormat );

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
      } else if ( childElement.tagName() == QLatin1String( "a" ) ) {
        if ( !convertLink( cursor, childElement, textFormat ) )
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

  QTextCharFormat textFormat( format );
  property.applyText( &textFormat );

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
      if ( paragraphElement.tagName() != QLatin1String( "p" ) ) {
        child = child.nextSiblingElement();
        continue;
      }

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
  /**
   * Find out dimension of the table
   */
  QDomElement rowElement = element.firstChildElement();

  int rowCounter = 0;
  int columnCounter = 0;
  while ( !rowElement.isNull() ) {
    if ( rowElement.tagName() == QLatin1String( "table-row" ) ) {
      rowCounter++;

      int counter = 0;
      QDomElement columnElement = rowElement.firstChildElement();
      while ( !columnElement.isNull() ) {
        if ( columnElement.tagName() == QLatin1String( "table-cell" ) ) {
          counter++;
        }
        columnElement = columnElement.nextSiblingElement();
      }

      columnCounter = qMax( columnCounter, counter );
    }

    rowElement = rowElement.nextSiblingElement();
  }

  /**
   * Create table
   */
  QTextTable *table = mCursor->insertTable( rowCounter, columnCounter );

  /**
   * Fill table
   */
  rowElement = element.firstChildElement();

  QTextTableFormat tableFormat;

  rowCounter = 0;
  while ( !rowElement.isNull() ) {
    if ( rowElement.tagName() == QLatin1String( "table-row" ) ) {

      int columnCounter = 0;
      QDomElement columnElement = rowElement.firstChildElement();
      while ( !columnElement.isNull() ) {
        if ( columnElement.tagName() == QLatin1String( "table-cell" ) ) {
          const StyleFormatProperty property = mStyleInformation->styleProperty( columnElement.attribute( "style-name" ) );

          QTextBlockFormat format;
          property.applyTableCell( &format );

          QDomElement paragraphElement = columnElement.firstChildElement();
          while ( !paragraphElement.isNull() ) {
            if ( paragraphElement.tagName() == QLatin1String( "p" ) ) {
              QTextTableCell cell = table->cellAt( rowCounter, columnCounter );
              QTextCursor cursor = cell.firstCursorPosition();
              cursor.setBlockFormat( format );

              if ( !convertParagraph( &cursor, paragraphElement, format ) )
                return false;
            }

            paragraphElement = paragraphElement.nextSiblingElement();
          }
          columnCounter++;
        }
        columnElement = columnElement.nextSiblingElement();
      }

      rowCounter++;
    }

    if ( rowElement.tagName() == QLatin1String( "table-column" ) ) {
      const StyleFormatProperty property = mStyleInformation->styleProperty( rowElement.attribute( "style-name" ) );
      property.applyTableColumn( &tableFormat );
    }

    rowElement = rowElement.nextSiblingElement();
  }

  table->setFormat( tableFormat );

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

bool Converter::convertLink( QTextCursor *cursor, const QDomElement &element, const QTextCharFormat &format )
{
  QDomNode child = element.firstChild();
  while ( !child.isNull() ) {
    if ( child.isElement() ) {
      const QDomElement childElement = child.toElement();
      if ( childElement.tagName() == QLatin1String( "span" ) ) {
        if ( !convertSpan( cursor, childElement, format ) )
          return false;
      }
    } else if ( child.isText() ) {
      const QDomText childText = child.toText();
      if ( !convertTextNode( cursor, childText, format ) )
        return false;
    }

    child = child.nextSibling();
  }

  InternalLinkInfo linkInfo;
  linkInfo.block = cursor->block();
  linkInfo.url = element.attribute( "href" );

  mInternalLinkInfos.append( linkInfo );

  return true;
}

bool Converter::createTableOfContents()
{
  const QSizeF pageSize = mTextDocument->pageSize();

  for ( int i = 0; i < mHeaderInfos.count(); ++i ) {
    const HeaderInfo headerInfo = mHeaderInfos[ i ];

    const QRectF rect = mTextDocument->documentLayout()->blockBoundingRect( headerInfo.block );

    int page = qRound( rect.y() ) / qRound( pageSize.height() );
    int offset = qRound( rect.y() ) % qRound( pageSize.height() );

    DocumentViewport viewport( page );
    viewport.rePos.normalizedX = (double)rect.x() / (double)pageSize.width();
    viewport.rePos.normalizedY = (double)offset / (double)pageSize.height();
    viewport.rePos.enabled = true;
    viewport.rePos.pos = DocumentViewport::Center;

    QStack<QDomNode> parentNodeStack;
    QDomNode parentNode = mTableOfContents;
    int level = 2;

    QDomElement item = mTableOfContents.createElement( headerInfo.text );
    item.setAttribute( "Viewport", viewport.toString() );

    int newLevel = headerInfo.level;
    if ( newLevel == level ) {
      parentNode.appendChild( item );
    } else if ( newLevel > level ) {
      parentNodeStack.push( parentNode );
      parentNode = parentNode.lastChildElement();
      parentNode.appendChild( item );
      level++;
    } else {
      for ( int i = level; i > newLevel; i-- ) {
        level--;
        parentNode = parentNodeStack.pop();
      }

      parentNode.appendChild( item );
    }
  }

  return true;
}

bool Converter::createLinksList()
{
  const QSizeF pageSize = mTextDocument->pageSize();

  for ( int i = 0; i < mInternalLinkInfos.count(); ++i ) {
    const InternalLinkInfo internalLinkInfo = mInternalLinkInfos[ i ];

    const QRectF rect = mTextDocument->documentLayout()->blockBoundingRect( internalLinkInfo.block );

    int page = qRound( rect.y() ) / qRound( pageSize.height() );
    int offset = qRound( rect.y() ) % qRound( pageSize.height() );

    double x = (double)rect.x() / (double)pageSize.width();
    double y = (double)offset / (double)pageSize.height();
    double width = (double)rect.width() / (double)pageSize.width();
    double height = (double)rect.height() / (double)pageSize.height();

    LinkInfo linkInfo;
    linkInfo.page = page;
    linkInfo.boundingRect = QRectF( rect.x(), offset, rect.width(), rect.height() );
    linkInfo.url = internalLinkInfo.url;

    mLinkInfos.append( linkInfo );
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

QDomDocument Converter::tableOfContents() const
{
  return mTableOfContents;
}

Converter::LinkInfo::List Converter::links() const
{
  return mLinkInfos;
}
