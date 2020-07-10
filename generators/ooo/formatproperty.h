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

#include <QColor>
#include <QVector>

class QTextBlockFormat;
class QTextCharFormat;
class QTextFormat;
class QTextListFormat;
class QTextTableFormat;

namespace OOO
{
class StyleInformation;

class FontFormatProperty
{
public:
    FontFormatProperty();

    void apply(QTextFormat *format) const;

    void setFamily(const QString &name);

private:
    QString mFamily;
};

class ParagraphFormatProperty
{
public:
    enum WritingMode { LRTB, RLTB, TBRL, TBLR, LR, RL, TB, PAGE };

    ParagraphFormatProperty();

    void apply(QTextFormat *format) const;

    void setPageNumber(int number);
    void setWritingMode(WritingMode mode);
    void setTextAlignment(Qt::Alignment alignment);
    void setBackgroundColor(const QColor &color);
    void setLeftMargin(const qreal margin);

    bool writingModeIsRightToLeft() const;

private:
    int mPageNumber;
    WritingMode mWritingMode;
    Qt::Alignment mAlignment;
    bool mHasAlignment;
    QColor mBackgroundColor;
    qreal mLeftMargin;
};

class TextFormatProperty
{
public:
    TextFormatProperty();
    explicit TextFormatProperty(const StyleInformation *information);

    void apply(QTextCharFormat *format) const;

    void setFontSize(int size);
    void setFontName(const QString &name);
    void setFontWeight(int weight);
    void setFontStyle(int style);
    void setTextPosition(int position);
    void setColor(const QColor &color);
    void setBackgroundColor(const QColor &color);

private:
    const StyleInformation *mStyleInformation;
    int mFontSize;
    bool mHasFontSize;
    int mFontWeight;
    QString mFontName;
    int mFontStyle;
    int mTextPosition;
    QColor mColor;
    QColor mBackgroundColor;
};

class PageFormatProperty
{
public:
    enum PageUsage { All, Left, Right, Mirrored };

    enum PrintOrientation { Portrait, Landscape };

    PageFormatProperty();

    void apply(QTextFormat *format) const;

    void setPageUsage(PageUsage usage);
    void setBottomMargin(double margin);
    void setLeftMargin(double margin);
    void setTopMargin(double margin);
    void setRightMargin(double margin);
    void setHeight(double height);
    void setWidth(double width);
    void setPrintOrientation(PrintOrientation orientation);

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
    enum Type { Number, Bullet };

    ListFormatProperty();
    explicit ListFormatProperty(Type type);

    void apply(QTextListFormat *format, int level) const;

    void addItem(int level, double indent = 0);

private:
    Type mType;
    QVector<double> mIndents;
};

class TableColumnFormatProperty
{
public:
    TableColumnFormatProperty();

    void apply(QTextTableFormat *format) const;

    void setWidth(double width);

private:
    double mWidth;
    bool isValid;
};

class TableCellFormatProperty
{
public:
    TableCellFormatProperty();

    void apply(QTextBlockFormat *format) const;

    void setBackgroundColor(const QColor &color);
    void setPadding(double padding);
    void setAlignment(Qt::Alignment alignment);

private:
    QColor mBackgroundColor;
    double mPadding;
    Qt::Alignment mAlignment;
    bool mHasAlignment;
};

class StyleFormatProperty
{
public:
    StyleFormatProperty();
    explicit StyleFormatProperty(const StyleInformation *information);

    void applyBlock(QTextBlockFormat *format) const;
    void applyText(QTextCharFormat *format) const;
    void applyTableColumn(QTextTableFormat *format) const;
    void applyTableCell(QTextBlockFormat *format) const;

    void setParentStyleName(const QString &parentStyleName);
    QString parentStyleName() const;

    void setFamily(const QString &family);
    void setDefaultStyle(bool defaultStyle);

    void setMasterPageName(const QString &masterPageName);

    void setParagraphFormat(const ParagraphFormatProperty &format);
    void setTextFormat(const TextFormatProperty &format);
    void setTableColumnFormat(const TableColumnFormatProperty format);
    void setTableCellFormat(const TableCellFormatProperty &format);

private:
    QString mParentStyleName;
    QString mFamily;
    QString mMasterPageName;
    ParagraphFormatProperty mParagraphFormat;
    TextFormatProperty mTextFormat;
    TableColumnFormatProperty mTableColumnFormat;
    TableCellFormatProperty mTableCellFormat;
    const StyleInformation *mStyleInformation;
    bool mDefaultStyle;
};

}

#endif
