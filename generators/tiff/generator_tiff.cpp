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
#include <qpainter.h>
#include <kglobal.h>
#include <klocale.h>
#include <kprinter.h>

#include <okular/core/document.h>
#include <okular/core/page.h>

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

static QDateTime convertTIFFDateTime( const char* tiffdate )
{
    if ( !tiffdate )
        return QDateTime();

    return QDateTime::fromString( QString::fromLatin1( tiffdate ), "yyyy:MM:dd HH:mm:ss" );
}

OKULAR_EXPORT_PLUGIN(TIFFGenerator)

TIFFGenerator::TIFFGenerator()
    : Okular::Generator(),
      d( new Private ), m_docInfo( 0 )
{
    setFeature( Threaded );
}

TIFFGenerator::~TIFFGenerator()
{
    if ( d->tiff )
    {
        TIFFClose( d->tiff );
        d->tiff = 0;
    }

    delete m_docInfo;
    delete d;
}

bool TIFFGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    d->tiff = TIFFOpen( QFile::encodeName( fileName ), "r" );
    if ( !d->tiff )
        return false;

    loadPages( pagesVector );

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

    return true;
}

QImage TIFFGenerator::image( Okular::PixmapRequest * request )
{
    bool generated = false;
    QImage img;

    if ( TIFFSetDirectory( d->tiff, request->page()->number() ) )
    {
        int rotation = request->page()->rotation();
        uint32 width = (uint32)request->page()->width();
        uint32 height = (uint32)request->page()->height();
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

            int reqwidth = request->width();
            int reqheight = request->height();
            if ( rotation % 2 == 1 )
                qSwap( reqwidth, reqheight );
            img = image.scaled( reqwidth, reqheight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

            generated = true;
        }
    }

    if ( !generated )
    {
        img = QImage( request->width(), request->height(), QImage::Format_RGB32 );
        img.fill( qRgb( 255, 255, 255 ) );
    }

    return img;
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
    m_docInfo->set( "dateTime", date.isValid() ? KGlobal::locale()->formatDateTime( date, KLocale::LongDate, true  ) : i18n( "Unknown" ), i18n( "Creation date" ) );

    return m_docInfo;
}

void TIFFGenerator::loadPages( QVector<Okular::Page*> & pagesVector )
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

        delete pagesVector[i];
        Okular::Page * page = new Okular::Page( i, width, height, Okular::Rotation0 );
        pagesVector[i] = page;

    }
}

bool TIFFGenerator::print( KPrinter& printer )
{
    uint32 width = 0;
    uint32 height = 0;

    tdir_t dirs = TIFFNumberOfDirectories( d->tiff );

    QPainter p( &printer );

    for ( tdir_t i = 0; i < dirs; ++i )
    {
        if ( !TIFFSetDirectory( d->tiff, i ) )
            continue;

        if ( TIFFGetField( d->tiff, TIFFTAG_IMAGEWIDTH, &width ) != 1 ||
             TIFFGetField( d->tiff, TIFFTAG_IMAGELENGTH, &height ) != 1 )
            continue;

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
        }

        if ( i != 0 )
            printer.newPage();

        p.drawImage( 0, 0, image );
    }

    return true;
}

#include "generator_tiff.moc"

