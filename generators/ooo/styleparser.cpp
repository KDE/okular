/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QDateTime>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QXmlSimpleReader>

#include <kglobal.h>
#include <klocale.h>

#include "document.h"
#include "styleinformation.h"
#include "styleparser.h"

using namespace OOO;

StyleParser::StyleParser( const Document *document, StyleInformation *styleInformation )
  : mDocument( document ), mStyleInformation( styleInformation )
{
}

bool StyleParser::parse()
{
  if ( !parseContentFile() )
    return false;

  if ( !parseStyleFile() )
    return false;

  if ( !parseMetaFile() )
    return false;

  return true;
}

bool StyleParser::parseContentFile()
{
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
    if ( element.tagName() == QLatin1String( "document-common-attrs" ) ) {
      if ( !parseDocumentCommonAttrs( element ) )
        return false;
    } else if ( element.tagName() == QLatin1String( "font-face-decls" ) ) {
      if ( !parseFontFaceDecls( element ) )
        return false;
    } else if ( element.tagName() == QLatin1String( "styles" ) ) {
      if ( !parseStyles( element ) )
        return false;
    } else if ( element.tagName() == QLatin1String( "automatic-styles" ) ) {
      if ( !parseAutomaticStyles( element ) )
        return false;
    }

    element = element.nextSiblingElement();
  }

  return true;
}

bool StyleParser::parseStyleFile()
{
  QXmlSimpleReader reader;

  QXmlInputSource source;
  source.setData( mDocument->styles() );

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
    if ( element.tagName() == QLatin1String( "styles" ) ) {
      if ( !parseAutomaticStyles( element ) )
        return false;
    } else if ( element.tagName() == QLatin1String( "automatic-styles" ) ) {
      if ( !parseAutomaticStyles( element ) )
        return false;
    } else if ( element.tagName() == QLatin1String( "master-styles" ) ) {
      if ( !parseMasterStyles( element ) )
        return false;
    }

    element = element.nextSiblingElement();
  }

  return true;
}

bool StyleParser::parseMetaFile()
{
  QXmlSimpleReader reader;

  QXmlInputSource source;
  source.setData( mDocument->meta() );

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
    if ( element.tagName() == QLatin1String( "meta" ) ) {
      QDomElement child = element.firstChildElement();
      while ( !child.isNull() ) {
        if ( child.tagName() == QLatin1String( "generator" ) ) {
          mStyleInformation->addMetaInformation( "producer", child.text(), i18n( "Producer" ) );
        } else if ( child.tagName() == QLatin1String( "creation-date" ) ) {
          const QDateTime dateTime = QDateTime::fromString( child.text(), Qt::ISODate );
          mStyleInformation->addMetaInformation( "creationDate", KGlobal::locale()->formatDateTime( dateTime, false, true ),
                                                 i18n( "Created" ) );
        } else if ( child.tagName() == QLatin1String( "initial-creator" ) ) {
          mStyleInformation->addMetaInformation( "creator", child.text(), i18n( "Creator" ) );
        } else if ( child.tagName() == QLatin1String( "creator" ) ) {
          mStyleInformation->addMetaInformation( "author", child.text(), i18n( "Author" ) );
        } else if ( child.tagName() == QLatin1String( "date" ) ) {
          const QDateTime dateTime = QDateTime::fromString( child.text(), Qt::ISODate );
          mStyleInformation->addMetaInformation( "modificationDate", KGlobal::locale()->formatDateTime( dateTime, false, true ),
                                                 i18n( "Modified" ) );
        }

        child = child.nextSiblingElement();
      }
    }

    element = element.nextSiblingElement();
  }

  return true;
}
bool StyleParser::parseDocumentCommonAttrs( QDomElement& )
{
  return true;
}

bool StyleParser::parseFontFaceDecls( QDomElement &parent )
{
  QDomElement element = parent.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "font-face" ) ) {
      FontFormatProperty property;
      property.setFamily( element.attribute( "font-family" ) );

      mStyleInformation->addFontProperty( element.attribute( "name" ), property );
    } else {
      qDebug( "unknown tag %s", qPrintable( element.tagName() ) );
    }

    element = element.nextSiblingElement();
  }

  return true;
}

bool StyleParser::parseStyles( QDomElement& )
{
  return true;
}

bool StyleParser::parseMasterStyles( QDomElement &parent )
{
  QDomElement element = parent.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "master-page" ) ) {
      mStyleInformation->addMasterLayout( element.attribute( "name" ), element.attribute( "page-layout-name" ) );
    } else {
      qDebug( "unknown tag %s", qPrintable( element.tagName() ) );
    }

    element = element.nextSiblingElement();
  }

  return true;
}

