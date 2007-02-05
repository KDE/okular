/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_AUDIOPLAYER_H_
#define _OKULAR_AUDIOPLAYER_H_

#include <core/okular_export.h>

#include <QtCore/QObject>

namespace Okular {

class LinkSound;
class Sound;

/**
 * @short An audio player.
 *
 * Singleton utility class to play sounds in documents using the KDE sound
 * system.
 */
class OKULAR_EXPORT AudioPlayer : public QObject
{
    Q_OBJECT

    public:
        ~AudioPlayer();

        /**
         * Gets the instance of the audio player.
         */
        static AudioPlayer * instance();

        /**
         * Enqueue the specified @p sound for playing, optionally taking more
         * information about the playing from the @p soundlink .
         */
        void playSound( const Sound * sound, const LinkSound * linksound = 0 );

    private:
        AudioPlayer();

        class Private;
        Private * const d;

        Q_DISABLE_COPY( AudioPlayer )
        Q_PRIVATE_SLOT( d, void finished( int ) )
};

}

#endif
