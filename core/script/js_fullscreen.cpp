/*
    SPDX-FileCopyrightText: 2008 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2008 Harri Porten <porten@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "js_fullscreen_p.h"

#include "settings_core.h"

using namespace Okular;

bool JSFullscreen::loop() const
{
    return SettingsCore::slidesLoop();
}

void JSFullscreen::setLoop(bool loop)
{
    SettingsCore::setSlidesLoop(loop);
}

bool JSFullscreen::useTimer() const
{
    return SettingsCore::slidesAdvance();
}

void JSFullscreen::setUseTimer(bool use)
{
    SettingsCore::setSlidesAdvance(use);
}

int JSFullscreen::timeDelay() const
{
    return SettingsCore::slidesAdvanceTime();
}

void JSFullscreen::setTimeDelay(int time)
{
    SettingsCore::setSlidesAdvanceTime(time);
}
