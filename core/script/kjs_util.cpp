/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_util_p.h"

#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>
#include <kjs/kjsarguments.h>

#include <kurl.h>

using namespace Okular;

static KJSPrototype *g_utilProto;

static KJSObject crackURL( KJSContext *context, void *,
                           const KJSArguments &arguments )
{
    if ( arguments.count() < 1 )
    {
        return context->throwException( "Missing URL argument" );
    }
    QString cURL = arguments.at( 0 ).toString( context );
    KUrl url( cURL );
    if ( !url.isValid() )
    {
        return context->throwException( "Invalid URL" );
    }
    if ( url.protocol() != QLatin1String( "file" )
         || url.protocol() != QLatin1String( "http" )
         || url.protocol() != QLatin1String( "https" ) )
    {
        return context->throwException( "Protocol not valid: '" + url.protocol() + '\'' );
    }

    KJSObject obj;
    obj.setProperty( context, "cScheme", url.protocol() );
    if ( url.hasUser() )
        obj.setProperty( context, "cUser", url.user() );
    if ( url.hasPass() )
        obj.setProperty( context, "cPassword", url.password() );
    obj.setProperty( context, "cHost", url.host() );
    obj.setProperty( context, "nPort", url.port( 80 ) );
    // TODO cPath       (Optional) The path portion of the URL.
    // TODO cParameters (Optional) The parameter string portion of the URL.
    if ( url.hasRef() )
        obj.setProperty( context, "cFragments", url.ref() );

    return obj;
}

void JSUtil::initType( KJSContext *ctx )
{
    static bool initialized = false;
    if ( initialized )
        return;
    initialized = true;

    g_utilProto = new KJSPrototype();
    g_utilProto->defineFunction( ctx, "crackURL", crackURL );
}

KJSObject JSUtil::object( KJSContext *ctx )
{
    return g_utilProto->constructObject( ctx, 0 );
}

