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

#include <QUrl>
#include <QLocale>
#include <QDebug>
#include <QRegularExpression>
#include <QDateTime>

using namespace Okular;

static KJSPrototype *g_utilProto;

static KJSObject crackURL( KJSContext *context, void *,
                           const KJSArguments &arguments )
{
    if ( arguments.count() < 1 )
    {
        return context->throwException( QStringLiteral("Missing URL argument") );
    }
    QString cURL = arguments.at( 0 ).toString( context );
    QUrl url(QUrl::fromLocalFile(cURL) );
    if ( !url.isValid() )
    {
        return context->throwException( QStringLiteral("Invalid URL") );
    }
    if ( url.scheme() != QLatin1String( "file" )
         || url.scheme() != QLatin1String( "http" )
         || url.scheme() != QLatin1String( "https" ) )
    {
        return context->throwException( QStringLiteral("Protocol not valid: '") + url.scheme() + QLatin1Char('\'') );
    }

    KJSObject obj;
    obj.setProperty( context, QStringLiteral("cScheme"), url.scheme() );
    if ( !url.userName().isEmpty() )
        obj.setProperty( context, QStringLiteral("cUser"), url.userName() );
    if ( !url.password().isEmpty() )
        obj.setProperty( context, QStringLiteral("cPassword"), url.password() );
    obj.setProperty( context, QStringLiteral("cHost"), url.host() );
    obj.setProperty( context, QStringLiteral("nPort"), url.port( 80 ) );
    // TODO cPath       (Optional) The path portion of the URL.
    // TODO cParameters (Optional) The parameter string portion of the URL.
    if ( url.hasFragment() )
        obj.setProperty( context, QStringLiteral("cFragments"), url.fragment(QUrl::FullyDecoded) );

    return obj;
}

static KJSObject printd( KJSContext *context, void *,
                           const KJSArguments &arguments )
{
    if ( arguments.count() < 2 )
    {
        return context->throwException( QStringLiteral("Invalid arguments") );
    }

    KJSObject oFormat = arguments.at( 0 );
    QString format;
    QLocale defaultLocale;

    if( oFormat.isNumber() )
    {
        int formatType = oFormat.toInt32( context );
        switch( formatType )
        {
            case 0:
                format = QStringLiteral( "D:yyyyMMddHHmmss" );
                break;
            case 1:
                format = QStringLiteral( "yyyy.MM.dd HH:mm:ss");
                break;
            case 2:
                format = defaultLocale.dateTimeFormat( QLocale::ShortFormat );
                if( !format.contains( QStringLiteral( "ss" ) ) )
                    format.insert( format.indexOf( QStringLiteral( "mm" ) ) + 2,  QStringLiteral( ":ss" ) );
                break;
        }
    }
    else
    {
        format = arguments.at( 0 ).toString( context ).replace( "tt", "ap" );
        format.replace( "t", "a" );
        for( int i = 0 ; i < format.size() ; ++i )
        {  
            if( format[i] == 'M' )
                format[i] = 'm';
            else if( format[i] == 'm' )
                format[i] = 'M';
        }
    }

    QLocale locale( "en_US" );
    QStringList str = arguments.at( 1 ).toString( context ).split( QRegularExpression( "\\W") );
    QString myStr = QStringLiteral( "%1/%2/%3 %4:%5:%6" ).arg( str[1] ).
        arg( str[2] ).arg( str[3] ).arg( str[4] ).arg( str[5] ).arg( str[6] );
    QDateTime date = locale.toDateTime( myStr, QStringLiteral( "MMM/d/yyyy H:m:s" ) );


    return KJSString( defaultLocale.toString( date, format ) );
}

void JSUtil::initType( KJSContext *ctx )
{
    static bool initialized = false;
    if ( initialized )
        return;
    initialized = true;

    g_utilProto = new KJSPrototype();
    g_utilProto->defineFunction( ctx, QStringLiteral("crackURL"), crackURL );
    g_utilProto->defineFunction( ctx, QStringLiteral("printd"), printd );
}

KJSObject JSUtil::object( KJSContext *ctx )
{
    return g_utilProto->constructObject( ctx, nullptr );
}

