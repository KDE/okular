/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "formatproperty.h"

#include <QtGui/QTextFormat>

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

  if ( mBackgroundColor.isValid() )
    format->setBackground( mBackgroundColor );
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

void ParagraphFormatProperty::setBackgroundColor( const QColor &color )
{
  mBackgroundColor = color;
}

TextFormatProperty::TextFormatProperty()
  : mStyleInformation( 0 ), mHasFontSize( false ),
    mFontWeight( -1 ), mTextPosition( 0 )
{
}

TextFormatProperty::TextFormatProperty( const StyleInformation *information )
  : mStyleInformation( information ), mHasFontSize( false ),
    mFontWeight( -1 ), mTextPosition( 0 )
{
}

void TextFormatProperty::apply( QTextCharFormat *format ) const
{
  if ( !mFontName.isEmpty() ) {
    if ( mStyleInformation ) {
      const FontFormatProperty property = mStyleInformation->fontProperty( mFontName );
      property.apply( format );
    }
  }

  if ( mFontWeight != -1 ) {
    QFont font = format->font();
    font.setWeight( mFontWeight );
    format->setFont( font );
  }

  if ( mHasFontSize ) {
    QFont font = format->font();
    font.setPointSize( mFontSize );
    format->setFont( font );
  }

  if ( mColor.isValid() )
    format->setForeground( mColor );

  if ( mBackgroundColor.isValid() )
    format->setBackground( mBackgroundColor );

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

void TextFormatProperty::setFontWeight( int weight )
{
  mFontWeight = weight;
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
  : mStyleInformation( 0 ), mDefaultStyle( false )
{
}

StyleFormatProperty::StyleFormatProperty( const StyleInformation *information )
  : mStyleInformation( information ), mDefaultStyle( false )
{
}

void StyleFormatProperty::applyBlock( QTextBlockFormat *format ) const
{
  if ( !mDefaultStyle && !mFamily.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mFamily );
    property.applyBlock( format );
  }

  if ( !mParentStyleName.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mParentStyleName );
    property.applyBlock( format );
  }

  mParagraphFormat.apply( format );
}

void StyleFormatProperty::applyText( QTextCharFormat *format ) const
{
  if ( !mDefaultStyle && !mFamily.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mFamily );
    property.applyText( format );
  }

  if ( !mParentStyleName.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mParentStyleName );
    property.applyText( format );
  }

  mTextFormat.apply( format );
}

void StyleFormatProperty::applyTableColumn( QTextTableFormat *format ) const
{
  if ( !mDefaultStyle && !mFamily.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mFamily );
    property.applyTableColumn( format );
  }

  if ( !mParentStyleName.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mParentStyleName );
    property.applyTableColumn( format );
  }

  mTableColumnFormat.apply( format );
}

void StyleFormatProperty::applyTableCell( QTextBlockFormat *format ) const
{
  if ( !mDefaultStyle && !mFamily.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mFamily );
    property.applyTableCell( format );
  }

  if ( !mParentStyleName.isEmpty() && mStyleInformation ) {
    const StyleFormatProperty property = mStyleInformation->styleProperty( mParentStyleName );
    property.applyTableCell( format );
  }

  mTableCellFormat.apply( format );
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

void StyleFormatProperty::setDefaultStyle( bool defaultStyle )
{
  mDefaultStyle = defaultStyle;
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

void StyleFormatProperty::setTableColumnFormat( const TableColumnFormatProperty &format )
{
  mTableColumnFormat = format;
}

void StyleFormatProperty::setTableCellFormat( const TableCellFormatProperty &format )
{
  mTableCellFormat = format;
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

double PageFormatProperty::margin() const
{
  return mLeftMargin;
}

ListFormatProperty::ListFormatProperty()
  : mType( Number )
{
  mIndents.resize( 10 );
}

ListFormatProperty::ListFormatProperty( Type type )
  : mType( type )
{
  mIndents.resize( 10 );
}

void ListFormatProperty::apply( QTextListFormat *format, int level ) const
{
  if ( mType == Number )
    format->setStyle( QTextListFormat::ListDecimal );
  else {
    format->setStyle( QTextListFormat::ListDisc );
    if ( level > 0 && level < 10 )
      format->setIndent( qRound( mIndents[ level ] ) );
  }
}

void ListFormatProperty::addItem( int level, double indent )
{
  if ( level < 0 || level >= 10 )
    return;

  mIndents[ level ] = indent;
}

TableColumnFormatProperty::TableColumnFormatProperty()
  : mWidth( 0 )
{
}

void TableColumnFormatProperty::apply( QTextTableFormat *format ) const
{
  QVector<QTextLength> lengths = format->columnWidthConstraints();
  lengths.append( QTextLength( QTextLength::FixedLength, mWidth ) );

  format->setColumnWidthConstraints( lengths );
}

void TableColumnFormatProperty::setWidth( double width )
{
  mWidth = width;
}

TableCellFormatProperty::TableCellFormatProperty()
  : mPadding( 0 ), mHasAlignment( false )
{
}

void TableCellFormatProperty::apply( QTextBlockFormat *format ) const
{
  if ( mBackgroundColor.isValid() )
    format->setBackground( mBackgroundColor );

  if ( mHasAlignment )
    format->setAlignment( mAlignment );
}

void TableCellFormatProperty::setBackgroundColor( const QColor &color )
{
  mBackgroundColor = color;
}

void TableCellFormatProperty::setPadding( double padding )
{
  mPadding = padding;
}

void TableCellFormatProperty::setAlignment( Qt::Alignment alignment )
{
  mAlignment = alignment;
  mHasAlignment = true;
}
