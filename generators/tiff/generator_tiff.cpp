/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qfile.h>
#include <qimage.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qthread.h>
#include <kimageeffect.h>

#include "core/page.h"
#include "generator_tiff.h"

#include <tiff.h>
#include <tiffio.h>

class TIFFGenerator::Private
{
    public:
        Private()
          : tiff( 0 ) {}

        TIFF* tiff;
};


class TIFFGeneratorThread : public QThread
{
    public:
        TIFFGeneratorThread();

        void startGeneration( PixmapRequest* request, TIFF* tiff );
        void endGeneration();

        PixmapRequest *request() const;
        QPixmap * takePixmap();

    private:
        void run();

        PixmapRequest* m_request;
        QPixmap* m_pix;
        TIFF* m_tiff;
};

TIFFGeneratorThread::TIFFGeneratorThread()
  : QThread(), m_request( 0 ), m_pix( 0 ), m_tiff( 0 )
{
}

void TIFFGeneratorThread::startGeneration( PixmapRequest* request, TIFF* tiff )
{
    m_request = request;
    m_tiff = tiff;
    start( QThread::InheritPriority );
}

void TIFFGeneratorThread::endGeneration()
{
    m_request = 0;
    m_tiff = 0;
}

PixmapRequest* TIFFGeneratorThread::request() const
{
    return m_request;
}

QPixmap* TIFFGeneratorThread::takePixmap()
{
    QPixmap* p = m_pix;
    m_pix = 0;
    return p;
}

void TIFFGeneratorThread::run()
{
    bool generated = false;
    m_pix = new QPixmap( m_request->width, m_request->height );

    if ( TIFFSetDirectory( m_tiff, m_request->page->number() ) )
    {
        int rotation = m_request->documentRotation;
        uint32 width = (uint32)m_request->page->width();
        uint32 height = (uint32)m_request->page->height();
        if ( rotation % 2 == 1 )
            qSwap( width, height );

        QImage image( width, height, QImage::Format_RGB32 );
        uint32 * data = (uint32 *)image.bits();

        // read data
        if ( TIFFReadRGBAImageOriented( m_tiff, width, height, data, ORIENTATION_TOPLEFT ) != 0 )
        {
            // an image read by ReadRGBAImage is ABGR, we need ARGB, so swap red and blue
            uint32 size = width * height;
            for ( uint32 i = 0; i < size; ++i )
            {
                uint32 red = ( data[i] & 0x00FF0000 ) >> 16;
                uint32 blue = ( data[i] & 0x000000FF ) << 16;
                data[i] = ( data[i] & 0xFF00FF00 ) + red + blue;
            }

            int reqwidth = m_request->width;
            int reqheight = m_request->height;
            if ( rotation % 2 == 1 )
                qSwap( reqwidth, reqheight );
            QImage smoothImage = image.scaled( reqwidth, reqheight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
            QImage finalImage = rotation > 0
                ? KImageEffect::rotate( smoothImage, (KImageEffect::RotateDirection)( rotation - 1 ) )
                : smoothImage;
            *m_pix = QPixmap::fromImage( finalImage );

            generated = true;
        }
    }

    if ( !generated )
    {
        m_pix->fill();
    }
}


OKULAR_EXPORT_PLUGIN(TIFFGenerator)

TIFFGenerator::TIFFGenerator( KPDFDocument * document ) : Generator( document ),
  d( new Private ), ready( false )
{
    thread = new TIFFGeneratorThread();
    connect( thread, SIGNAL( finished() ), this, SLOT( slotThreadFinished() ), Qt::QueuedConnection );
}

TIFFGenerator::~TIFFGenerator()
{
    if ( d->tiff )
    {
        TIFFClose( d->tiff );
        d->tiff = 0;
    }
    if ( thread )
    {
        thread->wait();
    }
    delete thread;

    delete d;
}

bool TIFFGenerator::loadDocument( const QString & fileName, QVector<KPDFPage*> & pagesVector )
{
    // closing the old document
    if ( d->tiff )
    {
        TIFFClose( d->tiff );
        d->tiff = 0;
    }

    d->tiff = TIFFOpen( QFile::encodeName( fileName ), "r" );
    if ( !d->tiff )
        return false;

    loadPages( pagesVector, 0 );

    ready = true;

    return true;
}

bool TIFFGenerator::canGeneratePixmap( bool /*async*/ )
{
    return ready;
}

void TIFFGenerator::generatePixmap( PixmapRequest * request )
{
    ready = false;

    if ( request->async )
    {
        thread->startGeneration( request, d->tiff );
        return;
    }

    bool generated = false;
    QPixmap * p = new QPixmap( request->width, request->height );

    if ( TIFFSetDirectory( d->tiff, request->page->number() ) )
    {
        int rotation = request->documentRotation;
        uint32 width = (uint32)request->page->width();
        uint32 height = (uint32)request->page->height();
        if ( rotation % 2 == 1 )
            qSwap( width, height );

        QImage image( width, height, QImage::Format_RGB32 );
        uint32 * data = (uint32 *)image.bits();

        // read data
        if ( TIFFReadRGBAImageOriented( d->tiff, width, height, data, ORIENTATION_TOPLEFT ) != 0 )
        {
            // an image read by ReadRGBAImage is ABGR, we need ARGB, so swap red and blue
            uint32 size = width * height;
            for ( uint32 i = 0; i < size; ++i )
            {
                uint32 red = ( data[i] & 0x00FF0000 ) >> 16;
                uint32 blue = ( data[i] & 0x000000FF ) << 16;
                data[i] = ( data[i] & 0xFF00FF00 ) + red + blue;
            }

            int reqwidth = request->width;
            int reqheight = request->height;
            if ( rotation % 2 == 1 )
                qSwap( reqwidth, reqheight );
            QImage smoothImage = image.scaled( reqwidth, reqheight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
            QImage finalImage = rotation > 0
                ? KImageEffect::rotate( smoothImage, (KImageEffect::RotateDirection)( rotation - 1 ) )
                : smoothImage;
            *p = QPixmap::fromImage( finalImage );

            generated = true;
        }
    }

    if ( !generated )
    {
        p->fill();
    }

    request->page->setPixmap( request->id, p );

    ready = true;

    // signal that the request has been accomplished
    signalRequestDone( request );
}

void TIFFGenerator::setOrientation( QVector<KPDFPage*> & pagesVector, int orientation )
{
    loadPages( pagesVector, orientation );
}

void TIFFGenerator::slotThreadFinished()
{
    PixmapRequest * request = thread->request();
    thread->endGeneration();

    request->page->setPixmap( request->id, thread->takePixmap() );

    ready = true;

    signalRequestDone( request );
}

void TIFFGenerator::loadPages( QVector<KPDFPage*> & pagesVector, int rotation )
{
    if ( !d->tiff )
        return;

    tdir_t dirs = TIFFNumberOfDirectories( d->tiff );
    pagesVector.resize( dirs );

    uint32 width = 0;
    uint32 height = 0;

    for ( tdir_t i = 0; i < dirs; ++i )
    {
        if ( !TIFFSetDirectory( d->tiff, i ) )
            continue;

        if ( TIFFGetField( d->tiff, TIFFTAG_IMAGEWIDTH, &width ) != 1 ||
             TIFFGetField( d->tiff, TIFFTAG_IMAGELENGTH, &height ) != 1 )
            continue;

        if ( rotation % 2 == 1 )
            qSwap( width, height );

        delete pagesVector[i];
        KPDFPage * page = new KPDFPage( i, width, height, rotation );
        pagesVector[i] = page;

    }
}

#include "generator_tiff.moc"

