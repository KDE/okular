/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// local includes
#include "sound.h"

using namespace Okular;

Sound::Sound( const QByteArray& data )
{
    m_type = Sound::Embedded;
    m_data = QVariant( data );
    init();
}

Sound::Sound( const QString& file )
{
    m_type = Sound::External;
    m_data = QVariant( file );
    init();
}

void Sound::init()
{
    m_samplingRate = 44100.0;
    m_channels = 1;
    m_bitsPerSample = 8;
    m_soundEncoding = Sound::Raw;
}

Sound::~Sound()
{
}

Sound::SoundType Sound::soundType() const
{
    return m_type;
}

QString Sound::url() const
{
    return m_type == Sound::External ? m_data.toString() : QString();
}

QByteArray Sound::data() const
{
    return m_type == Sound::Embedded ? m_data.toByteArray() : QByteArray();
};

double Sound::samplingRate() const
{
    return m_samplingRate;
}

void Sound::setSamplingRate( double sr )
{
    m_samplingRate = sr;
}

int Sound::channels() const
{
    return m_channels;
}

void Sound::setChannels( int ch )
{
    m_channels = ch;
}

int Sound::bitsPerSample() const
{
    return m_bitsPerSample;
}

void Sound::setBitsPerSample( int bps )
{
    m_bitsPerSample = bps;
}

Sound::SoundEncoding Sound::soundEncoding() const
{
    return m_soundEncoding;
}

void Sound::setSoundEncoding( Sound::SoundEncoding se )
{
    m_soundEncoding = se;
}
