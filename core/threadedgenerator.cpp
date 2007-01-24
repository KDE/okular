/***************************************************************************
 *   Copyright (C) 2007  Tobias Koenig <tokoe@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "core/page.h"
#include "core/textpage.h"

#include "threadedgenerator.h"
#include "threadedgenerator_p.h"

using namespace Okular;

class ThreadedGenerator::Private
{
    public:
        Private( ThreadedGenerator *parent )
            : mParent( parent ), mReady( true )
        {
            mPixmapGenerationThread = new PixmapGenerationThread( mParent );
            mParent->connect( mPixmapGenerationThread, SIGNAL( finished() ),
                              mParent, SLOT( pixmapGenerationFinished() ),
                              Qt::QueuedConnection );
        }

        ~Private()
        {
            if ( mPixmapGenerationThread )
                mPixmapGenerationThread->wait();

            delete mPixmapGenerationThread;
        }

        void pixmapGenerationFinished();
        void textpageGenerationFinished();

        ThreadedGenerator *mParent;
        PixmapGenerationThread *mPixmapGenerationThread;
        bool mReady;
};

void ThreadedGenerator::Private::pixmapGenerationFinished()
{
    PixmapRequest *request = mPixmapGenerationThread->request();
    mPixmapGenerationThread->endGeneration();

    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( mPixmapGenerationThread->image() ) ) );

    mReady = true;

    mParent->signalRequestDone( request );
}

void ThreadedGenerator::Private::textpageGenerationFinished()
{
}

ThreadedGenerator::ThreadedGenerator()
    : d( new Private( this ) )
{
}

ThreadedGenerator::~ThreadedGenerator()
{
    delete d;
}

bool ThreadedGenerator::canRequestPixmap() const
{
    return d->mReady;
}

void ThreadedGenerator::requestPixmap( PixmapRequest * request )
{
    d->mReady = false;

    d->mPixmapGenerationThread->startGeneration( request );
}

void ThreadedGenerator::requestTextPage( Page* )
{
}

TextPage* ThreadedGenerator::textPage( Page* )
{
    return 0;
}

bool ThreadedGenerator::canGeneratePixmap() const
{
    // dummy implementation
    return false;
}

void ThreadedGenerator::generatePixmap( PixmapRequest* )
{
    // dummy implementation
}

void ThreadedGenerator::generateSyncTextPage( Page* )
{
    // dummy implementation
}

#include "threadedgenerator.moc"
