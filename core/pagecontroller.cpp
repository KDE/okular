/***************************************************************************
 *   Copyright (C) 2007 by Pino Toscano <pino@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "pagecontroller_p.h"

// local includes
#include "page_p.h"
#include "rotationjob_p.h"

#include <threadweaver/queueing.h>

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
    connect(job, &RotationJob::done, this, &PageController::imageRotationDone);
    ThreadWeaver::enqueue(&m_weaver, job);
}

void PageController::imageRotationDone(const ThreadWeaver::JobPointer &j)
{
    RotationJob *job = static_cast<RotationJob *>(j.data());

    if (job->page()) {
        job->page()->imageRotationDone(job);

        emit rotationFinished(job->page()->m_number, job->page()->m_page);
    }
}
