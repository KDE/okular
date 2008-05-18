/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *   Copyright (C) 2008 by Harri Porten <porten@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kjs_app_p.h"

#include <kjs/kjsarguments.h>
#include <kjs/kjsobject.h>
#include <kjs/kjsprototype.h>

#include <qapplication.h>

#include <kglobal.h>
#include <klocale.h>

#include "../document_p.h"
#include "kjs_fullscreen_p.h"

using namespace Okular;

static KJSPrototype *g_appProto;

// the acrobat version we fake
static const double fake_acroversion = 8.00;

static const struct FakePluginInfo {
    const char *name;
    bool certified;
    bool loaded;
    const char *path;
} s_fake_plugins[] = {
    { "Annots", true, true, "" },
    { "EFS", true, true, "" },
    { "EScript", true, true, "" },
    { "Forms", true, true, "" },
    { "ReadOutLoud", true, true, "" },
    { "WebLink", true, true, "" }
};
static const int s_num_fake_plugins = sizeof( s_fake_plugins ) / sizeof( s_fake_plugins[0] );


static KJSObject appGetFormsVersion( KJSContext *, void * )
{
    // faking a bit...
    return KJSNumber( fake_acroversion );
}

static KJSObject appGetLanguage( KJSContext *, void * )
{
    QString lang;
    QString country;
    QString dummy;
    KLocale::splitLocale( KGlobal::locale()->language(),
                          lang, country, dummy, dummy );
    QString acroLang = QString::fromLatin1( "ENU" );
    if ( lang == QLatin1String( "da" ) )
        acroLang = QString::fromLatin1( "DAN" ); // Danish
    else if ( lang == QLatin1String( "de" ) )
        acroLang = QString::fromLatin1( "DEU" ); // German
    else if ( lang == QLatin1String( "en" ) )
        acroLang = QString::fromLatin1( "ENU" ); // English
    else if ( lang == QLatin1String( "es" ) )
        acroLang = QString::fromLatin1( "ESP" ); // Spanish
    else if ( lang == QLatin1String( "fr" ) )
        acroLang = QString::fromLatin1( "FRA" ); // French
    else if ( lang == QLatin1String( "it" ) )
        acroLang = QString::fromLatin1( "ITA" ); // Italian
    else if ( lang == QLatin1String( "ko" ) )
        acroLang = QString::fromLatin1( "KOR" ); // Korean
    else if ( lang == QLatin1String( "ja" ) )
        acroLang = QString::fromLatin1( "JPN" ); // Japanese
    else if ( lang == QLatin1String( "nl" ) )
        acroLang = QString::fromLatin1( "NLD" ); // Dutch
    else if ( lang == QLatin1String( "pt" ) && country == QLatin1String( "BR" ) )
        acroLang = QString::fromLatin1( "PTB" ); // Brazilian Portuguese
    else if ( lang == QLatin1String( "fi" ) )
        acroLang = QString::fromLatin1( "SUO" ); // Finnish
    else if ( lang == QLatin1String( "sv" ) )
        acroLang = QString::fromLatin1( "SVE" ); // Swedish
    else if ( lang == QLatin1String( "zh" ) && country == QLatin1String( "CN" ) )
        acroLang = QString::fromLatin1( "CHS" ); // Chinese Simplified
    else if ( lang == QLatin1String( "zh" ) && country == QLatin1String( "TW" ) )
        acroLang = QString::fromLatin1( "CHT" ); // Chinese Traditional
    return KJSString( acroLang );
}

static KJSObject appGetNumPlugins( KJSContext *, void * )
{
    return KJSNumber( s_num_fake_plugins );
}

static KJSObject appGetPlatform( KJSContext *, void * )
{
#if defined(Q_OS_WIN)
    return KJSString( QString::fromLatin1( "WIN" ) );
#elif defined(Q_OS_MAC)
    return KJSString( QString::fromLatin1( "MAC" ) );
#else
    return KJSString( QString::fromLatin1( "UNIX" ) );
#endif
}

