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

/** Converts a Number to a String using l10n
 *
 * String numberToString( Number number, String format = 'g', int precision = 6,
 *                        String LocaleName = system )
 */
QString JSUtil::numberToString(double number, const QString &fmt, int precision, const QString &localeName) const
{
    if (std::isnan(number)) {
        return QStringLiteral("NaN");
    }

    QChar format = QLatin1Char('g');
    if (!fmt.isEmpty()) {
        format = fmt[0];
    }

    QLocale locale;
    if (!localeName.isEmpty()) {
        locale = QLocale(localeName);
    }

    return locale.toString(number, format.toLatin1(), precision);
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
