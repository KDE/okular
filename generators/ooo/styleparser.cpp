/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "styleparser.h"

#include <QDateTime>
#include <QDomDocument>
#include <QDomElement>
#include <QFont>
#include <QXmlSimpleReader>

#include <KLocalizedString>

#include "document.h"
#include "styleinformation.h"

using namespace OOO;

StyleParser::StyleParser(const Document *document, const QDomDocument &domDocument, StyleInformation *styleInformation)
    : mDocument(document)
    , mDomDocument(domDocument)
    , mStyleInformation(styleInformation)
    , mMasterPageNameSet(false)
{
}

bool StyleParser::parse()
{
    if (!parseContentFile())
        return false;

    if (!parseStyleFile())
        return false;

    if (!parseMetaFile())
        return false;

    return true;
}

bool StyleParser::parseContentFile()
{
    const QDomElement documentElement = mDomDocument.documentElement();
    QDomElement element = documentElement.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("document-common-attrs")) {
            if (!parseDocumentCommonAttrs(element))
                return false;
        } else if (element.tagName() == QLatin1String("font-face-decls")) {
            if (!parseFontFaceDecls(element))
                return false;
        } else if (element.tagName() == QLatin1String("styles")) {
            if (!parseStyles(element))
                return false;
        } else if (element.tagName() == QLatin1String("automatic-styles")) {
            if (!parseAutomaticStyles(element))
                return false;
        }

        element = element.nextSiblingElement();
    }

    return true;
}

bool StyleParser::parseStyleFile()
{
    if (mDocument->styles().isEmpty())
        return true;

    QXmlSimpleReader reader;

    QXmlInputSource source;
    source.setData(mDocument->styles());

    QString errorMsg;
    int errorLine, errorCol;

    QDomDocument document;
    if (!document.setContent(&source, &reader, &errorMsg, &errorLine, &errorCol)) {
        qDebug("%s at (%d,%d)", qPrintable(errorMsg), errorLine, errorCol);
        return false;
    }

    const QDomElement documentElement = document.documentElement();
    QDomElement element = documentElement.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("styles")) {
            if (!parseAutomaticStyles(element))
                return false;
        } else if (element.tagName() == QLatin1String("automatic-styles")) {
            if (!parseAutomaticStyles(element))
                return false;
        } else if (element.tagName() == QLatin1String("master-styles")) {
            if (!parseMasterStyles(element))
                return false;
        }

        element = element.nextSiblingElement();
    }

    return true;
}

bool StyleParser::parseMetaFile()
{
    if (mDocument->meta().isEmpty())
        return true;

    QXmlSimpleReader reader;

    QXmlInputSource source;
    source.setData(mDocument->meta());

    QString errorMsg;
    int errorLine, errorCol;

    QDomDocument document;
    if (!document.setContent(&source, &reader, &errorMsg, &errorLine, &errorCol)) {
        qDebug("%s at (%d,%d)", qPrintable(errorMsg), errorLine, errorCol);
        return false;
    }

    const QDomElement documentElement = document.documentElement();
    QDomElement element = documentElement.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("meta")) {
            QDomElement child = element.firstChildElement();
            while (!child.isNull()) {
                if (child.tagName() == QLatin1String("generator")) {
                    mStyleInformation->addMetaInformation(QStringLiteral("producer"), child.text(), i18n("Producer"));
                } else if (child.tagName() == QLatin1String("creation-date")) {
                    const QDateTime dateTime = QDateTime::fromString(child.text(), Qt::ISODate);
                    mStyleInformation->addMetaInformation(QStringLiteral("creationDate"), QLocale().toString(dateTime, QLocale::LongFormat), i18n("Created"));
                } else if (child.tagName() == QLatin1String("initial-creator")) {
                    mStyleInformation->addMetaInformation(QStringLiteral("creator"), child.text(), i18n("Creator"));
                } else if (child.tagName() == QLatin1String("creator")) {
                    mStyleInformation->addMetaInformation(QStringLiteral("author"), child.text(), i18n("Author"));
                } else if (child.tagName() == QLatin1String("date")) {
                    const QDateTime dateTime = QDateTime::fromString(child.text(), Qt::ISODate);
                    mStyleInformation->addMetaInformation(QStringLiteral("modificationDate"), QLocale().toString(dateTime, QLocale::LongFormat), i18n("Modified"));
                }

                child = child.nextSiblingElement();
            }
        }

        element = element.nextSiblingElement();
    }

    return true;
}

bool StyleParser::parseDocumentCommonAttrs(QDomElement &)
{
    return true;
}