static KJSObject appGetPlugIns( KJSContext *context, void * )
{
    KJSArray plugins( context, s_num_fake_plugins );
    for ( int i = 0; i < s_num_fake_plugins; ++i )
    {
        const FakePluginInfo &info = s_fake_plugins[i];
        KJSObject plugin;
        plugin.setProperty( context, "certified", info.certified );
        plugin.setProperty( context, "loaded", info.loaded );
        plugin.setProperty( context, "name", info.name );
        plugin.setProperty( context, "path", info.path );
        plugin.setProperty( context, "version", fake_acroversion );
        plugins.setProperty( context, QString::number( i ), plugin );
    }
    return plugins;
}

static KJSObject appGetPrintColorProfiles( KJSContext *context, void * )
{
    return KJSArray( context, 0 );
}

static KJSObject appGetPrinterNames( KJSContext *context, void * )
{
    return KJSArray( context, 0 );
}

static KJSObject appGetViewerType( KJSContext *, void * )
{
    // faking a bit...
    return KJSString( QString::fromLatin1( "Reader" ) );
}

static KJSObject appGetViewerVariation( KJSContext *, void * )
{
    // faking a bit...
    return KJSString( QString::fromLatin1( "Reader" ) );
}

static KJSObject appGetViewerVersion( KJSContext *, void * )
{
    // faking a bit...
    return KJSNumber( fake_acroversion );
}

static KJSObject appBeep( KJSContext *context, void *,
                          const KJSArguments &arguments )
{
    if ( arguments.count() < 1 )
    {
        return context->throwException( "Missing beep type" );
    }
    QApplication::beep();
    return KJSUndefined();
}

static KJSObject appGetNthPlugInName( KJSContext *context, void *,
                                      const KJSArguments &arguments )
{
    if ( arguments.count() < 1 )
    {
        return context->throwException( "Missing plugin index" );
    }
    const int nIndex = arguments.at( 0 ).toInt32( context );

    if ( nIndex < 0 || nIndex >= s_num_fake_plugins )
        return context->throwException( "PlugIn index out of bounds" );

    const FakePluginInfo &info = s_fake_plugins[nIndex];
    return KJSString( info.name );
}

static KJSObject appGoBack( KJSContext *, void *object,
                            const KJSArguments & )
{
    const DocumentPrivate *doc = reinterpret_cast< DocumentPrivate * >( object );
    if ( doc->m_parent->historyAtBegin() )
        return KJSUndefined();

    doc->m_parent->setPrevViewport();
    return KJSUndefined();
}

static KJSObject appGoForward( KJSContext *, void *object,
                               const KJSArguments & )
{
    const DocumentPrivate *doc = reinterpret_cast< DocumentPrivate * >( object );
    if ( doc->m_parent->historyAtEnd() )
        return KJSUndefined();

    doc->m_parent->setNextViewport();
    return KJSUndefined();
}

void JSApp::initType( KJSContext *ctx )
{
    static bool initialized = false;
    if ( initialized )
        return;
    initialized = true;

    g_appProto = new KJSPrototype();

    g_appProto->defineProperty( ctx, "formsVersion", appGetFormsVersion );
    g_appProto->defineProperty( ctx, "language", appGetLanguage );
    g_appProto->defineProperty( ctx, "numPlugIns", appGetNumPlugins );
    g_appProto->defineProperty( ctx, "platform", appGetPlatform );
    g_appProto->defineProperty( ctx, "plugIns", appGetPlugIns );
    g_appProto->defineProperty( ctx, "printColorProfiles", appGetPrintColorProfiles );
    g_appProto->defineProperty( ctx, "printerNames", appGetPrinterNames );
    g_appProto->defineProperty( ctx, "viewerType", appGetViewerType );
    g_appProto->defineProperty( ctx, "viewerVariation", appGetViewerVariation );
    g_appProto->defineProperty( ctx, "viewerVersion", appGetViewerVersion );

    g_appProto->defineFunction( ctx, "beep", appBeep );
    g_appProto->defineFunction( ctx, "getNthPlugInName", appGetNthPlugInName );
    g_appProto->defineFunction( ctx, "goBack", appGoBack );
    g_appProto->defineFunction( ctx, "goForward", appGoForward );
}

KJSObject JSApp::object( KJSContext *ctx, DocumentPrivate *doc )
{
    return g_appProto->constructObject( ctx, doc );
}
