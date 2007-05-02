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

class Okular::ActionPrivate
{
    public:
        ActionPrivate()
        {
        }

        virtual ~ActionPrivate()
        {
        }
};

Action::Action( ActionPrivate &dd )
    : d_ptr( &dd )
{
}

Action::~Action()
{
    delete d_ptr;
}

QString Action::actionTip() const
{
    return "";
}

// ActionGoto

class Okular::ActionGotoPrivate : public Okular::ActionPrivate
{
    public:
        ActionGotoPrivate( const QString &fileName, const DocumentViewport &viewport )
            : ActionPrivate(), m_extFileName( fileName ), m_vp( viewport )
        {
        }

        QString m_extFileName;
        DocumentViewport m_vp;
};

ActionGoto::ActionGoto( const QString& fileName, const DocumentViewport & viewport )
    : Action( *new ActionGotoPrivate( fileName, viewport ) )
{
}

ActionGoto::~ActionGoto()
{
}

Action::ActionType ActionGoto::actionType() const
{
    return Goto;
}

QString ActionGoto::actionTip() const
{
    Q_D( const ActionGoto );
    return d->m_extFileName.isEmpty() ? ( d->m_vp.isValid() ? i18n( "Go to page %1", d->m_vp.pageNumber + 1 ) : "" ) :
                                     i18n("Open external file");
}

bool ActionGoto::isExternal() const
{
    Q_D( const ActionGoto );
    return !d->m_extFileName.isEmpty();
}

QString ActionGoto::fileName() const
{
    Q_D( const ActionGoto );
    return d->m_extFileName;
}

DocumentViewport ActionGoto::destViewport() const
{
    Q_D( const ActionGoto );
    return d->m_vp;
}

// ActionExecute

class Okular::ActionExecutePrivate : public Okular::ActionPrivate
{
    public:
        ActionExecutePrivate( const QString &file, const QString & parameters )
            : ActionPrivate(), m_fileName( file ), m_parameters( parameters )
        {
        }

        QString m_fileName;
        QString m_parameters;
};

ActionExecute::ActionExecute( const QString &file, const QString & parameters )
    : Action( *new ActionExecutePrivate( file, parameters ) )
{
}

ActionExecute::~ActionExecute()
{
}

Action::ActionType ActionExecute::actionType() const
{
    return Execute;
}

QString ActionExecute::actionTip() const
{
    Q_D( const Okular::ActionExecute );
    return i18n( "Execute '%1'...", d->m_fileName );
}

QString ActionExecute::fileName() const
{
    Q_D( const Okular::ActionExecute );
    return d->m_fileName;
}

QString ActionExecute::parameters() const
{
    Q_D( const Okular::ActionExecute );
    return d->m_parameters;
}

// BrowseAction

class Okular::ActionBrowsePrivate : public Okular::ActionPrivate
{
    public:
        ActionBrowsePrivate( const QString &url )
            : ActionPrivate(), m_url( url )
        {
        }

        QString m_url;
};

ActionBrowse::ActionBrowse( const QString &url )
    : Action( *new ActionBrowsePrivate( url ) )
{
}

ActionBrowse::~ActionBrowse()
{
}

Action::ActionType ActionBrowse::actionType() const
{
    return Browse;
}

QString ActionBrowse::actionTip() const
{
    Q_D( const Okular::ActionBrowse );
    return d->m_url;
}

QString ActionBrowse::url() const
{
    Q_D( const Okular::ActionBrowse );
    return d->m_url;
}

// ActionDocumentAction

class Okular::ActionDocumentActionPrivate : public Okular::ActionPrivate
{
    public:
        ActionDocumentActionPrivate( enum ActionDocumentAction::DocumentActionType documentActionType )
            : ActionPrivate(), m_type( documentActionType )
        {
        }

        ActionDocumentAction::DocumentActionType m_type;
};

ActionDocumentAction::ActionDocumentAction( enum DocumentActionType documentActionType )
    : Action( *new ActionDocumentActionPrivate( documentActionType ) )
{
}

ActionDocumentAction::~ActionDocumentAction()
{
}

ActionDocumentAction::DocumentActionType ActionDocumentAction::documentActionType() const
{
    Q_D( const Okular::ActionDocumentAction );
    return d->m_type;
}

Action::ActionType ActionDocumentAction::actionType() const
{
    return DocumentAction;
}

QString ActionDocumentAction::actionTip() const
{
    Q_D( const Okular::ActionDocumentAction );
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

class Okular::ActionSoundPrivate : public Okular::ActionPrivate
{
    public:
        ActionSoundPrivate( double volume, bool sync, bool repeat, bool mix, Okular::Sound *sound )
            : ActionPrivate(), m_volume( volume ), m_sync( sync ),
              m_repeat( repeat ), m_mix( mix ), m_sound( sound )
        {
        }

        ~ActionSoundPrivate()
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
    : Action( *new ActionSoundPrivate( volume, sync, repeat, mix, sound ) )
{
}

ActionSound::~ActionSound()
{
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
    Q_D( const Okular::ActionSound );
    return d->m_volume;
}

bool ActionSound::synchronous() const
{
    Q_D( const Okular::ActionSound );
    return d->m_sync;
}

bool ActionSound::repeat() const
{
    Q_D( const Okular::ActionSound );
    return d->m_repeat;
}

bool ActionSound::mix() const
{
    Q_D( const Okular::ActionSound );
    return d->m_mix;
}

Okular::Sound *ActionSound::sound() const
{
    Q_D( const Okular::ActionSound );
    return d->m_sound;
}

// ActionMovie

class Okular::ActionMoviePrivate : public Okular::ActionPrivate
{
    public:
        ActionMoviePrivate()
            : ActionPrivate()
        {
        }
};

ActionMovie::ActionMovie()
    : Action( *new ActionMoviePrivate() )
{
}

ActionMovie::~ActionMovie()
{
}

Action::ActionType ActionMovie::actionType() const
{
    return Movie;
}

QString ActionMovie::actionTip() const
{
    return i18n( "Play movie..." );
}
