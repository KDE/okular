/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "action.h"

// kde includes
#include <klocale.h>

// local includes
#include "document.h"
#include "sound.h"

using namespace Okular;

class Action::Private
{
};

Action::Action()
    : d( 0 )
{
}

Action::~Action()
{
    delete d;
}

QString Action::actionTip() const
{
    return "";
}

// ActionGoto

class ActionGoto::Private
{
    public:
        Private( const QString &fileName, const DocumentViewport &viewport )
            : m_extFileName( fileName ), m_vp( viewport )
        {
        }

        QString m_extFileName;
        DocumentViewport m_vp;
};

ActionGoto::ActionGoto( const QString& fileName, const DocumentViewport & viewport )
    : d( new Private( fileName, viewport ) )
{
}

ActionGoto::~ActionGoto()
{
    delete d;
}

Action::ActionType ActionGoto::actionType() const
{
    return Goto;
}

QString ActionGoto::actionTip() const
{
    return d->m_extFileName.isEmpty() ? ( d->m_vp.isValid() ? i18n( "Go to page %1", d->m_vp.pageNumber + 1 ) : "" ) :
                                     i18n("Open external file");
}

bool ActionGoto::isExternal() const
{
    return !d->m_extFileName.isEmpty();
}

QString ActionGoto::fileName() const
{
    return d->m_extFileName;
}

DocumentViewport ActionGoto::destViewport() const
{
    return d->m_vp;
}

// ActionExecute

class ActionExecute::Private
{
    public:
        Private( const QString &file, const QString & parameters )
            : m_fileName( file ), m_parameters( parameters )
        {
        }

        QString m_fileName;
        QString m_parameters;
};

ActionExecute::ActionExecute( const QString &file, const QString & parameters )
    : d( new Private( file, parameters ) )
{
}

ActionExecute::~ActionExecute()
{
    delete d;
}

Action::ActionType ActionExecute::actionType() const
{
    return Execute;
}

QString ActionExecute::actionTip() const
{
    return i18n( "Execute '%1'...", d->m_fileName );
}

QString ActionExecute::fileName() const
{
    return d->m_fileName;
}

QString ActionExecute::parameters() const
{
    return d->m_parameters;
}

// BrowseAction

class ActionBrowse::Private
{
    public:
        Private( const QString &url )
            : m_url( url )
        {
        }

        QString m_url;
};

ActionBrowse::ActionBrowse( const QString &url )
    : d( new Private( url ) )
{
}

ActionBrowse::~ActionBrowse()
{
    delete d;
}

Action::ActionType ActionBrowse::actionType() const
{
    return Browse;
}

QString ActionBrowse::actionTip() const
{
    return d->m_url;
}

QString ActionBrowse::url() const
{
    return d->m_url;
}

// ActionDocumentAction

class ActionDocumentAction::Private
{
    public:
        Private( enum DocumentActionType documentActionType )
            : m_type( documentActionType )
        {
        }

        DocumentActionType m_type;
};

ActionDocumentAction::ActionDocumentAction( enum DocumentActionType documentActionType )
    : d( new Private( documentActionType ) )
{
}

ActionDocumentAction::~ActionDocumentAction()
{
    delete d;
}

ActionDocumentAction::DocumentActionType ActionDocumentAction::documentActionType() const
{
    return d->m_type;
}

Action::ActionType ActionDocumentAction::actionType() const
{
    return DocumentAction;
}

QString ActionDocumentAction::actionTip() const
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

// ActionSound

class ActionSound::Private
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

ActionSound::ActionSound( double volume, bool sync, bool repeat, bool mix, Okular::Sound *sound )
    : d( new Private( volume, sync, repeat, mix, sound ) )
{
}

ActionSound::~ActionSound()
{
    delete d;
}

Action::ActionType ActionSound::actionType() const
{
    return Sound;
}

QString ActionSound::actionTip() const
{
    return i18n( "Play sound..." );
}

double ActionSound::volume() const
{
    return d->m_volume;
}

bool ActionSound::synchronous() const
{
    return d->m_sync;
}

bool ActionSound::repeat() const
{
    return d->m_repeat;
}

bool ActionSound::mix() const
{
    return d->m_mix;
}

Okular::Sound *ActionSound::sound() const
{
    return d->m_sound;
}

// ActionMovie
class ActionMovie::Private
{
};

ActionMovie::ActionMovie()
    : d( 0 )
{
}

ActionMovie::~ActionMovie()
{
    delete d;
}

Action::ActionType ActionMovie::actionType() const
{
    return Movie;
}

QString ActionMovie::actionTip() const
{
    return i18n( "Play movie..." );
}
