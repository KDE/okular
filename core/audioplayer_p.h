/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_AUDIOPLAYER_P_H_
#define _OKULAR_AUDIOPLAYER_P_H_

// qt/kde includes
#include <qhash.h>
#include <qsignalmapper.h>
#include <kurl.h>

class QBuffer;
class PlayData;
class SoundInfo;

namespace Okular {

class AudioPlayer;

class AudioPlayerPrivate
{
public:
    AudioPlayerPrivate( AudioPlayer * qq );

    ~AudioPlayerPrivate();

    int newId() const;
    bool play( const SoundInfo& si );
    void stopPlayings();

    // private slots
    void finished( int );

    AudioPlayer * q;

    QHash< int, PlayData * > m_playing;
    QSignalMapper m_mapper;
    KUrl m_currentDocument;
};

}

#endif
