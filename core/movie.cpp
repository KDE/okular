/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "movie.h"

// qt/kde includes
#include <qstring.h>

using namespace Okular;

class Movie::Private
{
    public:
        Private( const QString &url )
            : m_url( url ),
              m_rotation( Rotation0 ),
              m_playMode( PlayOnce ),
              m_showControls( false )
        {
        }

        QString m_url;
        QSize m_aspect;
        Rotation m_rotation;
        PlayMode m_playMode;
        bool m_showControls : 1;
};

Movie::Movie( const QString& fileName )
    : d( new Private( fileName ) )
{
}

Movie::~Movie()
{
    delete d;
}

QString Movie::url() const
{
    return d->m_url;
}

void Movie::setSize( const QSize &aspect )
{
    d->m_aspect = aspect;
}

QSize Movie::size() const
{
    return d->m_aspect;
}

void Movie::setRotation( Rotation rotation )
{
    d->m_rotation = rotation;
}

Rotation Movie::rotation() const
{
    return d->m_rotation;
}

void Movie::setShowControls( bool show )
{
    d->m_showControls = show;
}

bool Movie::showControls() const
{
    return d->m_showControls;
}

void Movie::setPlayMode( Movie::PlayMode mode )
{
    d->m_playMode = mode;
}

Movie::PlayMode Movie::playMode() const
{
    return d->m_playMode;
}
