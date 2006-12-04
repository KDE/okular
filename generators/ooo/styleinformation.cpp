/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "styleinformation.h"

using namespace OOO;

MetaInformation::MetaInformation( const QString &key, const QString &value, const QString &title )
  : mKey( key ), mValue( value ), mTitle( title )
{
}

QString MetaInformation::key() const
{
  return mKey;
}

QString MetaInformation::value() const
{
  return mValue;
}

QString MetaInformation::title() const
{
  return mTitle;
}

StyleInformation::StyleInformation()
{
}

StyleInformation::~StyleInformation()
{
}

void StyleInformation::addFontProperty( const QString &name, const FontFormatProperty &property )
{
  mFontProperties.insert( name, property );
}

FontFormatProperty StyleInformation::fontProperty( const QString &name ) const
{
  return mFontProperties[ name ];
}

void StyleInformation::addStyleProperty( const QString &name, const StyleFormatProperty &property )
{
  mStyleProperties.insert( name, property );
}

StyleFormatProperty StyleInformation::styleProperty( const QString &name ) const
{
  return mStyleProperties[ name ];
}

void StyleInformation::addPageProperty( const QString &name, const PageFormatProperty &property )
{
  mPageProperties.insert( name, property );
}

PageFormatProperty StyleInformation::pageProperty( const QString &name ) const
{
  return mPageProperties[ name ];
}

void StyleInformation::addListProperty( const QString &name, const ListFormatProperty &property )
{
  mListProperties[ name ] = property;
}

ListFormatProperty StyleInformation::listProperty( const QString &name ) const
{
  return mListProperties[ name ];
}

void StyleInformation::addMasterLayout( const QString &name, const QString &layoutName )
{
  mMasterLayouts.insert( name, layoutName );
}

QString StyleInformation::masterLayout( const QString &name )
{
  return mMasterLayouts[ name ];
}

void StyleInformation::setMasterPageName( const QString &name )
{
  mMasterPageName = name;
}

QString StyleInformation::masterPageName() const
{
  return mMasterPageName.isEmpty() ? mMasterLayouts[ "Standard" ] :  mMasterLayouts[ mMasterPageName ];
}

void StyleInformation::addMetaInformation( const QString &key, const QString &value, const QString &title )
{
  const MetaInformation meta( key, value, title );
  mMetaInformation.append( meta );
}

MetaInformation::List StyleInformation::metaInformation() const
{
  return mMetaInformation;
}

void StyleInformation::dump() const
{
  QMapIterator<QString, StyleFormatProperty> it( mStyleProperties );
  while ( it.hasNext() ) {
    it.next();
    qDebug( "%s", qPrintable( it.key() ) );
  }
}
