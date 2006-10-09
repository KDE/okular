/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_SOUND_H_
#define _OKULAR_SOUND_H_

#include "okular_export.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Okular {

/**
 * @short A sound.
 *
 * ...
 */
class OKULAR_EXPORT Sound
{
    public:
        enum SoundType {
            External,
            Embedded
        };

        enum SoundEncoding {
            Raw,
            Signed,
            muLaw,
            ALaw
        };

        Sound( const QByteArray& data );
        Sound( const QString& data );

        ~Sound();

        SoundType soundType() const;

        QString url() const;

        QByteArray data() const;

        double samplingRate() const;
        void setSamplingRate( double sr );

        int channels() const;
        void setChannels( int ch );

        int bitsPerSample() const;
        void setBitsPerSample( int bps );

        SoundEncoding soundEncoding() const;
        void setSoundEncoding( SoundEncoding se );

    private:
        void init();

        QVariant m_data;
        Sound::SoundType m_type;
        double m_samplingRate;
        int m_channels;
        int m_bitsPerSample;
        SoundEncoding m_soundEncoding;
};

}

#endif
