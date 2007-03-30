/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// local includes
#include "link.h"

#include <klocale.h>

KPDFLink::~KPDFLink()
{
}

QString KPDFLinkGoto::linkTip() const
{
    return m_extFileName.isEmpty() ? ( m_vp.pageNumber != -1 ? i18n( "Go to page %1" ).arg( m_vp.pageNumber + 1 ) : QString::null ) : i18n("Open external file");
}

QString KPDFLinkExecute::linkTip() const
{
    return i18n( "Execute '%1'..." ).arg( m_fileName );
}

QString KPDFLinkBrowse::linkTip() const
{
    return m_url;
}

QString KPDFLinkAction::linkTip() const
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

    return QString::null;
}
