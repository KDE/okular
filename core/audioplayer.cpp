/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "audioplayer.h"
#include "audioplayer_p.h"

// qt/kde includes
#include <qbuffer.h>
#include <qdir.h>
#include <kdebug.h>
#include <krandom.h>
#include <phonon/audiopath.h>
#include <phonon/audiooutput.h>
#include <phonon/abstractmediastream.h>
#include <phonon/mediaobject.h>

// local includes
#include "action.h"
#include "sound.h"

using namespace Okular;

// helper class used to store info about a sound to be played
class SoundInfo
{
public:
    explicit SoundInfo( const Sound * s = 0, const SoundAction * ls = 0 )
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


class PlayData
{
public:
    PlayData()
        : m_mediaobject( 0 ), m_path( 0 ), m_output( 0 ), m_buffer( 0 )
    {
    }

    void play()
    {
        if ( m_buffer )
        {
            m_buffer->open( QIODevice::ReadOnly );
        }
        m_mediaobject->play();
    }

    ~PlayData()
    {
        m_mediaobject->stop();
        delete m_mediaobject;
        delete m_path;
        delete m_output;
        delete m_buffer;
    }

    Phonon::MediaObject * m_mediaobject;
    Phonon::AudioPath * m_path;
    Phonon::AudioOutput * m_output;
    QBuffer * m_buffer;
    SoundInfo m_info;
};


AudioPlayerPrivate::AudioPlayerPrivate( AudioPlayer * qq )
    : q( qq )
{
    QObject::connect( &m_mapper, SIGNAL( mapped( int ) ), q, SLOT( finished( int ) ) );
}

AudioPlayerPrivate::~AudioPlayerPrivate()
{
    stopPlayings();
}

int AudioPlayerPrivate::newId() const
{
    int newid = 0;
    QHash< int, PlayData * >::const_iterator it;
    QHash< int, PlayData * >::const_iterator itEnd = m_playing.constEnd();
    do
    {
        newid = KRandom::random();
        it = m_playing.constFind( newid );
    } while ( it != itEnd );
    return newid;
}

bool AudioPlayerPrivate::play( const SoundInfo& si )
{
    kDebug() << k_funcinfo << endl;
    PlayData * data = new PlayData();
    data->m_output = new Phonon::AudioOutput( Phonon::NotificationCategory );
    data->m_output->setVolume( si.volume );
    data->m_path = new Phonon::AudioPath();
    data->m_path->addOutput( data->m_output );
    data->m_mediaobject = new Phonon::MediaObject();
    data->m_info = si;
    bool valid = false;

    switch ( si.sound->soundType() )
    {
        case Sound::External:
        {
            QString url = si.sound->url();
            kDebug() << "[AudioPlayer::Playinfo::play()] External, " << url << endl;
            if ( !url.isEmpty() )
            {
                if ( data->m_mediaobject->addAudioPath( data->m_path ) )
                {
                    int newid = newId();
                    m_mapper.setMapping( data->m_mediaobject, newid );
                    KUrl newurl;
                    if ( KUrl::isRelativeUrl( url ) )
                    {
                        newurl = m_currentDocument;
                        newurl.setFileName( url );
                    }
                    else
                    {
                        newurl = url;
                    }
                    data->m_mediaobject->setCurrentSource( newurl );
                    m_playing.insert( newid, data );
                    valid = true;
                    kDebug() << "[AudioPlayer::Playinfo::play()] PLAY url" << endl;
                }
            }
            break;
        }
        case Sound::Embedded:
        {
#if 0 // disable because of broken bytestream in xine :(
            QByteArray filedata = si.sound->data();
            kDebug() << "[AudioPlayer::Playinfo::play()] Embedded, " << filedata.length() << endl;
            if ( !filedata.isEmpty() )
            {
                kDebug() << "[AudioPlayer::Playinfo::play()] mediaobject: " << data->m_mediaobject << endl;
                if ( data->m_mediaobject->addAudioPath( data->m_path ) )
                {
                    int newid = newId();
                    m_mapper.setMapping( data->m_mediaobject, newid );
                    data->m_buffer = new QBuffer();
                    data->m_buffer->setData( filedata );
                    data->m_mediaobject->setCurrentSource( Phonon::MediaSource( data->m_buffer ) );
                    m_playing.insert( newid, data );
                    valid = true;
                    kDebug() << "[AudioPlayer::Playinfo::play()] PLAY data" << endl;
                }
            }
#endif
            break;
        }
    }
    if ( !valid )
    {
        delete data;
        data = 0;
    }
    if ( data )
    {
        QObject::connect( data->m_mediaobject, SIGNAL( finished() ), &m_mapper, SLOT( map() ) );
        data->play();
    }
    return valid;
}

void AudioPlayerPrivate::stopPlayings()
{
    qDeleteAll( m_playing );
    m_playing.clear();
}

void AudioPlayerPrivate::finished( int id )
{
    QHash< int, PlayData * >::iterator it = m_playing.find( id );
    if ( it == m_playing.end() )
        return;

    SoundInfo si = it.value()->m_info;
    // if the sound must be repeated indefinitely, then start the playback
    // again, otherwise destroy the PlayData as it's no more useful
    if ( si.repeat )
    {
        it.value()->play();
    }
    else
    {
        delete it.value();
        m_playing.erase( it );
    }
    kDebug() << k_funcinfo << "finished, " << m_playing.count() << endl;
}


AudioPlayer::AudioPlayer()
  : QObject(), d( new AudioPlayerPrivate( this ) )
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

void AudioPlayer::playSound( const Sound * sound, const SoundAction * linksound )
{
    // we can't play null pointers ;)
    if ( !sound )
        return;

    // we don't play external sounds for remote documents
    if ( sound->soundType() == Sound::External && !d->m_currentDocument.isLocalFile() )
        return;

    kDebug() << k_funcinfo << endl;
    SoundInfo si( sound, linksound );

    // if the mix flag of the new sound is false, then the currently playing
    // sounds must be stopped.
    if ( !si.mix )
        d->stopPlayings();

    d->play( si );
}

void AudioPlayer::stopPlaybacks()
{
    d->stopPlayings();
}

#include "audioplayer.moc"
