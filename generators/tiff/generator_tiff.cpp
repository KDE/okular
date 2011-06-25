/***************************************************************************
 *   Copyright (C) 2006 by Pino Toscano <toscano.pino@tiscali.it>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_tiff.h"

#include <qbuffer.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qlist.h>
#include <qpainter.h>
#include <QtGui/QPrinter>

#include <kaboutdata.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>

#include <core/document.h>
#include <core/page.h>
#include <core/fileprinter.h>
#include <core/utils.h>

#include <tiff.h>
#include <tiffio.h>

#define TiffDebug 4714

tsize_t okular_tiffReadProc( thandle_t handle, tdata_t buf, tsize_t size )
{
    QIODevice * device = static_cast< QIODevice * >( handle );
    return device->isReadable() ? device->read( static_cast< char * >( buf ), size ) : -1;
}

tsize_t okular_tiffWriteProc( thandle_t handle, tdata_t buf, tsize_t size )
{
    QIODevice * device = static_cast< QIODevice * >( handle );
    return device->write( static_cast< char * >( buf ), size );
}

toff_t okular_tiffSeekProc( thandle_t handle, toff_t offset, int whence )
{
    QIODevice * device = static_cast< QIODevice * >( handle );
    switch ( whence )
    {
        case SEEK_SET:
            device->seek( offset );
            break;
        case SEEK_CUR:
            device->seek( device->pos() + offset );
            break;
        case SEEK_END:
            device->seek( device->size() + offset );
            break;
    }

    return device->pos();
}

int okular_tiffCloseProc( thandle_t handle )
{
    Q_UNUSED( handle )
    return 0;
}

toff_t okular_tiffSizeProc( thandle_t handle )
{
    QIODevice * device = static_cast< QIODevice * >( handle );
    return device->size();
}

int okular_tiffMapProc( thandle_t, tdata_t *, toff_t * )
{
    return 0;
}

void okular_tiffUnmapProc( thandle_t, tdata_t, toff_t )
{
}


class TIFFGenerator::Private
{
    public:
        Private()
          : tiff( 0 ), dev( 0 ) {}

        TIFF* tiff;
        QByteArray data;
        QIODevice* dev;
};

static QDateTime convertTIFFDateTime( const char* tiffdate )
{
    if ( !tiffdate )
        return QDateTime();

    return QDateTime::fromString( QString::fromLatin1( tiffdate ), "yyyy:MM:dd HH:mm:ss" );
}

static void adaptSizeToResolution( TIFF *tiff, ttag_t whichres, double dpi, uint32 *size )
{
    float resvalue = 1.0;
    uint16 resunit = 0;
    if ( !TIFFGetField( tiff, whichres, &resvalue )
         || !TIFFGetFieldDefaulted( tiff, TIFFTAG_RESOLUTIONUNIT, &resunit ) )
        return;

    float newsize = *size / resvalue;
    switch ( resunit )
    {
        case RESUNIT_INCH:
            *size = (uint32)( newsize * dpi );
            break;
        case RESUNIT_CENTIMETER:
            *size = (uint32)( newsize * 10.0 / 25.4 * dpi );
            break;
        case RESUNIT_NONE:
            break;
    }
}

static Okular::Rotation readTiffRotation( TIFF *tiff )
{
    uint32 tiffOrientation = 0;

    if ( !TIFFGetField( tiff, TIFFTAG_ORIENTATION, &tiffOrientation ) )
        return Okular::Rotation0;

    Okular::Rotation ret = Okular::Rotation0;
    switch ( tiffOrientation )
    {
        case ORIENTATION_TOPLEFT:
        case ORIENTATION_TOPRIGHT:
            ret = Okular::Rotation0;
            break;
        case ORIENTATION_BOTRIGHT:
        case ORIENTATION_BOTLEFT:
            ret = Okular::Rotation180;
            break;
        case ORIENTATION_LEFTTOP:
        case ORIENTATION_LEFTBOT:
            ret = Okular::Rotation270;
            break;
        case ORIENTATION_RIGHTTOP:
        case ORIENTATION_RIGHTBOT:
            ret = Okular::Rotation90;
            break;
    }

    return ret;
}

static KAboutData createAboutData()
{
    KAboutData aboutData(
         "okular_tiff",
         "okular_tiff",
         ki18n( "TIFF Backend" ),
         "0.2.4",
         ki18n( "A TIFF backend" ),
         KAboutData::License_GPL,
         ki18n( "© 2006-2008 Pino Toscano" ),
         ki18nc( "This represents the libtiff version, as string with copyrights as well; can be left as-is.", "%1" ).subs( TIFFGetVersion() )
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
    setFeature( ReadRawData );
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
    QFile* qfile = new QFile( fileName );
    qfile->open( QIODevice::ReadOnly );
    d->dev = qfile;
    d->data = QFile::encodeName( QFileInfo( *qfile ).fileName() );
    return loadTiff( pagesVector, d->data.constData() );
}

bool TIFFGenerator::loadDocumentFromData( const QByteArray & fileData, QVector< Okular::Page * > & pagesVector )
{
    d->data = fileData;
    QBuffer* qbuffer = new QBuffer( &d->data );
    qbuffer->open( QIODevice::ReadOnly );
    d->dev = qbuffer;
    return loadTiff( pagesVector, "<stdin>" );
}

bool TIFFGenerator::loadTiff( QVector< Okular::Page * > & pagesVector, const char *name )
{
    d->tiff = TIFFClientOpen( name, "r", d->dev,
                  okular_tiffReadProc, okular_tiffWriteProc, okular_tiffSeekProc,
                  okular_tiffCloseProc, okular_tiffSizeProc,
                  okular_tiffMapProc, okular_tiffUnmapProc );
    if ( !d->tiff )
    {
        delete d->dev;
        d->dev = 0;
        d->data.clear();
        return false;
    }

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
        delete d->dev;
        d->dev = 0;
        d->data.clear();
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
        uint32 width = 1;
        uint32 height = 1;
        uint32 orientation = 0;
        TIFFGetField( d->tiff, TIFFTAG_IMAGEWIDTH, &width );
        TIFFGetField( d->tiff, TIFFTAG_IMAGELENGTH, &height );

        if ( !TIFFGetField( d->tiff, TIFFTAG_ORIENTATION, &orientation ) )
            orientation = ORIENTATION_TOPLEFT;

        QImage image( width, height, QImage::Format_RGB32 );
        uint32 * data = (uint32 *)image.bits();

        // read data
        if ( TIFFReadRGBAImageOriented( d->tiff, width, height, data, orientation ) != 0 )
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
    m_docInfo->set( Okular::DocumentInfo::Description, buffer ? QString::fromLatin1( buffer ) : i18nc( "Unknown description", "Unknown" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_SOFTWARE, &buffer );
    m_docInfo->set( Okular::DocumentInfo::Producer, buffer ? QString::fromLatin1( buffer ) : i18nc( "Unknown producer", "Unknown" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_COPYRIGHT, &buffer );
    m_docInfo->set( Okular::DocumentInfo::Copyright, buffer ? QString::fromLatin1( buffer ) : i18nc( "Unknown copyright statement", "Unknown" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_ARTIST, &buffer );
    m_docInfo->set( Okular::DocumentInfo::Author, buffer ? QString::fromLatin1( buffer ) : i18nc( "Unknown author", "Unknown" ) );

    buffer = 0;
    TIFFGetField( d->tiff, TIFFTAG_DATETIME, &buffer );
    QDateTime date = convertTIFFDateTime( buffer );
    m_docInfo->set( Okular::DocumentInfo::CreationDate, date.isValid() ? KGlobal::locale()->formatDateTime( date, KLocale::LongDate, true  ) : i18nc( "Unknown creation date", "Unknown" ) );

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

    const double dpiX = Okular::Utils::dpiX();
    const double dpiY = Okular::Utils::dpiY();

    for ( tdir_t i = 0; i < dirs; ++i )
    {
        if ( !TIFFSetDirectory( d->tiff, i ) )
            continue;

        if ( TIFFGetField( d->tiff, TIFFTAG_IMAGEWIDTH, &width ) != 1 ||
             TIFFGetField( d->tiff, TIFFTAG_IMAGELENGTH, &height ) != 1 )
            continue;

        adaptSizeToResolution( d->tiff, TIFFTAG_XRESOLUTION, dpiX, &width );
        adaptSizeToResolution( d->tiff, TIFFTAG_YRESOLUTION, dpiY, &height );

        Okular::Page * page = new Okular::Page( realdirs, width, height, readTiffRotation( d->tiff ) );
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

    QPainter p( &printer );

    QList<int> pageList = Okular::FilePrinter::pageList( printer, document()->pages(),
                                                         document()->currentPage() + 1,
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

        QSize targetSize = printer.pageRect().size();

        if ( (image.width() < targetSize.width()) && (image.height() < targetSize.height()) )
        {
            // draw small images at 100% (don't scale up)
            p.drawImage( 0, 0, image );
        } else {
            // fit to page
            p.drawImage( 0, 0, image.scaled( targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
        }
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

