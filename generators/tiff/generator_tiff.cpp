/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_tiff.h"

#include <qdatetime.h>
#include <qfile.h>
#include <qimage.h>
#include <qlist.h>
#include <qpainter.h>
#include <QtGui/QPrinter>

#include <kaboutdata.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include <okular/core/document.h>
#include <okular/core/page.h>
#include <okular/core/fileprinter.h>

#include <tiff.h>
#include <tiffio.h>

#define TiffDebug 4714

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

static KAboutData createAboutData()
{
    KAboutData aboutData(
         "okular_tiff",
         "okular_tiff",
         ki18n( "TIFF Backend" ),
         "0.1.1",
         ki18n( "A TIFF backend" ),
         KAboutData::License_GPL,
         ki18n( "© 2006-2007 Pino Toscano" )
    );
    aboutData.addAuthor( ki18n( "Pino Toscano" ), KLocalizedString(), "pino@kde.org" );
    return aboutData;
}

OKULAR_EXPORT_PLUGIN( TIFFGenerator, createAboutData() )

TIFFGenerator::TIFFGenerator( QObject *parent, const QVariantList &args )
    : Okular::Generator( parent, args ),
      d( new Private ), m_docInfo( 0 )
{
    setFeature( Threaded );
    setFeature( PrintNative );
    setFeature( PrintToFile );
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

bool TIFFGenerator::doCloseDocument()
{
    // closing the old document
    if ( d->tiff )
    {
        TIFFClose( d->tiff );
        d->tiff = 0;
        delete m_docInfo;
        m_docInfo = 0;
        m_pageMapping.clear();
    }

    return true;
}

QImage TIFFGenerator::image( Okular::PixmapRequest * request )
{
    bool generated = false;
    QImage img;

    if ( TIFFSetDirectory( d->tiff, mapPage( request->page()->number() ) ) )
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

    m_docInfo->set( Okular::DocumentInfo::MimeType, "image/tiff" );

    char* buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_IMAGEDESCRIPTION, &buffer );
    m_docInfo->set( Okular::DocumentInfo::Description, buffer ? QString::fromLatin1( buffer ) : i18n( "Unknown" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_SOFTWARE, &buffer );
    m_docInfo->set( Okular::DocumentInfo::Producer, buffer ? QString::fromLatin1( buffer ) : i18n( "Unknown" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_COPYRIGHT, &buffer );
    m_docInfo->set( Okular::DocumentInfo::Copyright, buffer ? QString::fromLatin1( buffer ) : i18n( "Unknown" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_ARTIST, &buffer );
    m_docInfo->set( Okular::DocumentInfo::Author, buffer ? QString::fromLatin1( buffer ) : i18n( "Unknown" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_DATETIME, &buffer );
    QDateTime date = convertTIFFDateTime( buffer );
    m_docInfo->set( Okular::DocumentInfo::CreationDate, date.isValid() ? KGlobal::locale()->formatDateTime( date, KLocale::LongDate, true  ) : i18n( "Unknown" ) );

    return m_docInfo;
}

void TIFFGenerator::loadPages( QVector<Okular::Page*> & pagesVector )
{
    if ( !d->tiff )
        return;

    tdir_t dirs = TIFFNumberOfDirectories( d->tiff );
    pagesVector.resize( dirs );
    tdir_t realdirs = 0;

    uint32 width = 0;
    uint32 height = 0;

    for ( tdir_t i = 0; i < dirs; ++i )
    {
        if ( !TIFFSetDirectory( d->tiff, i ) )
            continue;

        if ( TIFFGetField( d->tiff, TIFFTAG_IMAGEWIDTH, &width ) != 1 ||
             TIFFGetField( d->tiff, TIFFTAG_IMAGELENGTH, &height ) != 1 )
            continue;

        Okular::Page * page = new Okular::Page( realdirs, width, height, Okular::Rotation0 );
        pagesVector[ realdirs ] = page;

        m_pageMapping[ realdirs ] = i;

        ++realdirs;
    }

    pagesVector.resize( realdirs );
}

bool TIFFGenerator::print( QPrinter& printer )
{
    uint32 width = 0;
    uint32 height = 0;

    tdir_t dirs = TIFFNumberOfDirectories( d->tiff );

    QPainter p( &printer );

    QList<int> pageList = Okular::FilePrinter::pageList( printer, document()->pages(),
                                                         document()->bookmarkedPageList() );

    for ( tdir_t i = 0; i < pageList.count(); ++i )
    {
        if ( !TIFFSetDirectory( d->tiff, mapPage( pageList[i] - 1 ) ) )
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

int TIFFGenerator::mapPage( int page ) const
{
    QHash< int, int >::const_iterator it = m_pageMapping.find( page );
    if ( it == m_pageMapping.end() )
    {
        kWarning(TiffDebug) << "Requesting unmapped page" << page << ":" << m_pageMapping;
        return -1;
    }
    return it.value();
}

#include "generator_tiff.moc"

