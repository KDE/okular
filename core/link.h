/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _KPDF_LINK_H_
#define _KPDF_LINK_H_

#include <qstring.h>
#include <qrect.h>

/**
 * @short Encapsulates data that describes a link.
 * TODO: comment
 */
class KPDFLink
{
    public:
        // get link type (inherited classes mustreturn an unique identifier)
        enum LinkType { Goto, Execute, Browse, Action, Movie };
        virtual LinkType linkType() const = 0;

        // virtual destructor (remove warnings)
        virtual ~KPDFLink();
};


class KPDFLinkGoto : public KPDFLink
{
    public:
        // define a 'Viewport' TODO MERGE WITH DOCUMENT VIEWPORT
        struct Viewport {
            int page;
            bool fitWidth, fitHeight;
            QRect rectPercent;
        };

        // query for goto parameters
        bool isExternal() const { return !m_extFileName.isEmpty(); }
        const QString & fileName() const { return m_extFileName; }
        const Viewport & destViewport() const { return m_vp; }

        // create a KPDFLink_Goto
        KPDFLinkGoto( QString extFileName, Viewport vp ) { m_extFileName = extFileName; m_vp = vp; }
        LinkType linkType() const { return Goto; }

    private:
        QString m_extFileName;
        Viewport m_vp;
};


class KPDFLinkExecute : public KPDFLink
{
    public:
        // query for filename / parameters
        const QString & fileName() const { return m_fileName; }
        const QString & parameters() const { return m_parameters; }

        // create a KPDFLink_Execute
        KPDFLinkExecute( const QString & file, const QString & params ) { m_fileName = file; m_parameters = params; }
        LinkType linkType() const { return Execute; }

    private:
        QString m_fileName;
        QString m_parameters;
};


class KPDFLinkBrowse : public KPDFLink
{
    public:
        // query for URL
        const QString & url() const { return m_url; }

        // create a KPDFLink_Browse
        KPDFLinkBrowse( const QString &url ) { m_url = url; }
        LinkType linkType() const { return Browse; }

    private:
        QString m_url;
};


class KPDFLinkAction : public KPDFLink
{
    public:
        // define types of actions
        enum ActionType { PageFirst, PagePrev, PageNext, PageLast, HistoryBack, HistoryForward, Quit, Find, GoToPage };

        // query for action type
        ActionType actionType() const { return m_type; }

        // create a KPDFLink_Action
        KPDFLinkAction( enum ActionType actionType ) { m_type = actionType; }
        LinkType linkType() const { return Action; }

    private:
        ActionType m_type;
};


class KPDFLinkMovie : public KPDFLink
//TODO: this
{
    public:
        KPDFLinkMovie() {};
        LinkType linkType() const { return Movie; }
};

#endif
