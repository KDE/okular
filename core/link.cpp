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
#include "link.h"

using namespace Okular;

Link::~Link()
{
}

QString Link::linkTip() const
{
    return "";
}

// LinkGoto
LinkGoto::LinkGoto( QString fileName, const DocumentViewport & viewport )
    : m_extFileName( fileName ), m_vp( viewport )
{
}

Link::LinkType LinkGoto::linkType() const
{
    return Goto;
}

QString LinkGoto::linkTip() const
{
    return m_extFileName.isEmpty() ? ( m_vp.isValid() ? i18n( "Go to page %1", m_vp.pageNumber + 1 ) : "" ) :
                                     i18n("Open external file");
}

bool LinkGoto::isExternal() const
{
    return !m_extFileName.isEmpty();
}

QString LinkGoto::fileName() const
{
    return m_extFileName;
}

DocumentViewport LinkGoto::destViewport() const
{
    return m_vp;
}

// LinkExecute
LinkExecute::LinkExecute( const QString &file, const QString & parameters )
    : m_fileName( file ), m_parameters( parameters )
{
}

Link::LinkType LinkExecute::linkType() const
{
    return Execute;
}

QString LinkExecute::linkTip() const
{
    return i18n( "Execute '%1'...", m_fileName );
}

QString LinkExecute::fileName() const
{
    return m_fileName;
}

QString LinkExecute::parameters() const
{
    return m_parameters;
}

// BrowseLink
LinkBrowse::LinkBrowse( const QString &url )
    : m_url( url )
{
}

Link::LinkType LinkBrowse::linkType() const
{
    return Browse;
}

QString LinkBrowse::linkTip() const
{
    return i18n( "Browse '%1'...", m_url );
}

QString LinkBrowse::url() const
{
    return m_url;
}

// LinkAction
LinkAction::LinkAction( enum ActionType actionType )
    : m_type( actionType )
{
}

LinkAction::ActionType LinkAction::actionType() const
{
    return m_type;
}

Link::LinkType LinkAction::linkType() const
{
    return Action;
}

QString LinkAction::linkTip() const
{
    switch ( m_type )
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
LinkSound::LinkSound( double volume, bool sync, bool repeat, bool mix, Okular::Sound *sound )
    : m_volume( volume ), m_sync( sync ), m_repeat( repeat ),
      m_mix( mix ), m_sound( sound )
{
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
    return m_volume;
}

bool LinkSound::synchronous() const
{
    return m_sync;
}

bool LinkSound::repeat() const
{
    return m_repeat;
}

bool LinkSound::mix() const
{
    return m_mix;
}

Okular::Sound *LinkSound::sound() const
{
    return m_sound;
}

// LinkMovie
LinkMovie::LinkMovie()
{
}

Link::LinkType LinkMovie::linkType() const
{
    return Movie;
}

QString LinkMovie::linkTip() const
{
    return i18n( "Play movie..." );
}
