/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "tts.h"

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
    {
    }

    void setupIface();
    void teardownIface();

    OkularTTS *q;
    org::kde::KSpeech* kspeech;
    QSet< int > jobs;
};

void OkularTTS::Private::setupIface()
{
    if ( kspeech )
        return;

    // If Jovie not running, start it.
    QDBusReply<bool> reply = QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.KSpeech" );
    bool jovieactive = false;
    if ( reply.isValid() )
        jovieactive = reply.value();
    if ( !jovieactive )
    {
        QString error;
        if ( KToolInvocation::startServiceByDesktopName( "jovie", QStringList(), &error ) )
        {
            emit q->errorMessage( i18n( "Starting Jovie Failed: %1", error ) );
        }
        else
        {
            jovieactive = true;
        }
    }
    if ( jovieactive )
    {
        // creating the connection to the kspeech interface
        kspeech = new org::kde::KSpeech( "org.kde.KSpeech", "/KSpeech", QDBusConnection::sessionBus() );
        kspeech->setParent( q );
        kspeech->setApplicationName( "Okular" );
        connect( kspeech, SIGNAL( jobStateChanged( const QString &, int, int ) ),
                 q, SLOT( slotJobStateChanged( const QString &, int, int ) ) );
        connect( QDBusConnection::sessionBus().interface(), SIGNAL( serviceUnregistered( const QString & ) ),
                 q, SLOT( slotServiceUnregistered( const QString & ) ) );
        connect( QDBusConnection::sessionBus().interface(), SIGNAL( serviceOwnerChanged( const QString &, const QString &, const QString & ) ),
                 q, SLOT( slotServiceOwnerChanged( const QString &, const QString &, const QString & ) ) );
    }
}

void OkularTTS::Private::teardownIface()
{
    disconnect( QDBusConnection::sessionBus().interface(), 0, q, 0 );

    delete kspeech;
    kspeech = 0;
}


OkularTTS::OkularTTS( QObject *parent )
    : QObject( parent ), d( new Private( this ) )
{
}

OkularTTS::~OkularTTS()
{
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
    if ( service == QLatin1String( "org.kde.KSpeech" ) )
    {
        d->teardownIface();
    }
}

void OkularTTS::slotServiceOwnerChanged( const QString &service, const QString &, const QString &newOwner )
{
    if ( service == QLatin1String( "org.kde.KSpeech" ) && newOwner.isEmpty() )
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