bool StyleParser::parseFontFaceDecls(QDomElement &parent)
{
    QDomElement element = parent.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("font-face")) {
            FontFormatProperty property;
            property.setFamily(element.attribute(QStringLiteral("font-family")));

            mStyleInformation->addFontProperty(element.attribute(QStringLiteral("name")), property);
        } else {
            qDebug("unknown tag %s", qPrintable(element.tagName()));
        }

        element = element.nextSiblingElement();
    }

    return true;
}

bool StyleParser::parseStyles(QDomElement &)
{
    return true;
}

bool StyleParser::parseMasterStyles(QDomElement &parent)
{
    QDomElement element = parent.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("master-page")) {
            mStyleInformation->addMasterLayout(element.attribute(QStringLiteral("name")), element.attribute(QStringLiteral("page-layout-name")));
            if (!mMasterPageNameSet) {
                mStyleInformation->setMasterPageName(element.attribute(QStringLiteral("name")));
                mMasterPageNameSet = true;
            }
        } else {
            qDebug("unknown tag %s", qPrintable(element.tagName()));
        }

        element = element.nextSiblingElement();
    }

    return true;
}

bool StyleParser::parseAutomaticStyles(QDomElement &parent)
{
    QDomElement element = parent.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("style")) {
            const StyleFormatProperty property = parseStyleProperty(element);
            mStyleInformation->addStyleProperty(element.attribute(QStringLiteral("name")), property);
        } else if (element.tagName() == QLatin1String("page-layout")) {
            QDomElement child = element.firstChildElement();
            while (!child.isNull()) {
                if (child.tagName() == QLatin1String("page-layout-properties")) {
                    const PageFormatProperty property = parsePageProperty(child);
                    mStyleInformation->addPageProperty(element.attribute(QStringLiteral("name")), property);
                }

                child = child.nextSiblingElement();
            }
        } else if (element.tagName() == QLatin1String("list-style")) {
            const ListFormatProperty property = parseListProperty(element);
            mStyleInformation->addListProperty(element.attribute(QStringLiteral("name")), property);
        } else if (element.tagName() == QLatin1String("default-style")) {
            StyleFormatProperty property = parseStyleProperty(element);
            property.setDefaultStyle(true);
            mStyleInformation->addStyleProperty(element.attribute(QStringLiteral("family")), property);
        } else {
            qDebug("unknown tag %s", qPrintable(element.tagName()));
        }

        element = element.nextSiblingElement();
    }

    return true;
}

StyleFormatProperty StyleParser::parseStyleProperty(QDomElement &parent)
{
    StyleFormatProperty property(mStyleInformation);

    property.setParentStyleName(parent.attribute(QStringLiteral("parent-style-name")));
    property.setFamily(parent.attribute(QStringLiteral("family")));
    if (parent.hasAttribute(QStringLiteral("master-page-name"))) {
        property.setMasterPageName(parent.attribute(QStringLiteral("master-page-name")));
        if (!mMasterPageNameSet) {
            mStyleInformation->setMasterPageName(parent.attribute(QStringLiteral("master-page-name")));
            mMasterPageNameSet = true;
        }
    }

    QDomElement element = parent.firstChildElement();
    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("paragraph-properties")) {
            const ParagraphFormatProperty paragraphProperty = parseParagraphProperty(element);
            property.setParagraphFormat(paragraphProperty);
        } else if (element.tagName() == QLatin1String("text-properties")) {
            const TextFormatProperty textProperty = parseTextProperty(element);
            property.setTextFormat(textProperty);
        } else if (element.tagName() == QLatin1String("table-column-properties")) {
            const TableColumnFormatProperty tableColumnProperty = parseTableColumnProperty(element);
            property.setTableColumnFormat(tableColumnProperty);
        } else if (element.tagName() == QLatin1String("table-cell-properties")) {
            const TableCellFormatProperty tableCellProperty = parseTableCellProperty(element);
            property.setTableCellFormat(tableCellProperty);
        } else {
            qDebug("unknown tag %s", qPrintable(element.tagName()));
        }

        element = element.nextSiblingElement();
    }

    return property;
}

