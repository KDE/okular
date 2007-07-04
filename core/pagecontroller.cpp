/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pagecontroller_p.h"

// qt/kde includes
#include <kglobal.h>
#include <threadweaver/ThreadWeaver.h>

// local includes
#include "page_p.h"
#include "rotationjob_p.h"

K_GLOBAL_STATIC( Okular::PageController, page_controller_self )

using namespace Okular;

PageController::PageController()
  : QObject()
{
    connect( ThreadWeaver::Weaver::instance(),
             SIGNAL( jobDone(ThreadWeaver::Job*) ),
             SLOT( imageRotationDone(ThreadWeaver::Job*) ) );
}

PageController::~PageController()
{
}

PageController * PageController::self()
{
    return page_controller_self;
}

void PageController::addRotationJob(RotationJob *job)
{
    ThreadWeaver::Weaver::instance()->enqueue(job);
}

void PageController::imageRotationDone(ThreadWeaver::Job *j)
{
    RotationJob *job = qobject_cast< RotationJob * >(j);

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
