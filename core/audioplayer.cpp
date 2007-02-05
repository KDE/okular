/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <qhash.h>
#include <qsignalmapper.h>
#include <kdebug.h>
#include <krandom.h>
#include <phonon/audiopath.h>
#include <phonon/audiooutput.h>
#include <phonon/bytestream.h>
#include <phonon/mediaobject.h>

// local includes
#include "audioplayer.h"
#include "link.h"
#include "sound.h"

using namespace Okular;

// helper class used to store info about a sound to be played
class SoundInfo
{
public:
    explicit SoundInfo( const Sound * s = 0, const LinkSound * ls = 0 )
      : sound( s ), volume( 0.5 ), synchronous( false ), repeat( false ),
        mix( false )
    {
        if ( ls )
        {
            volume = ls->volume();
            synchronous = ls->synchronous();
            repeat = ls->repeat();
            mix = ls->mix();
        }
    }

    const Sound * sound;
    double volume;
    bool synchronous;
    bool repeat;
    bool mix;
};


class PlayInfo
{
public:
    PlayInfo()
        : m_mediaobject( 0 ), m_bytestream( 0 ), m_path( 0 ), m_output( 0 )
    {
    }

    ~PlayInfo()
    {
        if ( m_bytestream )
        {
            m_bytestream->stop();
            delete m_bytestream;
        }
        if ( m_mediaobject )
        {
            m_mediaobject->stop();
            delete m_mediaobject;
        }
        delete m_path;
        delete m_output;
    }

    Phonon::MediaObject * m_mediaobject;
    Phonon::ByteStream * m_bytestream;
    Phonon::AudioPath * m_path;
    Phonon::AudioOutput * m_output;
    SoundInfo m_info;
};


class AudioPlayer::Private
{
public:
    Private( AudioPlayer * qq )
      : q( qq )
    {
        connect( &m_mapper, SIGNAL( mapped( int ) ), q, SLOT( finished( int ) ) );
    }

    ~Private()
    {
        stopPlayings();
    }

    int newId() const;
    bool play( const SoundInfo& si );
    void stopPlayings();

    // private slots
    void finished( int );

    AudioPlayer * q;

    QHash< int, PlayInfo * > m_playing;
    QSignalMapper m_mapper;
};

int AudioPlayer::Private::newId() const
{
    int newid = 0;
    QHash< int, PlayInfo * >::const_iterator it;
    QHash< int, PlayInfo * >::const_iterator itEnd = m_playing.constEnd();
    do
    {
        newid = KRandom::random();
        it = m_playing.constFind( newid );
    } while ( it != itEnd );
    return newid;
}

bool AudioPlayer::Private::play( const SoundInfo& si )
{
    kDebug() << k_funcinfo << endl;
    PlayInfo * info = new PlayInfo();
    info->m_output = new Phonon::AudioOutput( Phonon::NotificationCategory );
    info->m_output->setVolume( si.volume );
    info->m_path = new Phonon::AudioPath();
    info->m_path->addOutput( info->m_output );
    info->m_info = si;
    bool valid = false;

    switch ( si.sound->soundType() )
    {
        case Sound::External:
        {
            QString url = si.sound->url();
            kDebug() << "[AudioPlayer::Playinfo::play()] External, " << url << endl;
            if ( !url.isEmpty() )
            {
                info->m_mediaobject = new Phonon::MediaObject();
                if ( info->m_mediaobject->addAudioPath( info->m_path ) )
                {
                    QObject::connect( info->m_mediaobject, SIGNAL( finished() ), &m_mapper, SLOT( map() ) );
                    int newid = newId();
                    m_mapper.setMapping( info->m_mediaobject, newid );
                    info->m_mediaobject->setUrl( url );
                    m_playing.insert( newid, info );
                    valid = true;
                    info->m_mediaobject->play();
                    kDebug() << "[AudioPlayer::Playinfo::play()] PLAY url" << endl;
                }
            }
            break;
        }
        case Sound::Embedded:
        {
#if 0 // disable because of broken bytestream in xine :(
            QByteArray data = si.sound->data();
            kDebug() << "[AudioPlayer::Playinfo::play()] Embedded, " << data.length() << endl;
            if ( !data.isEmpty() )
            {
                kDebug() << "[AudioPlayer::Playinfo::play()] bytestream: " << info->m_bytestream << endl;
                info->m_bytestream = new Phonon::ByteStream();
                kDebug() << "[AudioPlayer::Playinfo::play()] bytestream: " << info->m_bytestream << endl;
                if ( info->m_bytestream->addAudioPath( info->m_path ) )
                {
                    QObject::connect( info->m_bytestream, SIGNAL( finished() ), &m_mapper, SLOT( map() ) );
                    int newid = newId();
                    m_mapper.setMapping( info->m_mediaobject, newid );
                    m_playing.insert( newid, info );
                    info->m_bytestream->writeData( data );
                    info->m_bytestream->setStreamSize( data.length() );
                    info->m_bytestream->setStreamSeekable( true );
                    info->m_bytestream->endOfData();
                    valid = true;
                    info->m_bytestream->play();
                    kDebug() << "[AudioPlayer::Playinfo::play()] PLAY data" << endl;
                }
            }
#endif
            break;
        }
    }
    if ( !valid )
    {
        delete info;
        info = 0;
    }
    return valid;
}

void AudioPlayer::Private::stopPlayings()
{
    qDeleteAll( m_playing );
    m_playing.clear();
}

void AudioPlayer::Private::finished( int id )
{
    QHash< int, PlayInfo * >::iterator it = m_playing.find( id );
    if ( it == m_playing.end() )
        return;

    delete it.value();
    m_playing.erase( it );
    kDebug() << k_funcinfo << "finished, " << m_playing.count() << endl;
}


AudioPlayer::AudioPlayer()
  : QObject(), d( new Private( this ) )
{
}

AudioPlayer::~AudioPlayer()
{
    delete d;
}

AudioPlayer * AudioPlayer::instance()
{
    static AudioPlayer ap;
    return &ap;
}

void AudioPlayer::playSound( const Sound * sound, const LinkSound * linksound )
{
    // we can't play null pointers ;)
    if ( !sound )
        return;

    kDebug() << k_funcinfo << endl;
    SoundInfo si( sound, linksound );

    // if the mix flag of the new sound is false, then the currently playing
    // sounds must be stopped.
    if ( !si.mix )
        d->stopPlayings();

    d->play( si );
}

#include "audioplayer.moc"
