/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_ACTION_H_
#define _OKULAR_ACTION_H_

#include <okular/core/okular_export.h>

#include <QtCore/QString>

namespace Okular {

class Sound;
class DocumentViewport;

/**
 * @short Encapsulates data that describes a link.
 *
 * This is the base class for links. It makes mandatory for inherited
 * widgets to reimplement the 'actionType' method and return the type of
 * the link described by the reimplemented class.
 */
class OKULAR_EXPORT Action
{
    public:
        /**
         * Describes the type of link.
         */
        enum ActionType {
            Goto,       ///< Goto a given page or external document
            Execute,    ///< Execute a command or external application
            Browse,     ///< Browse a given website
            DocumentAction,     ///< Start a custom action
            Sound,      ///< Play a sound
            Movie       ///< Play a movie
        };

        /**
         * Creates a new link.
         */
        Action();

        /**
         * Destroys the link.
         */
        virtual ~Action();

        /**
         * Returns the type of the link. Every inherited class must return
         * an unique identifier.
         *
         * @see ActionType
         */
        virtual ActionType actionType() const = 0;

        /**
         * Returns a i18n'ed tip of the link action that is presented to
         * the user.
         */
        virtual QString actionTip() const;

    private:
        class Private;
        Private* const d;

        Q_DISABLE_COPY( Action )
};


/**
 * The Goto link changes the viewport to another page
 * or loads an external document.
 */
class OKULAR_EXPORT ActionGoto : public Action
{
    public:
        /**
         * Creates a new goto link.
         *
         * @p fileName The name of an external file that shall be loaded.
         * @p viewport The target viewport information of the current document.
         */
        ActionGoto( const QString& fileName, const DocumentViewport & viewport );

        /**
         * Destroys the goto link.
         */
        virtual ~ActionGoto();

        /**
         * Returns the link type.
         */
        ActionType actionType() const;

        /**
         * Returns the link tip.
         */
        QString actionTip() const;

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
        class Private;
        Private* const d;

        Q_DISABLE_COPY( ActionGoto )
};

/**
 * The Execute link executes an external application.
 */
class OKULAR_EXPORT ActionExecute : public Action
{
    public:
        /**
         * Creates a new execute link.
         *
         * @param fileName The file name of the application to execute.
         * @param parameters The parameters of the application to execute.
         */
        ActionExecute( const QString &fileName, const QString &parameters );

        /**
         * Destroys the execute link.
         */
        virtual ~ActionExecute();

        /**
         * Returns the link type.
         */
        ActionType actionType() const;

        /**
         * Returns the link tip.
         */
        QString actionTip() const;

        /**
         * Returns the file name of the application to execute.
         */
        QString fileName() const;

        /**
         * Returns the parameters of the application to execute.
         */
        QString parameters() const;

    private:
        class Private;
        Private* const d;

        Q_DISABLE_COPY( ActionExecute )
};

/**
 * The Browse link browses an url by opening a web browser or
 * email client, depedning on the url protocol (e.g. http, mailto, etc.).
 */
class OKULAR_EXPORT ActionBrowse : public Action
{
    public:
        /**
         * Creates a new browse link.
         *
         * @param url The url to browse.
         */
        ActionBrowse( const QString &url );

        /**
         * Destroys the browse link.
         */
        virtual ~ActionBrowse();

        /**
         * Returns the link type.
         */
        ActionType actionType() const;

        /**
         * Returns the link tip.
         */
        QString actionTip() const;

        /**
         * Returns the url to browse.
         */
        QString url() const;

    private:
        class Private;
        Private* const d;

        Q_DISABLE_COPY( ActionBrowse )
};

/**
 * The Action link contains an action that is performed on
 * the current document.
 */
class OKULAR_EXPORT ActionDocumentAction : public Action
{
    public:
        /**
         * Describes the possible action types.
         *
         * WARNING KEEP IN SYNC WITH POPPLER!
         */
        enum DocumentActionType {
            PageFirst = 1,        ///< Jump to first page
            PagePrev = 2,         ///< Jump to previous page
            PageNext = 3,         ///< Jump to next page
            PageLast = 4,         ///< Jump to last page
            HistoryBack = 5,      ///< Go back in page history
            HistoryForward = 6,   ///< Go forward in page history
            Quit = 7,             ///< Quit application
            Presentation = 8,     ///< Start presentation
            EndPresentation = 9,  ///< End presentation
            Find = 10,            ///< Open find dialog
            GoToPage = 11,        ///< Goto page
            Close = 12            ///< Close document
        };

        /**
         * Creates a new link action.
         *
         * @param actionType The type of action.
         */
        explicit ActionDocumentAction( enum DocumentActionType documentActionType );

        /**
         * Destroys the action link.
         */
        virtual ~ActionDocumentAction();

        /**
         * Returns the link type.
         */
        ActionType actionType() const;

        /**
         * Returns the link tip.
         */
        QString actionTip() const;

        /**
         * Returns the type of action.
         */
        DocumentActionType documentActionType() const;

    private:
        class Private;
        Private* const d;

        Q_DISABLE_COPY( ActionDocumentAction )
};

/**
 * The Sound link plays a sound on activation.
 */
class OKULAR_EXPORT ActionSound : public Action
{
    public:
        /**
         * Creates a new sound link.
         *
         * @param volume The volume of the sound.
         * @param synchronous Whether the sound shall be played synchronous.
         * @param repeat Whether the sound shall be repeated.
         * @param mix Whether the sound shall be mixed.
         * @param sound The sound object which contains the sound data.
         */
        ActionSound( double volume, bool synchronous, bool repeat, bool mix, Okular::Sound *sound );

        /**
         * Destroys the sound link.
         */
        virtual ~ActionSound();

        /**
         * Returns the link type.
         */
        ActionType actionType() const;

        /**
         * Returns the link tip.
         */
        QString actionTip() const;

        /**
         * Returns the volume of the sound.
         */
        double volume() const;

        /**
         * Returns whether the sound shall be played synchronous.
         */
        bool synchronous() const;

        /**
         * Returns whether the sound shall be repeated.
         */
        bool repeat() const;

        /**
         * Returns whether the sound shall be mixed.
         */
        bool mix() const;

        /**
         * Returns the sound object which contains the sound data.
         */
        Okular::Sound *sound() const;

    private:
        class Private;
        Private* const d;

        Q_DISABLE_COPY( ActionSound )
};

/**
 * The Movie link plays a video on activation.
 */
class ActionMovie : public Action
{
    public:
        /**
         * Creates a new movie link.
         */
        ActionMovie();

        /**
         * Destroys the movie link.
         */
        virtual ~ActionMovie();

        /**
         * Returns the link type.
         */
        ActionType actionType() const;

        /**
         * Returns the link tip.
         */
        QString actionTip() const;

    private:
        class Private;
        Private* const d;

        Q_DISABLE_COPY( ActionMovie )
};

}

#endif
