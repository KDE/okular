/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_console_p.h"

#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>
#include <kjs/kjsarguments.h>

#include <kdebug.h>

#include "../debug_p.h"

using namespace Okular;

static KJSPrototype *g_consoleProto;

static KJSObject consoleClear( KJSContext *, void *, const KJSArguments & )
{
    return KJSUndefined();
}

static KJSObject consoleHide( KJSContext *, void *, const KJSArguments & )
{
    return KJSUndefined();
}

static KJSObject consolePrintln( KJSContext *ctx, void *,
                                 const KJSArguments &arguments )
{
    QString cMessage = arguments.at( 0 ).toString( ctx );
    kDebug(OkularDebug) << "CONSOLE:" << cMessage;

    return KJSUndefined();
}

static KJSObject consoleShow( KJSContext *, void *, const KJSArguments & )
{
    return KJSUndefined();
}

void JSConsole::initType( KJSContext *ctx )
{
    static bool initialized = false;
    if ( initialized )
        return;
    initialized = true;

    g_consoleProto = new KJSPrototype();

    g_consoleProto->defineFunction( ctx, "clear", consoleClear );
    g_consoleProto->defineFunction( ctx, "hide", consoleHide );
    g_consoleProto->defineFunction( ctx, "println", consolePrintln );
    g_consoleProto->defineFunction( ctx, "hide", consoleShow );
}

KJSObject JSConsole::object( KJSContext *ctx )
{
    return g_consoleProto->constructObject( ctx, 0 );
}
