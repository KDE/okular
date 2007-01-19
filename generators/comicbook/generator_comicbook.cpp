/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QThread>
#include <QtGui/QPainter>

#include <kprinter.h>

#include <okular/core/page.h>

#include "generator_comicbook.h"

OKULAR_EXPORT_PLUGIN(ComicBookGenerator)

class GeneratorThread : public QThread
{
    public:
        GeneratorThread();

        void startGeneration( Okular::PixmapRequest* request, ComicBook::Document *document );
        void endGeneration();

        Okular::PixmapRequest *request() const;
        QImage image();

    private:
        void run();

        Okular::PixmapRequest* mRequest;
        QImage mImage;
        ComicBook::Document* mDocument;
};

GeneratorThread::GeneratorThread()
  : QThread(), mRequest( 0 ), mDocument( 0 )
{
}

void GeneratorThread::startGeneration( Okular::PixmapRequest* request, ComicBook::Document *document )
{
    mRequest = request;
    mDocument = document;
    start( QThread::InheritPriority );
}

void GeneratorThread::endGeneration()
{
    mRequest = 0;
    mDocument = 0;
}

Okular::PixmapRequest* GeneratorThread::request() const
{
    return mRequest;
}

QImage GeneratorThread::image()
{
    const QImage image = mImage;
    mImage = QImage();

    return image;
}

void GeneratorThread::run()
{
    int width = mRequest->width();
    int height = mRequest->height();

    mImage = mDocument->pageImage( mRequest->pageNumber() );
    mImage = mImage.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
}

ComicBookGenerator::ComicBookGenerator()
    : Generator(), mReady( false )
{
    mThread = new GeneratorThread();
    connect( mThread, SIGNAL( finished() ), this, SLOT( threadFinished() ), Qt::QueuedConnection );
}

ComicBookGenerator::~ComicBookGenerator()
{
    if ( mThread )
        mThread->wait();

    delete mThread;
}

bool ComicBookGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    if ( !mDocument.open( fileName ) )
        return false;

    int pages = mDocument.pages();
    pagesVector.resize( pages );

    for ( int i = 0; i < pages; ++i ) {
        Okular::Page * page = new Okular::Page( i, 800, 600, Okular::Rotation0 );
        pagesVector[i] = page;
    }

    mReady = true;

    return true;
}

bool ComicBookGenerator::closeDocument()
{
    mReady = false;

    return true;
}

bool ComicBookGenerator::canGeneratePixmap( bool ) const
{
    return mReady;
}

void ComicBookGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    mReady = false;

    if ( request->asynchronous() ) {
        mThread->startGeneration( request, &mDocument );
        return;
    }

    int width = request->width();
    int height = request->height();

    QImage image = mDocument.pageImage( request->pageNumber() );
    image = image.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( image ) ) );

    mReady = true;

    // signal that the request has been accomplished
    signalRequestDone( request );
}

void ComicBookGenerator::threadFinished()
{
    Okular::PixmapRequest *request = mThread->request();
    mThread->endGeneration();

    request->page()->setPixmap( request->id(), new QPixmap( QPixmap::fromImage( mThread->image() ) ) );

    mReady = true;

    signalRequestDone( request );
}

bool ComicBookGenerator::print( KPrinter& printer )
{
    QPainter p( &printer );

    for ( int i = 0; i < mDocument.pages(); ++i ) {
        QImage image = mDocument.pageImage( i );
        uint left, top, right, bottom;
        printer.margins( &left, &top, &right, &bottom );

        int pageWidth = printer.width() - right;
        int pageHeight = printer.height() - bottom;

        if ( (image.width() > pageWidth) || (image.height() > pageHeight) )
            image = image.scaled( pageWidth, pageHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

        if ( i != 0 )
            printer.newPage();

        p.drawImage( 0, 0, image );
    }

    return true;
}

bool ComicBookGenerator::hasFeature( GeneratorFeature ) const
{
    return false;
}

#include "generator_comicbook.moc"

