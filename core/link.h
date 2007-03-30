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
#include "core/document.h" // for DocumentViewport

/**
 * @short Encapsulates data that describes a link.
 *
 * This is the base class for links. It makes mandatory for inherited
 * widgets to reimplement the 'linkType' method and return the type of
 * the link described by the reimplemented class.
 */
class KPDFLink
{
    public:
        // get link type (inherited classes mustreturn an unique identifier)
        enum LinkType { Goto, Execute, Browse, Action, Movie };
        virtual LinkType linkType() const = 0;
        virtual QString linkTip() const { return QString::null; }

        // virtual destructor (remove warnings)
        virtual ~KPDFLink();
};


/** Goto: a viewport and maybe a reference to an external filename **/
class KPDFLinkGoto : public KPDFLink
{
    public:
        // query for goto parameters
        bool isExternal() const { return !m_extFileName.isEmpty(); }
        const QString & fileName() const { return m_extFileName; }
        const DocumentViewport & destViewport() const { return m_vp; }

        // create a KPDFLink_Goto
        KPDFLinkGoto( QString extFileName, const DocumentViewport & vp ) { m_extFileName = extFileName; m_vp = vp; }
        LinkType linkType() const { return Goto; }
        QString linkTip() const;

    private:
        QString m_extFileName;
        DocumentViewport m_vp;
};

/** Execute: filename and parameters to execute **/
class KPDFLinkExecute : public KPDFLink
{
    public:
        // query for filename / parameters
        const QString & fileName() const { return m_fileName; }
        const QString & parameters() const { return m_parameters; }

        // create a KPDFLink_Execute
        KPDFLinkExecute( const QString & file, const QString & params ) { m_fileName = file; m_parameters = params; }
        LinkType linkType() const { return Execute; }
        QString linkTip() const;

    private:
        QString m_fileName;
        QString m_parameters;
};

/** Browse: an URL to open, ranging from 'http://' to 'mailto:' etc.. **/
class KPDFLinkBrowse : public KPDFLink
{
    public:
        // query for URL
        const QString & url() const { return m_url; }

        // create a KPDFLink_Browse
        KPDFLinkBrowse( const QString &url ) { m_url = url; }
        LinkType linkType() const { return Browse; }
        QString linkTip() const;

    private:
        QString m_url;
};

/** Action: contains an action to perform on document / kpdf **/
class KPDFLinkAction : public KPDFLink
{
    public:
        // define types of actions
        enum ActionType { PageFirst, PagePrev, PageNext, PageLast, HistoryBack, HistoryForward, Quit, Presentation, EndPresentation, Find, GoToPage, Close };

        // query for action type
        ActionType actionType() const { return m_type; }

        // create a KPDFLink_Action
        KPDFLinkAction( enum ActionType actionType ) { m_type = actionType; }
        LinkType linkType() const { return Action; }
        QString linkTip() const;

    private:
        ActionType m_type;
};

/** Movie: Not yet defined -> think renaming to 'Media' link **/
class KPDFLinkMovie : public KPDFLink
// TODO this (Movie link)
{
    public:
        KPDFLinkMovie() {};
        LinkType linkType() const { return Movie; }
};

#endif
