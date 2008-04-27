/***************************************************************************
 *   Copyright (C) 2008 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "view.h"
#include "view_p.h"

// local includes
#include "document_p.h"

using namespace Okular;

ViewPrivate::ViewPrivate()
    : document( 0 )
{
}

ViewPrivate::~ViewPrivate()
{
}


View::View( const QString &name )
    : d_ptr( new ViewPrivate() )
{
    d_ptr->name = name;
}

View::~View()
{
    if ( d_ptr->document )
    {
        d_ptr->document->m_views.remove( this );
    }

    delete d_ptr;
}

Document* View::viewDocument() const
{
    return d_ptr->document ? d_ptr->document->m_parent : 0;
}

QString View::name() const
{
    return d_ptr->name;
}

bool View::supportsCapability( View::ViewCapability capability ) const
{
    Q_UNUSED( capability )
    return false;
}

View::CapabilityFlags View::capabilityFlags( View::ViewCapability capability ) const
{
    Q_UNUSED( capability )
    return 0;
}

QVariant View::capability( View::ViewCapability capability ) const
{
    Q_UNUSED( capability )
    return QVariant();
}

void View::setCapability( View::ViewCapability capability, const QVariant &option )
{
    Q_UNUSED( capability )
    Q_UNUSED( option )
}

