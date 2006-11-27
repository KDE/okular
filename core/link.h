/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_LINK_H_
#define _OKULAR_LINK_H_

#include "okular_export.h"

#include <QtCore/QRect>
#include <QtCore/QString>

#include "document.h" // for DocumentViewport

namespace Okular {

class Sound;

/**
 * @short Encapsulates data that describes a link.
 *
 * This is the base class for links. It makes mandatory for inherited
 * widgets to reimplement the 'linkType' method and return the type of
 * the link described by the reimplemented class.
 */
class OKULAR_EXPORT Link
{
    public:
        /**
         * Describes the type of link.
         */
        enum LinkType {
            Goto,       ///< Goto a given page or external document
            Execute,    ///< Execute a command or external application
            Browse,     ///< Browse a given website
            Action,     ///< Start a custom action
            Sound,      ///< Play a sound
            Movie       ///< Play a movie
        };

        /**
         * Destroys the link.
         */
        virtual ~Link();

        /**
         * Returns the type of the link. Every inherited class must return
         * an unique identifier.
         *
         * @see LinkType
         */
        virtual LinkType linkType() const = 0;

        /**
         * Returns a i18n'ed tip of the link action that is presented to
         * the user.
         */
        virtual QString linkTip() const;
};


/**
 * The Goto link changes the viewport to another page
 * or loads an external document.
 */
class OKULAR_EXPORT LinkGoto : public Link
{
    public:
        /**
         * Creates a new goto link.
         *
         * @p fileName The name of an external file that shall be loaded.
         * @p viewport The target viewport information of the current document.
         */
        LinkGoto( QString fileName, const DocumentViewport & viewport );

        /**
         * Returns the link type.
         */
        LinkType linkType() const;

        /**
         * Returns the link tip.
         */
        QString linkTip() const;

        /**
         * Returns whether the goto link points to an external document.
         */
        bool isExternal() const;

        /**
         * Returns the filename of the external document.
         */
        QString fileName() const;

        /**
         * Returns the document viewport the goto link points to.
         */
        DocumentViewport destViewport() const;

    private:
        QString m_extFileName;
        DocumentViewport m_vp;
};

/** Execute: filename and parameters to execute **/
class OKULAR_EXPORT LinkExecute : public Link
{
    public:
        // query for filename / parameters
        const QString & fileName() const { return m_fileName; }
        const QString & parameters() const { return m_parameters; }

        // create a Link_Execute
        LinkExecute( const QString & file, const QString & params ) { m_fileName = file; m_parameters = params; }
        LinkType linkType() const { return Execute; }
        QString linkTip() const;

    private:
        QString m_fileName;
        QString m_parameters;
};

/** Browse: an URL to open, ranging from 'http://' to 'mailto:' etc.. **/
class OKULAR_EXPORT LinkBrowse : public Link
{
    public:
        // query for URL
        const QString & url() const { return m_url; }

        // create a Link_Browse
        LinkBrowse( const QString &url ) { m_url = url; }
        LinkType linkType() const { return Browse; }
        QString linkTip() const;

    private:
        QString m_url;
};

/** Action: contains an action to perform on document / okular **/
class OKULAR_EXPORT LinkAction : public Link
{
    public:
        // define types of actions
        // WARNING KEEP IN SYNC WITH POPPLER
        enum ActionType { PageFirst = 1,
                  PagePrev = 2,
                  PageNext = 3,
                  PageLast = 4,
                  HistoryBack = 5,
                  HistoryForward = 6,
                  Quit = 7,
                  Presentation = 8,
                  EndPresentation = 9,
                  Find = 10,
                  GoToPage = 11,
                  Close = 12 };

        // query for action type
        ActionType actionType() const { return m_type; }

        // create a Link_Action
        LinkAction( enum ActionType actionType ) { m_type = actionType; }
        LinkType linkType() const { return Action; }
        QString linkTip() const;

    private:
        ActionType m_type;
};

/** Sound: a sound to be played **/
class OKULAR_EXPORT LinkSound : public Link
{
    public:
        // create a Link_Sound
        LinkSound( double volume, bool sync, bool repeat, bool mix, Okular::Sound *sound ) { m_volume = volume; m_sync = sync; m_repeat = repeat; m_mix = mix; m_sound = sound; }

        LinkType linkType() const { return Sound; };

        double volume() const { return m_volume; }
        bool synchronous() const { return m_sync; }
        bool repeat() const { return m_repeat; }
        bool mix() const { return m_mix; }
        Okular::Sound *sound() const { return m_sound; }

    private:
        double m_volume;
        bool m_sync;
        bool m_repeat;
        bool m_mix;
        Okular::Sound *m_sound;
};

/** Movie: Not yet defined -> think renaming to 'Media' link **/
class LinkMovie : public Link
// TODO this (Movie link)
{
    public:
        LinkMovie() {};
        LinkType linkType() const { return Movie; }
};

}

#endif
