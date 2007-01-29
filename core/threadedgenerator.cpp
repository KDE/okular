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
            : mParent( parent ),
              mPixmapReady( true ),
              mTextPageReady( true )
        {
            mPixmapGenerationThread = new PixmapGenerationThread( mParent );
            mParent->connect( mPixmapGenerationThread, SIGNAL( finished() ),
                              mParent, SLOT( pixmapGenerationFinished() ),
                              Qt::QueuedConnection );

            mTextPageGenerationThread = new TextPageGenerationThread( mParent );
            mParent->connect( mTextPageGenerationThread, SIGNAL( finished() ),
                              mParent, SLOT( textpageGenerationFinished() ),
                              Qt::QueuedConnection );
        }

        ~Private()
        {
            if ( mPixmapGenerationThread )
                mPixmapGenerationThread->wait();

            delete mPixmapGenerationThread;

            if ( mTextPageGenerationThread )
                mTextPageGenerationThread->wait();

            delete mTextPageGenerationThread;
        }

        void pixmapGenerationFinished();
        void textpageGenerationFinished();

        ThreadedGenerator *mParent;
        PixmapGenerationThread *mPixmapGenerationThread;
        TextPageGenerationThread *mTextPageGenerationThread;
        bool mPixmapReady;
        bool mTextPageReady;
};

void ThreadedGenerator::Private::pixmapGenerationFinished()
{
    PixmapRequest *request = mPixmapGenerationThread->request();
    mPixmapGenerationThread->endGeneration();

    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( mPixmapGenerationThread->image() ) ) );

    mPixmapReady = true;

    mParent->signalPixmapRequestDone( request );
}

void ThreadedGenerator::Private::textpageGenerationFinished()
{
    Page *page = mTextPageGenerationThread->page();
    mTextPageGenerationThread->endGeneration();

    mTextPageReady = true;

    if ( mTextPageGenerationThread->textPage() )
        page->setTextPage( mTextPageGenerationThread->textPage() );
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
    return d->mPixmapReady;
}

void ThreadedGenerator::requestPixmap( PixmapRequest * request )
{
    d->mPixmapReady = false;

    d->mPixmapGenerationThread->startGeneration( request );
}

bool ThreadedGenerator::canRequestTextPage() const
{
    return d->mTextPageReady;
}

void ThreadedGenerator::requestTextPage( Page * page )
{
    d->mTextPageReady = false;

    d->mTextPageGenerationThread->startGeneration( page );
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

bool ThreadedGenerator::canGenerateTextPage() const
{
    // dummy implementation
    return false;
}

void ThreadedGenerator::generateSyncTextPage( Page* )
{
    // dummy implementation
}

#include "threadedgenerator.moc"
