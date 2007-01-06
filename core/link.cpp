/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// kde includes
#include <klocale.h>

// local includes
#include "document.h"
#include "link.h"
#include "sound.h"

using namespace Okular;

class Link::Private
{
};

Link::Link()
    : d( 0 )
{
}

Link::~Link()
{
    delete d;
}

QString Link::linkTip() const
{
    return "";
}

// LinkGoto

class LinkGoto::Private
{
    public:
        Private( const QString &fileName, const DocumentViewport &viewport )
            : m_extFileName( fileName ), m_vp( viewport )
        {
        }

        QString m_extFileName;
        DocumentViewport m_vp;
};

LinkGoto::LinkGoto( const QString& fileName, const DocumentViewport & viewport )
    : d( new Private( fileName, viewport ) )
{
}

LinkGoto::~LinkGoto()
{
    delete d;
}

Link::LinkType LinkGoto::linkType() const
{
    return Goto;
}

QString LinkGoto::linkTip() const
{
    return d->m_extFileName.isEmpty() ? ( d->m_vp.isValid() ? i18n( "Go to page %1", d->m_vp.pageNumber + 1 ) : "" ) :
                                     i18n("Open external file");
}

bool LinkGoto::isExternal() const
{
    return !d->m_extFileName.isEmpty();
}

QString LinkGoto::fileName() const
{
    return d->m_extFileName;
}

DocumentViewport LinkGoto::destViewport() const
{
    return d->m_vp;
}

// LinkExecute

class LinkExecute::Private
{
    public:
        Private( const QString &file, const QString & parameters )
            : m_fileName( file ), m_parameters( parameters )
        {
        }

        QString m_fileName;
        QString m_parameters;
};

LinkExecute::LinkExecute( const QString &file, const QString & parameters )
    : d( new Private( file, parameters ) )
{
}

LinkExecute::~LinkExecute()
{
    delete d;
}

Link::LinkType LinkExecute::linkType() const
{
    return Execute;
}

QString LinkExecute::linkTip() const
{
    return i18n( "Execute '%1'...", d->m_fileName );
}

QString LinkExecute::fileName() const
{
    return d->m_fileName;
}

QString LinkExecute::parameters() const
{
    return d->m_parameters;
}

// BrowseLink

class LinkBrowse::Private
{
    public:
        Private( const QString &url )
            : m_url( url )
        {
        }

        QString m_url;
};

LinkBrowse::LinkBrowse( const QString &url )
    : d( new Private( url ) )
{
}

LinkBrowse::~LinkBrowse()
{
    delete d;
}

Link::LinkType LinkBrowse::linkType() const
{
    return Browse;
}

QString LinkBrowse::linkTip() const
{
    return d->m_url;
}

QString LinkBrowse::url() const
{
    return d->m_url;
}

// LinkAction

class LinkAction::Private
{
    public:
        Private( enum ActionType actionType )
            : m_type( actionType )
        {
        }

        ActionType m_type;
};

LinkAction::LinkAction( enum ActionType actionType )
    : d( new Private( actionType ) )
{
}

LinkAction::~LinkAction()
{
    delete d;
}

LinkAction::ActionType LinkAction::actionType() const
{
    return d->m_type;
}

Link::LinkType LinkAction::linkType() const
{
    return Action;
}

QString LinkAction::linkTip() const
{
    switch ( d->m_type )
    {
        case PageFirst:
            return i18n( "First Page" );
        case PagePrev:
            return i18n( "Previous Page" );
        case PageNext:
            return i18n( "Next Page" );
        case PageLast:
            return i18n( "Last Page" );
        case HistoryBack:
            return i18n( "Back" );
        case HistoryForward:
            return i18n( "Forward" );
        case Quit:
            return i18n( "Quit" );
        case Presentation:
            return i18n( "Start Presentation" );
        case EndPresentation:
            return i18n( "End Presentation" );
        case Find:
            return i18n( "Find..." );
        case GoToPage:
            return i18n( "Go To Page..." );
        case Close:
        default: ;
    }

    return QString();
}

// LinkSound

class LinkSound::Private
{
    public:
        Private( double volume, bool sync, bool repeat, bool mix, Okular::Sound *sound )
            : m_volume( volume ), m_sync( sync ), m_repeat( repeat ),
              m_mix( mix ), m_sound( sound )
        {
        }

        ~Private()
        {
            delete m_sound;
        }

        double m_volume;
        bool m_sync;
        bool m_repeat;
        bool m_mix;
        Okular::Sound *m_sound;
};

LinkSound::LinkSound( double volume, bool sync, bool repeat, bool mix, Okular::Sound *sound )
    : d( new Private( volume, sync, repeat, mix, sound ) )
{
}

LinkSound::~LinkSound()
{
    delete d;
}

Link::LinkType LinkSound::linkType() const
{
    return Sound;
}

QString LinkSound::linkTip() const
{
    return i18n( "Play sound..." );
}

double LinkSound::volume() const
{
    return d->m_volume;
}

bool LinkSound::synchronous() const
{
    return d->m_sync;
}

bool LinkSound::repeat() const
{
    return d->m_repeat;
}

bool LinkSound::mix() const
{
    return d->m_mix;
}

Okular::Sound *LinkSound::sound() const
{
    return d->m_sound;
}

// LinkMovie
class LinkMovie::Private
{
};

LinkMovie::LinkMovie()
    : d( 0 )
{
}

LinkMovie::~LinkMovie()
{
    delete d;
}

Link::LinkType LinkMovie::linkType() const
{
    return Movie;
}

QString LinkMovie::linkTip() const
{
    return i18n( "Play movie..." );
}