ParagraphFormatProperty StyleParser::parseParagraphProperty(QDomElement &parent)
{
    ParagraphFormatProperty property;

    property.setPageNumber(parent.attribute(QStringLiteral("page-number")).toInt());

    static QMap<QString, ParagraphFormatProperty::WritingMode> map;
    if (map.isEmpty()) {
        map.insert(QStringLiteral("lr-tb"), ParagraphFormatProperty::LRTB);
        map.insert(QStringLiteral("rl-tb"), ParagraphFormatProperty::RLTB);
        map.insert(QStringLiteral("tb-rl"), ParagraphFormatProperty::TBRL);
        map.insert(QStringLiteral("tb-lr"), ParagraphFormatProperty::TBLR);
        map.insert(QStringLiteral("lr"), ParagraphFormatProperty::LR);
        map.insert(QStringLiteral("rl"), ParagraphFormatProperty::RL);
        map.insert(QStringLiteral("tb"), ParagraphFormatProperty::TB);
        map.insert(QStringLiteral("page"), ParagraphFormatProperty::PAGE);
    }
    property.setWritingMode(map[parent.attribute(QStringLiteral("writing-mode"))]);

    static QMap<QString, Qt::Alignment> alignMap;
    if (alignMap.isEmpty()) {
        alignMap.insert(QStringLiteral("center"), Qt::AlignCenter);
        alignMap.insert(QStringLiteral("left"), Qt::AlignLeft);
        alignMap.insert(QStringLiteral("right"), Qt::AlignRight);
        alignMap.insert(QStringLiteral("justify"), Qt::AlignJustify);
        if (property.writingModeIsRightToLeft()) {
            alignMap.insert(QStringLiteral("start"), Qt::AlignRight);
            alignMap.insert(QStringLiteral("end"), Qt::AlignLeft);
        } else {
            // not right to left
            alignMap.insert(QStringLiteral("start"), Qt::AlignLeft);
            alignMap.insert(QStringLiteral("end"), Qt::AlignRight);
        }
    }
    if (parent.hasAttribute(QStringLiteral("text-align"))) {
        property.setTextAlignment(alignMap[parent.attribute(QStringLiteral("text-align"), QStringLiteral("left"))]);
    }

    const QString marginLeft = parent.attribute(QStringLiteral("margin-left"));
    if (!marginLeft.isEmpty()) {
        qreal leftMargin = qRound(convertUnit(marginLeft));
        property.setLeftMargin(leftMargin);
    }

    const QString colorText = parent.attribute(QStringLiteral("background-color"));
    if (!colorText.isEmpty() && colorText != QLatin1String("transparent")) {
        property.setBackgroundColor(QColor(colorText));
    }

    return property;
}

TextFormatProperty StyleParser::parseTextProperty(QDomElement &parent)
{
    TextFormatProperty property;

    const QString fontSize = parent.attribute(QStringLiteral("font-size"));
    if (!fontSize.isEmpty())
        property.setFontSize(qRound(convertUnit(fontSize)));

    static QMap<QString, QFont::Weight> weightMap;
    if (weightMap.isEmpty()) {
        weightMap.insert(QStringLiteral("normal"), QFont::Normal);
        weightMap.insert(QStringLiteral("bold"), QFont::Bold);
    }

    const QString fontWeight = parent.attribute(QStringLiteral("font-weight"));
    if (!fontWeight.isEmpty())
        property.setFontWeight(weightMap[fontWeight]);

    static QMap<QString, QFont::Style> fontStyleMap;
    if (fontStyleMap.isEmpty()) {
        fontStyleMap.insert(QStringLiteral("normal"), QFont::StyleNormal);
        fontStyleMap.insert(QStringLiteral("italic"), QFont::StyleItalic);
        fontStyleMap.insert(QStringLiteral("oblique"), QFont::StyleOblique);
    }

    const QString fontStyle = parent.attribute(QStringLiteral("font-style"));
    if (!fontStyle.isEmpty())
        property.setFontStyle(fontStyleMap.value(fontStyle, QFont::StyleNormal));

    const QColor color(parent.attribute(QStringLiteral("color")));
    if (color.isValid()) {
        property.setColor(color);
    }

    const QString colorText = parent.attribute(QStringLiteral("background-color"));
    if (!colorText.isEmpty() && colorText != QLatin1String("transparent")) {
        property.setBackgroundColor(QColor(colorText));
    }

    return property;
}

PageFormatProperty StyleParser::parsePageProperty(QDomElement &parent)
{
    PageFormatProperty property;

    property.setBottomMargin(convertUnit(parent.attribute(QStringLiteral("margin-bottom"))));
    property.setLeftMargin(convertUnit(parent.attribute(QStringLiteral("margin-left"))));
    property.setTopMargin(convertUnit(parent.attribute(QStringLiteral("margin-top"))));
    property.setRightMargin(convertUnit(parent.attribute(QStringLiteral("margin-right"))));
    property.setWidth(convertUnit(parent.attribute(QStringLiteral("page-width"))));
    property.setHeight(convertUnit(parent.attribute(QStringLiteral("page-height"))));

    return property;
}

