/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_util_p.h"

#include <QDateTime>
#include <QDebug>
#include <QJSEngine>
#include <QLocale>
#include <QRegularExpression>
#include <QUrl>

#include <cmath>
#include <iomanip>

using namespace Okular;

QJSValue JSUtil::crackURL(const QString &cURL) const
{
    QUrl url(QUrl::fromLocalFile(cURL));
    if (!url.isValid()) {
        return qjsEngine(this)->newErrorObject(QJSValue::URIError, QStringLiteral("Invalid URL"));
    }
    if (url.scheme() != QLatin1String("file") || url.scheme() != QLatin1String("http") || url.scheme() != QLatin1String("https")) {
        return qjsEngine(this)->newErrorObject(QJSValue::URIError, QStringLiteral("Protocol not valid: '") + url.scheme() + QLatin1Char('\''));
    }

    QJSValue obj;
    obj.setProperty(QStringLiteral("cScheme"), url.scheme());
    if (!url.userName().isEmpty()) {
        obj.setProperty(QStringLiteral("cUser"), url.userName());
    }
    if (!url.password().isEmpty()) {
        obj.setProperty(QStringLiteral("cPassword"), url.password());
    }
    obj.setProperty(QStringLiteral("cHost"), url.host());
    obj.setProperty(QStringLiteral("nPort"), url.port(80));
    // TODO cPath       (Optional) The path portion of the URL.
    // TODO cParameters (Optional) The parameter string portion of the URL.
    if (url.hasFragment()) {
        obj.setProperty(QStringLiteral("cFragments"), url.fragment(QUrl::FullyDecoded));
    }

    return obj;
}

QJSValue JSUtil::printd(const QJSValue &oFormat, const QDateTime &oDate) const
{
    QString format;
    QLocale defaultLocale;

    if (oFormat.isNumber()) {
        int formatType = oFormat.toInt();
        switch (formatType) {
        case 0:
            format = QStringLiteral("D:yyyyMMddHHmmss");
            break;
        case 1:
            format = QStringLiteral("yyyy.MM.dd HH:mm:ss");
            break;
        case 2:
            format = defaultLocale.dateTimeFormat(QLocale::ShortFormat);
            if (!format.contains(QStringLiteral("ss"))) {
                format.insert(format.indexOf(QStringLiteral("mm")) + 2, QStringLiteral(":ss"));
            }
            break;
        }
    } else {
        format = oFormat.toString().replace(QLatin1String("tt"), QLatin1String("ap"));
        format.replace(QLatin1Char('t'), QLatin1Char('a'));
        for (QChar &formatChar : format) {
            if (formatChar == QLatin1Char('M')) {
                formatChar = QLatin1Char('m');
            } else if (formatChar == QLatin1Char('m')) {
                formatChar = QLatin1Char('M');
            }
        }
    }

    return defaultLocale.toString(oDate, format);
}

class Style0 : public std::numpunct<wchar_t>
{
protected:
    wchar_t do_decimal_point() const override
    {
        return '.';
    }
    wchar_t do_thousands_sep() const override
    {
        return ',';
    }
    std::string do_grouping() const override
    {
        return "\3";
    }
};

class Style1 : public std::numpunct<wchar_t>
{
protected:
    wchar_t do_decimal_point() const override
    {
        return '.';
    }
};

class Style2 : public std::numpunct<wchar_t>
{
protected:
    wchar_t do_decimal_point() const override
    {
        return ',';
    }
    wchar_t do_thousands_sep() const override
    {
        return '.';
    }
    std::string do_grouping() const override
    {
        return "\3";
    }
};

class Style3 : public std::numpunct<wchar_t>
{
protected:
    wchar_t do_decimal_point() const override
    {
        return ',';
    }
};

class Style4 : public std::numpunct<wchar_t>
{
protected:
    wchar_t do_decimal_point() const override
    {
        return '.';
    }
    wchar_t do_thousands_sep() const override
    {
        // Note this is ’ (U+2019) not ' (U+0027)
        return L'’';
    }
    std::string do_grouping() const override
    {
        return "\3";
    }
};

/** Converts a Number to a String using the given separator style
 *
 * String numberToString( Number number, int formatStyle, int precision, int separatorStyle )
 *
 * formatStyle is an integer denoting format style
 *          0 => default format style
 *          1 => fixed format style
 *
 * sepStyle is an integer denoting separator style
 *          0 => . as decimal separator   , as thousand separators => 1,234.56
 *          1 => . as decimal separator     no thousand separators => 1234.56
 *          2 => , as decimal separator   . as thousand separators => 1.234,56
 *          3 => , as decimal separator     no thousand separators => 1234,56
 *          4 => . as decimal separator     ’ as thousand separators => 1’234.56%
 */
QString JSUtil::numberToString(double number, int formatStyle, int precision, int separatorStyle) const
{
    if (std::isnan(number)) {
        return QStringLiteral("NaN");
    }

    std::wostringstream oss;
    std::numpunct<wchar_t> *customFormatter;
    switch (separatorStyle) {
    case 1:
        customFormatter = new Style1();
        break;
    case 2:
        customFormatter = new Style2();
        break;
    case 3:
        customFormatter = new Style3();
        break;
    case 4:
        customFormatter = new Style4();
        break;
    default:
        customFormatter = new Style0();
        break;
    }
    oss.imbue(std::locale(oss.getloc(), customFormatter));

    if (formatStyle == 1) {
        std::fixed(oss);
    }
    oss << std::setprecision(precision) << number;

    return QString::fromStdWString(oss.str());
}

/** Converts a String to a Number trying with the current locale first and
 * if that fails trying with the reverse locale for the decimal separator
 *
 * Number stringToNumber( String number ) */
double JSUtil::stringToNumber(const QString &number) const
{
    if (number.isEmpty()) {
        return 0;
    }

    const QLocale locale;
    bool ok;
    double converted = locale.toDouble(number, &ok);

    if (!ok) {
        const QLocale locale2(locale.decimalPoint() == QLatin1Char('.') ? QStringLiteral("de") : QStringLiteral("en"));
        converted = locale2.toDouble(number, &ok);
        if (!ok) {
            return NAN;
        }
    }

    return converted;
}

/*
 * Get locale dependent month names
 *
 * A method not present in the Adobe Javascript APIs reference. We add it here for our convenience. Not to be used externally.
 */
QStringList JSUtil::getMonths() const
{
    QLocale defaultLocale;
    QStringList monthNames;
    // add both long and short format dates for processing in parseDate method.
    for (int i = 1; i <= 12; i++) {
        monthNames << defaultLocale.monthName(i, QLocale::LongFormat).toUpper();
        monthNames << defaultLocale.monthName(i, QLocale::ShortFormat).toUpper();
    }
    return monthNames;
}
