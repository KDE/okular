/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef OKULAR_SCRIPT_JS_FULLSCREEN_P_H
#define OKULAR_SCRIPT_JS_FULLSCREEN_P_H

#include <QObject>

namespace Okular
{
class JSFullscreen : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loop READ loop WRITE setLoop)               // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(bool useTimer READ useTimer WRITE setUseTimer)   // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(int timeDelay READ timeDelay WRITE setTimeDelay) // clazy:exclude=qproperty-without-notify

public:
    bool loop() const;
    void setLoop(bool loop);
    bool useTimer() const;
    void setUseTimer(bool use);
    int timeDelay() const;
    void setTimeDelay(int time);
};

}

#endif
