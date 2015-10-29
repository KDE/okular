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

#include <QLocale>

#include <KLocalizedString>

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
    QLocale locale;
    QString lang = QLocale::languageToString(locale.language());
    QString country = QLocale::countryToString(locale.country());
    QString acroLang = QStringLiteral( "ENU" );
    if ( lang == QLatin1String( "da" ) )
        acroLang = QStringLiteral( "DAN" ); // Danish
    else if ( lang == QLatin1String( "de" ) )
        acroLang = QStringLiteral( "DEU" ); // German
    else if ( lang == QLatin1String( "en" ) )
        acroLang = QStringLiteral( "ENU" ); // English
    else if ( lang == QLatin1String( "es" ) )
        acroLang = QStringLiteral( "ESP" ); // Spanish
    else if ( lang == QLatin1String( "fr" ) )
        acroLang = QStringLiteral( "FRA" ); // French
    else if ( lang == QLatin1String( "it" ) )
        acroLang = QStringLiteral( "ITA" ); // Italian
    else if ( lang == QLatin1String( "ko" ) )
        acroLang = QStringLiteral( "KOR" ); // Korean
    else if ( lang == QLatin1String( "ja" ) )
        acroLang = QStringLiteral( "JPN" ); // Japanese
    else if ( lang == QLatin1String( "nl" ) )
        acroLang = QStringLiteral( "NLD" ); // Dutch
    else if ( lang == QLatin1String( "pt" ) && country == QLatin1String( "BR" ) )
        acroLang = QStringLiteral( "PTB" ); // Brazilian Portuguese
    else if ( lang == QLatin1String( "fi" ) )
        acroLang = QStringLiteral( "SUO" ); // Finnish
    else if ( lang == QLatin1String( "sv" ) )
        acroLang = QStringLiteral( "SVE" ); // Swedish
    else if ( lang == QLatin1String( "zh" ) && country == QLatin1String( "CN" ) )
        acroLang = QStringLiteral( "CHS" ); // Chinese Simplified
    else if ( lang == QLatin1String( "zh" ) && country == QLatin1String( "TW" ) )
        acroLang = QStringLiteral( "CHT" ); // Chinese Traditional
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
    return KJSString( QLatin1String( "UNIX" ) );
#endif
}

static KJSObject appGetPlugIns( KJSContext *context, void * )
{
    KJSArray plugins( context, s_num_fake_plugins );
    for ( int i = 0; i < s_num_fake_plugins; ++i )
    {
        const FakePluginInfo &info = s_fake_plugins[i];
        KJSObject plugin;
        plugin.setProperty( context, QStringLiteral("certified"), info.certified );
        plugin.setProperty( context, QStringLiteral("loaded"), info.loaded );
        plugin.setProperty( context, QStringLiteral("name"), info.name );
        plugin.setProperty( context, QStringLiteral("path"), info.path );
        plugin.setProperty( context, QStringLiteral("version"), fake_acroversion );
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
    return KJSString( QLatin1String( "Reader" ) );
}

static KJSObject appGetViewerVariation( KJSContext *, void * )
{
    // faking a bit...
    return KJSString( QLatin1String( "Reader" ) );
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
        return context->throwException( QStringLiteral("Missing beep type") );
    }
    QApplication::beep();
    return KJSUndefined();
}

static KJSObject appGetNthPlugInName( KJSContext *context, void *,
                                      const KJSArguments &arguments )
{
    if ( arguments.count() < 1 )
    {
        return context->throwException( QStringLiteral("Missing plugin index") );
    }
    const int nIndex = arguments.at( 0 ).toInt32( context );

    if ( nIndex < 0 || nIndex >= s_num_fake_plugins )
        return context->throwException( QStringLiteral("PlugIn index out of bounds") );

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

    g_appProto->defineProperty( ctx, QStringLiteral("formsVersion"), appGetFormsVersion );
    g_appProto->defineProperty( ctx, QStringLiteral("language"), appGetLanguage );
    g_appProto->defineProperty( ctx, QStringLiteral("numPlugIns"), appGetNumPlugins );
    g_appProto->defineProperty( ctx, QStringLiteral("platform"), appGetPlatform );
    g_appProto->defineProperty( ctx, QStringLiteral("plugIns"), appGetPlugIns );
    g_appProto->defineProperty( ctx, QStringLiteral("printColorProfiles"), appGetPrintColorProfiles );
    g_appProto->defineProperty( ctx, QStringLiteral("printerNames"), appGetPrinterNames );
    g_appProto->defineProperty( ctx, QStringLiteral("viewerType"), appGetViewerType );
    g_appProto->defineProperty( ctx, QStringLiteral("viewerVariation"), appGetViewerVariation );
    g_appProto->defineProperty( ctx, QStringLiteral("viewerVersion"), appGetViewerVersion );

    g_appProto->defineFunction( ctx, QStringLiteral("beep"), appBeep );
    g_appProto->defineFunction( ctx, QStringLiteral("getNthPlugInName"), appGetNthPlugInName );
    g_appProto->defineFunction( ctx, QStringLiteral("goBack"), appGoBack );
    g_appProto->defineFunction( ctx, QStringLiteral("goForward"), appGoForward );
}

KJSObject JSApp::object( KJSContext *ctx, DocumentPrivate *doc )
{
    return g_appProto->constructObject( ctx, doc );
}
