/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtGui/QTextFormat>

#include "formatproperty.h"
#include "styleinformation.h"

using namespace OOO;

FontFormatProperty::FontFormatProperty()
  : mFamily( "Nimbus Sans L" )
{
}

void FontFormatProperty::apply( QTextFormat *format ) const
{
  format->setProperty( QTextFormat::FontFamily, mFamily );
}


void FontFormatProperty::setFamily( const QString &name )
{
  mFamily = name;
}

ParagraphFormatProperty::ParagraphFormatProperty()
  : mPageNumber( 0 ), mWritingMode( LRTB ), mAlignment( Qt::AlignLeft ),
    mHasAlignment( false )
{
}

void ParagraphFormatProperty::apply( QTextFormat *format ) const
{
  if ( mWritingMode == LRTB || mWritingMode == TBLR || mWritingMode == LR || mWritingMode == TB )
    format->setLayoutDirection( Qt::LeftToRight );
  else
    format->setLayoutDirection( Qt::RightToLeft );

  if ( mHasAlignment ) {
    static_cast<QTextBlockFormat*>( format )->setAlignment( mAlignment );
  }

  format->setProperty( QTextFormat::FrameWidth, 595 );
}

void ParagraphFormatProperty::setPageNumber( int number )
{
  mPageNumber = number;
}

void ParagraphFormatProperty::setWritingMode( WritingMode mode )
{
  mWritingMode = mode;
}

void ParagraphFormatProperty::setTextAlignment( Qt::Alignment alignment )
{
  mHasAlignment = true;
  mAlignment = alignment;
}

TextFormatProperty::TextFormatProperty()
  : mStyleInformation( 0 ), mHasFontSize( false ), mTextPosition( 0 )
{
}

TextFormatProperty::TextFormatProperty( const StyleInformation *information )
  : mStyleInformation( information ), mHasFontSize( false ), mTextPosition( 0 )
{
}

void TextFormatProperty::apply( QTextFormat *format ) const
{
  if ( mHasFontSize )
    format->setProperty( QTextFormat::FontPointSize, mFontSize );

  if ( mColor.isValid() )
    format->setForeground( mColor );

  if ( mBackgroundColor.isValid() )
    format->setBackground( mBackgroundColor );

  if ( !mFontName.isEmpty() ) {
    if ( mStyleInformation ) {
      const FontFormatProperty property = mStyleInformation->fontProperty( mFontName );
      property.apply( format );
    }
  }

  // TODO: get FontFormatProperty and apply it
  // TODO: how to set the base line?!?

}

void TextFormatProperty::setFontSize( int size )
{
  mHasFontSize = true;
  mFontSize = size;
}

void TextFormatProperty::setFontName( const QString &name )
{
  mFontName = name;
}

void TextFormatProperty::setTextPosition( int position )
{
  mTextPosition = position;
}

void TextFormatProperty::setColor( const QColor &color )
{
  mColor = color;
}

void TextFormatProperty::setBackgroundColor( const QColor &color )
{
  mBackgroundColor = color;
}

StyleFormatProperty::StyleFormatProperty()
  : mStyleInformation( 0 )
{
}

StyleFormatProperty::StyleFormatProperty( const StyleInformation *information )
  : mStyleInformation( information )
{
}

void StyleFormatProperty::apply( QTextBlockFormat *blockFormat, QTextCharFormat *textFormat ) const
{
  if ( !mParentStyleName.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mParentStyleName );
    property.apply( blockFormat, textFormat );
  }

  mParagraphFormat.apply( blockFormat );
  mTextFormat.apply( textFormat );
}

void StyleFormatProperty::setParentStyleName( const QString &parentStyleName )
{
  mParentStyleName = parentStyleName;
}

QString StyleFormatProperty::parentStyleName() const
{
  return mParentStyleName;
}

void StyleFormatProperty::setFamily( const QString &family )
{
  mFamily = family;
}

void StyleFormatProperty::setMasterPageName( const QString &masterPageName )
{
  mMasterPageName = masterPageName;
}

void StyleFormatProperty::setParagraphFormat( const ParagraphFormatProperty &format )
{
  mParagraphFormat = format;
}

void StyleFormatProperty::setTextFormat( const TextFormatProperty &format )
{
  mTextFormat = format;
}

PageFormatProperty::PageFormatProperty()
{
}

void PageFormatProperty::apply( QTextFormat *format ) const
{
  format->setProperty( QTextFormat::BlockBottomMargin, mBottomMargin );
  format->setProperty( QTextFormat::BlockLeftMargin, mLeftMargin );
  format->setProperty( QTextFormat::BlockTopMargin, mTopMargin );
  format->setProperty( QTextFormat::BlockRightMargin, mRightMargin );
  format->setProperty( QTextFormat::FrameWidth, mWidth );
  format->setProperty( QTextFormat::FrameHeight, mHeight );
}

void PageFormatProperty::setPageUsage( PageUsage usage )
{
  mPageUsage = usage;
}

void PageFormatProperty::setBottomMargin( double margin )
{
  mBottomMargin = margin;
}

void PageFormatProperty::setLeftMargin( double margin )
{
  mLeftMargin = margin;
}

void PageFormatProperty::setTopMargin( double margin )
{
  mTopMargin = margin;
}

void PageFormatProperty::setRightMargin( double margin )
{
  mRightMargin = margin;
}

void PageFormatProperty::setHeight( double height )
{
  mHeight = height;
}

void PageFormatProperty::setWidth( double width )
{
  mWidth = width;
}

void PageFormatProperty::setPrintOrientation( PrintOrientation orientation )
{
  mPrintOrientation = orientation;
}

double PageFormatProperty::width() const
{
  return mWidth;
}

double PageFormatProperty::height() const
{
  return mHeight;
}
