/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qdatetime.h>
#include <qfile.h>
#include <qimage.h>
#include <qlist.h>
#include <qpixmap.h>
#include <qthread.h>
#include <kglobal.h>
#include <kimageeffect.h>
#include <klocale.h>

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

        void startGeneration( Okular::PixmapRequest* request, TIFF* tiff );
        void endGeneration();

        Okular::PixmapRequest *request() const;
        QPixmap * takePixmap();

    private:
        void run();

        Okular::PixmapRequest* m_request;
        QPixmap* m_pix;
        TIFF* m_tiff;
};

TIFFGeneratorThread::TIFFGeneratorThread()
  : QThread(), m_request( 0 ), m_pix( 0 ), m_tiff( 0 )
{
}

void TIFFGeneratorThread::startGeneration( Okular::PixmapRequest* request, TIFF* tiff )
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

Okular::PixmapRequest* TIFFGeneratorThread::request() const
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

static QDateTime convertTIFFDateTime( const char* tiffdate )
{
    if ( !tiffdate )
        return QDateTime();

    return QDateTime::fromString( QString::fromLatin1( tiffdate ), "yyyy:MM:dd HH:mm:ss" );
}

OKULAR_EXPORT_PLUGIN(TIFFGenerator)

TIFFGenerator::TIFFGenerator( Okular::Document * document ) : Okular::Generator( document ),
  d( new Private ), ready( false ), m_docInfo( 0 )
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
    delete m_docInfo;

    delete d;
}

bool TIFFGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    d->tiff = TIFFOpen( QFile::encodeName( fileName ), "r" );
    if ( !d->tiff )
        return false;

    loadPages( pagesVector, 0 );

    ready = true;

    return true;
}

bool TIFFGenerator::closeDocument()
{
    // closing the old document
    if ( d->tiff )
    {
        TIFFClose( d->tiff );
        d->tiff = 0;
        delete m_docInfo;
        m_docInfo = 0;
    }

    ready = false;

    return true;
}

bool TIFFGenerator::canGeneratePixmap( bool /*async*/ )
{
    return ready;
}

void TIFFGenerator::generatePixmap( Okular::PixmapRequest * request )
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

const Okular::DocumentInfo * TIFFGenerator::generateDocumentInfo()
{
    if ( !d->tiff )
        return 0;

    if ( m_docInfo )
        return m_docInfo;

    m_docInfo = new Okular::DocumentInfo();

    m_docInfo->set( "mimeType", "image/tiff" );

    char* buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_IMAGEDESCRIPTION, &buffer );
    m_docInfo->set( "description", buffer ? QString::fromLatin1( buffer ) : i18n( "Unknown" ), i18n( "Description" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_SOFTWARE, &buffer );
    m_docInfo->set( "software", buffer ? QString::fromLatin1( buffer ) : i18n( "Unknown" ), i18n( "Software" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_COPYRIGHT, &buffer );
    m_docInfo->set( "copyright", buffer ? QString::fromLatin1( buffer ) : i18n( "Unknown" ), i18n( "Copyright" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_ARTIST, &buffer );
    m_docInfo->set( "artist", buffer ? QString::fromLatin1( buffer ) : i18n( "Unknown" ), i18n( "Artist" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_DATETIME, &buffer );
    QDateTime date = convertTIFFDateTime( buffer );
    m_docInfo->set( "dateTime", date.isValid() ? KGlobal::locale()->formatDateTime( date, false, true  ) : i18n( "Unknown" ), i18n( "Creation date" ) );

    return m_docInfo;
}

void TIFFGenerator::slotThreadFinished()
{
    Okular::PixmapRequest * request = thread->request();
    thread->endGeneration();

    request->page->setPixmap( request->id, thread->takePixmap() );

    ready = true;

    signalRequestDone( request );
}

void TIFFGenerator::loadPages( QVector<Okular::Page*> & pagesVector, int rotation )
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
        Okular::Page * page = new Okular::Page( i, width, height, rotation );
        pagesVector[i] = page;

    }
}

#include "generator_tiff.moc"

