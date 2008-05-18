/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_fullscreen_p.h"

#include <assert.h>

#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>

#include "settings.h"

using namespace Okular;

static KJSPrototype *g_fsProto;

static KJSObject fsGetLoop( KJSContext *, void * )
{
    return KJSBoolean( Settings::slidesLoop() );
}

static void fsSetLoop( KJSContext *ctx, void *, KJSObject value )
{
    bool loop = value.toBoolean( ctx );
    Settings::setSlidesLoop( loop );
}

static KJSObject fsGetUseTimer( KJSContext *, void * )
{
    return KJSBoolean( Settings::slidesAdvance() );
}

static void fsSetUseTimer( KJSContext *ctx, void *, KJSObject value )
{
    bool use = value.toBoolean( ctx );
    Settings::setSlidesAdvance( use );
}

static KJSObject fsGetTimeDelay( KJSContext *, void * )
{
    return KJSNumber( Settings::slidesAdvanceTime() );
}

static void fsSetTimeDelay( KJSContext *ctx, void *, KJSObject value )
{
    int time = static_cast<int>( value.toNumber( ctx ) );
    Settings::setSlidesAdvanceTime( time );
}

void JSFullscreen::initType( KJSContext *ctx )
{
    static bool initialized = false;
    if ( initialized )
        return;
    initialized = true;

    if ( !g_fsProto )
        g_fsProto = new KJSPrototype();

    g_fsProto->defineProperty( ctx, "loop", fsGetLoop, fsSetLoop );
    g_fsProto->defineProperty( ctx, "useTimer",
                               fsGetUseTimer, fsSetUseTimer );
    g_fsProto->defineProperty( ctx, "timeDelay",
                               fsGetTimeDelay, fsSetTimeDelay );
}

KJSObject JSFullscreen::object( KJSContext *ctx )
{
    assert( g_fsProto );
    return g_fsProto->constructObject( ctx );
}
