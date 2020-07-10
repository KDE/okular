/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_spell_p.h"

#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>

#include <QString>

using namespace Okular;

static KJSPrototype *g_spellProto;

// Spell.available
static KJSObject spellGetAvailable(KJSContext *, void *)
{
    return KJSBoolean(false);
}

void JSSpell::initType(KJSContext *ctx)
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    g_spellProto = new KJSPrototype();
    g_spellProto->defineProperty(ctx, QStringLiteral("available"), spellGetAvailable);
}

KJSObject JSSpell::object(KJSContext *ctx)
{
    return g_spellProto->constructObject(ctx, nullptr);
}
