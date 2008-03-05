/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tts.h"

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <klocale.h>
#include <ktoolinvocation.h>

#include "pageviewutils.h"

/* Private storage. */
class OkularTTS::Private
{
public:
    Private()
    {
    }

    PageViewMessage *messageWindow;
};


OkularTTS::OkularTTS( PageViewMessage *messageWindow, QObject *parent )
    : QObject( parent ), d( new Private )
{
    d->messageWindow = messageWindow;
}

OkularTTS::~OkularTTS()
{
    delete d;
}

void OkularTTS::say( const QString &text )
{
    if ( text.isEmpty() )
        return;

    // Albert says is this ever necessary?
    // we already attached on Part constructor
    // If KTTSD not running, start it.
    QDBusReply<bool> reply = QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.kttsd" );
    bool kttsdactive = false;
    if ( reply.isValid() )
        kttsdactive = reply.value();
    if ( !kttsdactive )
    {
        QString error;
        if ( KToolInvocation::startServiceByDesktopName( "kttsd", QStringList(), &error ) )
        {
            d->messageWindow->display( i18n( "Starting KTTSD Failed: %1", error ) );
        }
        else
        {
            kttsdactive = true;
        }
    }
    if ( kttsdactive )
    {
        // creating the connection to the kspeech interface
        QDBusInterface kspeech( "org.kde.kttsd", "/KSpeech", "org.kde.KSpeech" );
        kspeech.call( "setApplicationName", "Okular" );
        kspeech.call( "say", text, 0 );
    }
}

#include "tts.moc"
