/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OOO_FORMATPROPERTY_H
#define OOO_FORMATPROPERTY_H

#include <QtCore/QVector>
#include <QtGui/QColor>

class QTextBlockFormat;
class QTextCharFormat;
class QTextListFormat;
class QTextFormat;

namespace OOO {

class StyleInformation;

class FormatProperty
{
  public:
    virtual ~FormatProperty() {}

    virtual void apply( QTextFormat *format ) const = 0;
};

class FontFormatProperty : public FormatProperty
{
  public:
    FontFormatProperty();
    virtual ~FontFormatProperty() {}

    virtual void apply( QTextFormat *format ) const;

    void setFamily( const QString &name );

  private:
    QString mFamily;
};

class ParagraphFormatProperty : public FormatProperty
{
  public:
    enum WritingMode
    {
      LRTB,
      RLTB,
      TBRL,
      TBLR,
      LR,
      RL,
      TB,
      PAGE
    };

    ParagraphFormatProperty();
    virtual ~ParagraphFormatProperty() {}

    virtual void apply( QTextFormat *format ) const;

    void setPageNumber( int number );
    void setWritingMode( WritingMode mode );
    void setTextAlignment( Qt::Alignment alignment );

  private:
    int mPageNumber;
    WritingMode mWritingMode;
    Qt::Alignment mAlignment;
    bool mHasAlignment;
};

class TextFormatProperty : public FormatProperty
{
  public:
    TextFormatProperty();
    TextFormatProperty( const StyleInformation *information );
    virtual ~TextFormatProperty() {}

    virtual void apply( QTextFormat *format ) const;

    void setFontSize( int size );
    void setFontName( const QString &name );
    void setTextPosition( int position );
    void setColor( const QColor &color );
    void setBackgroundColor( const QColor &color );

  private:
    const StyleInformation *mStyleInformation;
    int mFontSize;
    bool mHasFontSize;
    QString mFontName;
    int mTextPosition;
    QColor mColor;
    QColor mBackgroundColor;
};

class StyleFormatProperty
{
  public:
    StyleFormatProperty();
    StyleFormatProperty( const StyleInformation *information );
    virtual ~StyleFormatProperty() {}

    virtual void apply( QTextBlockFormat *blockFormat, QTextCharFormat *textFormat ) const;

    void setParentStyleName( const QString &parentStyleName );
    QString parentStyleName() const;

    void setFamily( const QString &family );
    void setMasterPageName( const QString &masterPageName );

    void setParagraphFormat( const ParagraphFormatProperty &format );
    void setTextFormat( const TextFormatProperty &format );

  private:
    QString mParentStyleName;
    QString mFamily;
    QString mMasterPageName;
    ParagraphFormatProperty mParagraphFormat;
    TextFormatProperty mTextFormat;
    const StyleInformation *mStyleInformation;
};

class PageFormatProperty : public FormatProperty
{
  public:
    enum PageUsage
    {
      All,
      Left,
      Right,
      Mirrored
    };

    enum PrintOrientation
    {
      Portrait,
      Landscape
    };

    PageFormatProperty();
    virtual ~PageFormatProperty() {}

    virtual void apply( QTextFormat *format ) const;

    void setPageUsage( PageUsage usage );
    void setBottomMargin( double margin );
    void setLeftMargin( double margin );
    void setTopMargin( double margin );
    void setRightMargin( double margin );
    void setHeight( double height );
    void setWidth( double width );
    void setPrintOrientation( PrintOrientation orientation );

    double width() const;
    double height() const;
    double margin() const;

  private:
    PageUsage mPageUsage;
    double mBottomMargin;
    double mLeftMargin;
    double mTopMargin;
    double mRightMargin;
    double mHeight;
    double mWidth;
    PrintOrientation mPrintOrientation;
};

class ListFormatProperty
{
  public:
    enum Type
    {
      Number,
      Bullet
    };

    ListFormatProperty();
    ListFormatProperty( Type type );
    virtual ~ListFormatProperty();

    virtual void apply( QTextListFormat *format, int level ) const;

    void addItem( int level, double indent = 0 );

  private:
    Type mType;
    QVector<double> mIndents;
};

}

#endif
