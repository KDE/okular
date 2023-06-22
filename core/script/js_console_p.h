/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_CONSOLE_P_H
#define OKULAR_SCRIPT_JS_CONSOLE_P_H

#include <QObject>

namespace Okular
{
class JSConsole : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void clear();
    Q_INVOKABLE void hide();
    Q_INVOKABLE void println(const QString &cMessage);
    Q_INVOKABLE void show();
};

}

#endif