bool StyleParser::parseAutomaticStyles( QDomElement &parent )
{
  QDomElement element = parent.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "style" ) ) {
      StyleFormatProperty property = parseStyleProperty( element );
      mStyleInformation->addStyleProperty( element.attribute( "name" ), property );
    } else if ( element.tagName() == QLatin1String( "page-layout" ) ) {
      QDomElement child = element.firstChildElement();
      while ( !child.isNull() ) {
        if ( child.tagName() == QLatin1String( "page-layout-properties" ) ) {
          PageFormatProperty property = parsePageProperty( child );
          mStyleInformation->addPageProperty( element.attribute( "name" ), property );
        }

        child = child.nextSiblingElement();
      }
    } else {
      qDebug( "unknown tag %s", qPrintable( element.tagName() ) );
    }

    element = element.nextSiblingElement();
  }

  return true;
}

StyleFormatProperty StyleParser::parseStyleProperty( QDomElement &parent )
{
  StyleFormatProperty property( mStyleInformation );

  property.setParentStyleName( parent.attribute( "parent-style-name" ) );
  property.setFamily( parent.attribute( "family" ) );
  property.setMasterPageName( parent.attribute( "master-page-name" ) );

  QDomElement element = parent.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "paragraph-properties" ) ) {
      ParagraphFormatProperty paragraphProperty = parseParagraphProperty( element );
      property.setParagraphFormat( paragraphProperty );
    } else if ( element.tagName() == QLatin1String( "text-properties" ) ) {
      TextFormatProperty textProperty = parseTextProperty( element );
      property.setTextFormat( textProperty );
    } else {
      qDebug( "unknown tag %s", qPrintable( element.tagName() ) );
    }

    element = element.nextSiblingElement();
  }

  return property;
}

ParagraphFormatProperty StyleParser::parseParagraphProperty( QDomElement &parent )
{
  ParagraphFormatProperty property;

  property.setPageNumber( parent.attribute( "page-number" ).toInt() );

  static QMap<QString, ParagraphFormatProperty::WritingMode> map;
  if ( map.isEmpty() ) {
    map.insert( "lr-tb", ParagraphFormatProperty::LRTB );
    map.insert( "rl-tb", ParagraphFormatProperty::RLTB );
    map.insert( "tb-rl", ParagraphFormatProperty::TBRL );
    map.insert( "tb-lr", ParagraphFormatProperty::TBLR );
    map.insert( "lr", ParagraphFormatProperty::LR );
    map.insert( "rl", ParagraphFormatProperty::RL );
    map.insert( "tb", ParagraphFormatProperty::TB );
    map.insert( "page", ParagraphFormatProperty::PAGE );
  }
  property.setWritingMode( map[ parent.attribute( "writing-mode" ) ] );

  static QMap<QString, Qt::Alignment> alignMap;
  if ( alignMap.isEmpty() ) {
    alignMap.insert( "center", Qt::AlignCenter );
    alignMap.insert( "left", Qt::AlignLeft );
    alignMap.insert( "right", Qt::AlignRight );
  }
  if ( parent.hasAttribute( "text-align" ) ) {
    property.setTextAlignment( alignMap[ parent.attribute( "text-align", "left" ) ] );
  }

  return property;
}

TextFormatProperty StyleParser::parseTextProperty( QDomElement &parent )
{
  TextFormatProperty property;

  QString fontSize = parent.attribute( "font-size" );
  if ( !fontSize.isEmpty() )
    property.setFontSize( convertPoints( fontSize ) );

  QColor color( parent.attribute( "color" ) );
  if ( color.isValid() ) {
    property.setColor( color );
  }

  const QString colorText = parent.attribute( "background-color" );
  if ( !colorText.isEmpty() && colorText != QLatin1String( "transparent" ) ) {
    property.setBackgroundColor( QColor( colorText ) );
  }

  return property;
}

PageFormatProperty StyleParser::parsePageProperty( QDomElement &parent )
{
  PageFormatProperty property;

  property.setBottomMargin( convertPoints( parent.attribute( "margin-bottom" ) ) );
  property.setLeftMargin( convertPoints( parent.attribute( "margin-left" ) ) );
  property.setTopMargin( convertPoints( parent.attribute( "margin-top" ) ) );
  property.setRightMargin( convertPoints( parent.attribute( "margin-right" ) ) );
  property.setWidth( convertPoints( parent.attribute( "page-width" ) ) );
  property.setHeight( convertPoints( parent.attribute( "page-height" ) ) );

  return property;
}

int StyleParser::convertPoints( const QString &data ) const
{
  return data.left( data.length() - 2 ).toInt();
}
