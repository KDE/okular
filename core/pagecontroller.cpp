/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// qt/kde includes
#include <kglobal.h>

// local includes
#include "page_p.h"
#include "pagecontroller_p.h"
#include "rotationjob.h"

K_GLOBAL_STATIC( Okular::PageController, page_controller_self )

using namespace Okular;

PageController::PageController()
  : QObject()
{
}

PageController::~PageController()
{
}

PageController * PageController::self()
{
    return page_controller_self;
}

void PageController::imageRotationDone()
{
    RotationJob *job = sender() ? qobject_cast< RotationJob * >( sender() ) : 0;

    if ( !job )
        return;

    if ( job->page() )
    {
        job->page()->imageRotationDone( job );

        emit rotationFinished( job->page()->m_number );
    }

    job->deleteLater();
}

#include "pagecontroller_p.moc"
