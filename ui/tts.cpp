/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tts.h"

#include <qdbusservicewatcher.h>
#include <qset.h>

#include <klocale.h>
#include <kspeech.h>
#include <ktoolinvocation.h>

#include "kspeechinterface.h"

/* Private storage. */
class OkularTTS::Private
{
public:
    Private( OkularTTS *qq )
        : q( qq ), kspeech( 0 )
        , watcher( "org.kde.kttsd", QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration )
    {
    }

    void setupIface();
    void teardownIface();

    OkularTTS *q;
    org::kde::KSpeech* kspeech;
    QSet< int > jobs;
    QDBusServiceWatcher watcher;
};

void OkularTTS::Private::setupIface()
{
    if ( kspeech )
        return;

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
            emit q->errorMessage( i18n( "Starting Jovie Text-to-Speech service Failed: %1", error ) );
        }
        else
        {
            kttsdactive = true;
        }
    }
    if ( kttsdactive )
    {
        // creating the connection to the kspeech interface
        kspeech = new org::kde::KSpeech( "org.kde.kttsd", "/KSpeech", QDBusConnection::sessionBus() );
        kspeech->setParent( q );
        kspeech->setApplicationName( "Okular" );
        connect( kspeech, SIGNAL(jobStateChanged(QString,int,int)),
                 q, SLOT(slotJobStateChanged(QString,int,int)) );
    }
}

void OkularTTS::Private::teardownIface()
{
    delete kspeech;
    kspeech = 0;
}


OkularTTS::OkularTTS( QObject *parent )
    : QObject( parent ), d( new Private( this ) )
{
    connect( &d->watcher, SIGNAL(serviceUnregistered(QString)),
             this, SLOT(slotServiceUnregistered(QString)) );
}

OkularTTS::~OkularTTS()
{
    disconnect( &d->watcher, 0, this, 0 );

    delete d;
}

void OkularTTS::say( const QString &text )
{
    if ( text.isEmpty() )
        return;

    d->setupIface();
    if ( d->kspeech )
    {
        QDBusReply< int > reply = d->kspeech->say( text, KSpeech::soPlainText );
        if ( reply.isValid() )
        {
            d->jobs.insert( reply.value() );
            emit hasSpeechs( true );
        }
    }
}

void OkularTTS::stopAllSpeechs()
{
    if ( !d->kspeech )
        return;

    d->kspeech->removeAllJobs();
}

void OkularTTS::slotServiceUnregistered( const QString &service )
{
    if ( service == QLatin1String( "org.kde.kttsd" ) )
    {
        d->teardownIface();
    }
}

void OkularTTS::slotJobStateChanged( const QString &appId, int jobNum, int state )
{
    // discard non ours job
    if ( appId != QDBusConnection::sessionBus().baseService() || !d->kspeech )
        return;

    switch ( state )
    {
        case KSpeech::jsDeleted:
            d->jobs.remove( jobNum );
            emit hasSpeechs( !d->jobs.isEmpty() );
            break;
        case KSpeech::jsFinished:
            d->kspeech->removeJob( jobNum );
            break;
    }
}

#include "tts.moc"