ListFormatProperty StyleParser::parseListProperty(QDomElement &parent)
{
    ListFormatProperty property;

    QDomElement element = parent.firstChildElement();
    if (element.tagName() == QLatin1String("list-level-style-number"))
        property = ListFormatProperty(ListFormatProperty::Number);
    else
        property = ListFormatProperty(ListFormatProperty::Bullet);

    while (!element.isNull()) {
        if (element.tagName() == QLatin1String("list-level-style-number")) {
            int level = element.attribute(QStringLiteral("level")).toInt();
            property.addItem(level, 0.0);
        } else if (element.tagName() == QLatin1String("list-level-style-bullet")) {
            int level = element.attribute(QStringLiteral("level")).toInt();
            property.addItem(level, convertUnit(element.attribute(QStringLiteral("space-before"))));
        }

        element = element.nextSiblingElement();
    }

    return property;
}

TableColumnFormatProperty StyleParser::parseTableColumnProperty(QDomElement &parent)
{
    TableColumnFormatProperty property;

    const double width = convertUnit(parent.attribute(QStringLiteral("column-width")));
    property.setWidth(width);

    return property;
}

TableCellFormatProperty StyleParser::parseTableCellProperty(QDomElement &parent)
{
    TableCellFormatProperty property;

    if (parent.hasAttribute(QStringLiteral("background-color")))
        property.setBackgroundColor(QColor(parent.attribute(QStringLiteral("background-color"))));

    property.setPadding(convertUnit(parent.attribute(QStringLiteral("padding"))));

    static QMap<QString, Qt::Alignment> map;
    if (map.isEmpty()) {
        map.insert(QStringLiteral("top"), Qt::AlignTop);
        map.insert(QStringLiteral("middle"), Qt::AlignVCenter);
        map.insert(QStringLiteral("bottom"), Qt::AlignBottom);
        map.insert(QStringLiteral("left"), Qt::AlignLeft);
        map.insert(QStringLiteral("right"), Qt::AlignRight);
        map.insert(QStringLiteral("center"), Qt::AlignHCenter);
    }

    if (parent.hasAttribute(QStringLiteral("align")) && parent.hasAttribute(QStringLiteral("vertical-align"))) {
        property.setAlignment(map[parent.attribute(QStringLiteral("align"))] | map[parent.attribute(QStringLiteral("vertical-align"))]);
    } else if (parent.hasAttribute(QStringLiteral("align"))) {
        property.setAlignment(map[parent.attribute(QStringLiteral("align"))]);
    } else if (parent.hasAttribute(QStringLiteral("vertical-align"))) {
        property.setAlignment(map[parent.attribute(QStringLiteral("vertical-align"))]);
    }

    return property;
}

double StyleParser::convertUnit(const QString &data)
{
#define MM_TO_POINT(mm) ((mm)*2.83465058)
#define CM_TO_POINT(cm) ((cm)*28.3465058)
#define DM_TO_POINT(dm) ((dm)*283.465058)
#define INCH_TO_POINT(inch) ((inch)*72.0)
#define PI_TO_POINT(pi) ((pi)*12)
#define DD_TO_POINT(dd) ((dd)*154.08124)
#define CC_TO_POINT(cc) ((cc)*12.840103)

    double points = 0;
    if (data.endsWith(QLatin1String("pt"))) {
        points = data.leftRef(data.length() - 2).toDouble();
    } else if (data.endsWith(QLatin1String("cm"))) {
        double value = data.leftRef(data.length() - 2).toDouble();
        points = CM_TO_POINT(value);
    } else if (data.endsWith(QLatin1String("mm"))) {
        double value = data.leftRef(data.length() - 2).toDouble();
        points = MM_TO_POINT(value);
    } else if (data.endsWith(QLatin1String("dm"))) {
        double value = data.leftRef(data.length() - 2).toDouble();
        points = DM_TO_POINT(value);
    } else if (data.endsWith(QLatin1String("in"))) {
        double value = data.leftRef(data.length() - 2).toDouble();
        points = INCH_TO_POINT(value);
    } else if (data.endsWith(QLatin1String("inch"))) {
        double value = data.leftRef(data.length() - 4).toDouble();
        points = INCH_TO_POINT(value);
    } else if (data.endsWith(QLatin1String("pi"))) {
        double value = data.leftRef(data.length() - 4).toDouble();
        points = PI_TO_POINT(value);
    } else if (data.endsWith(QLatin1String("dd"))) {
        double value = data.leftRef(data.length() - 4).toDouble();
        points = DD_TO_POINT(value);
    } else if (data.endsWith(QLatin1String("cc"))) {
        double value = data.leftRef(data.length() - 4).toDouble();
        points = CC_TO_POINT(value);
    } else {
        if (!data.isEmpty()) {
            qDebug("unknown unit for '%s'", qPrintable(data));
        }
        points = 12;
    }

    return points;
}
