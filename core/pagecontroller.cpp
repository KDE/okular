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

using namespace Okular;

PageController::PageController()
  : QObject()
{
}

PageController::~PageController()
{
}

void PageController::addRotationJob(RotationJob *job)
{
    connect( job, SIGNAL(done(ThreadWeaver::Job*)),
             this, SLOT(imageRotationDone(ThreadWeaver::Job*)) );
    ThreadWeaver::Weaver::instance()->enqueue(job);
}

void PageController::imageRotationDone(ThreadWeaver::Job *j)
{
    RotationJob *job = static_cast< RotationJob * >( j );

    if ( job->page() )
    {
        job->page()->imageRotationDone( job );

        emit rotationFinished( job->page()->m_number, job->page()->m_page );
    }

    job->deleteLater();
}

#include "pagecontroller_p.moc"
