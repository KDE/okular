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
        // get link type (inherited classes mustreturn an unique identifier)
        enum LinkType { Goto, Execute, Browse, Action, Movie };
        virtual LinkType linkType() const = 0;
        virtual QString linkTip() const;

        // virtual destructor (remove warnings)
        virtual ~Link();
};


/** Goto: a viewport and maybe a reference to an external filename **/
class OKULAR_EXPORT LinkGoto : public Link
{
    public:
        // query for goto parameters
        bool isExternal() const { return !m_extFileName.isEmpty(); }
        const QString & fileName() const { return m_extFileName; }
        const DocumentViewport & destViewport() const { return m_vp; }

        // create a Link_Goto
        LinkGoto( QString extFileName, const DocumentViewport & vp ) { m_extFileName = extFileName; m_vp = vp; }
        LinkType linkType() const { return Goto; }
        QString linkTip() const;

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
