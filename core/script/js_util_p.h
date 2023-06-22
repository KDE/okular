/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_UTIL_P_H
#define OKULAR_SCRIPT_JS_UTIL_P_H

#include <QJSValue>
#include <QObject>

namespace Okular
{
class JSUtil : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE QJSValue crackURL(const QString &cURL) const;
    Q_INVOKABLE QJSValue printd(const QJSValue &oFormat, const QDateTime &oDate) const;
    Q_INVOKABLE double stringToNumber(const QString &number) const;
    Q_INVOKABLE QString numberToString(double number, const QString &fmt = QStringLiteral("g"), int precision = 6, const QString &localeName = {}) const;
};

}

#endif
