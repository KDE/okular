/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <aacid@kde.org>               *
 *   Copyright (C) 2006-2007 by Pino Toscano <pino@kde.org>                *
 *   Copyright (C) 2006-2007 by Tobias Koenig <tokoe@kde.org>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_kimgio.h"

#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QPainter>
#include <QPrinter>
#include <QMimeType>
#include <QMimeDatabase>

#include <KAboutData>
#include <qaction.h>
#include <kactioncollection.h>
#include <QIcon>
#include <KLocalizedString>

#include <kexiv2/kexiv2.h>

#include <core/page.h>

OKULAR_EXPORT_PLUGIN(KIMGIOGenerator, "libokularGenerator_kimgio.json")

KIMGIOGenerator::KIMGIOGenerator( QObject *parent, const QVariantList &args )
    : Generator( parent, args )
{
    setFeature( ReadRawData );
    setFeature( Threaded );
    setFeature( TiledRendering );
    setFeature( PrintNative );
    setFeature( PrintToFile );
}

KIMGIOGenerator::~KIMGIOGenerator()
{
}

bool KIMGIOGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    QFile f( fileName );
    if ( !f.open(QFile::ReadOnly) ) {
        emit error( i18n( "Unable to load document: %1", f.errorString() ), -1 );
        return false;
    }
    return loadDocumentInternal( f.readAll(), fileName, pagesVector );
}

bool KIMGIOGenerator::loadDocumentFromData( const QByteArray & fileData, QVector<Okular::Page*> & pagesVector )
{
    return loadDocumentInternal( fileData, QString(), pagesVector );
}

bool KIMGIOGenerator::loadDocumentInternal(const QByteArray & fileData, const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    QBuffer buffer;
    buffer.setData( fileData );
    buffer.open( QIODevice::ReadOnly );

    QImageReader reader( &buffer, QImageReader::imageFormat( &buffer ) );
    reader.setAutoDetectImageFormat( true );
    if ( !reader.read( &m_img ) ) {
        emit error( i18n( "Unable to load document: %1", reader.errorString() ), -1 );
        return false;
    }
    QMimeDatabase db;
    auto mime = db.mimeTypeForFileNameAndData( fileName, fileData );
    docInfo.set( Okular::DocumentInfo::MimeType, mime.name() );

    // Apply transformations dictated by Exif metadata
    KExiv2Iface::KExiv2 exifMetadata;
    if ( exifMetadata.loadFromData( fileData ) ) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0) && QT_VERSION < QT_VERSION_CHECK(5, 4, 2)
        // Qt 5.4 (up to 5.4.1) rotates jpeg images automatically with no way of disabling it
        // See https://bugreports.qt.io/browse/QTBUG-37946
        // and https://codereview.qt-project.org/#/c/98013/
        // and https://codereview.qt-project.org/#/c/110668/
        if (reader.format() != QByteArrayLiteral("jpeg")) {
            exifMetadata.rotateExifQImage( m_img, exifMetadata.getImageOrientation() );
        }
#else
        exifMetadata.rotateExifQImage(m_img, exifMetadata.getImageOrientation());
#endif
    }

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    return true;
}

bool KIMGIOGenerator::doCloseDocument()
{
    m_img = QImage();

    return true;
}

QImage KIMGIOGenerator::image( Okular::PixmapRequest * request )
{
    // perform a smooth scaled generation
    if ( request->isTile() )
    {
        const QRect srcRect = request->normalizedRect().geometry( m_img.width(), m_img.height() );
        const QRect destRect = request->normalizedRect().geometry( request->width(), request->height() );

        QImage destImg( destRect.size(), QImage::Format_RGB32 );
        destImg.fill( Qt::white );

        QPainter p( &destImg );
        p.setRenderHint( QPainter::SmoothPixmapTransform );
        p.drawImage( destImg.rect(), m_img, srcRect );

        return destImg;
    }
    else
    {
        int width = request->width();
        int height = request->height();
        if ( request->page()->rotation() % 2 == 1 )
            qSwap( width, height );

        return m_img.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    }
}

bool KIMGIOGenerator::print( QPrinter& printer )
{
    QPainter p( &printer );

    QImage image( m_img );

    if ( ( image.width() > printer.width() ) || ( image.height() > printer.height() ) )

        image = image.scaled( printer.width(), printer.height(),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation );

    p.drawImage( 0, 0, image );

    return true;
}

Okular::DocumentInfo KIMGIOGenerator::generateDocumentInfo( const QSet<Okular::DocumentInfo::Key> &keys ) const
{
    Q_UNUSED(keys);

    return docInfo;
}

#include "generator_kimgio.moc"

