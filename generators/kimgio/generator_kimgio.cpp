/***************************************************************************
 *   Copyright (C) 2005 by Albert Astals Cid <aacid@kde.org>               *
 *   Copyright (C) 2006-2007 by Pino Toscano <pino@kde.org>                *
 *   Copyright (C) 2006-2007 by Tobias Koenig <tokoe@kde.org>              *
 *   Copyright (C) 2017      Klarälvdalens Datakonsult AB, a KDAB Group    *
 *                           company, info@kdab.com. Work sponsored by the *
 *                           LiMux project of the city of Munich           *
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
    setFeature( SwapBackingFile );
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
        if (!m_img.isNull()) {
            emit warning( i18n( "This document appears malformed. Here is a best approximation of the document's intended appearance." ), -1 );
        } else {
            emit error( i18n( "Unable to load document: %1", reader.errorString() ), -1 );
            return false;
        }
    }
    QMimeDatabase db;
    auto mime = db.mimeTypeForFileNameAndData( fileName, fileData );
    docInfo.set( Okular::DocumentInfo::MimeType, mime.name() );

    // Apply transformations dictated by Exif metadata
    KExiv2Iface::KExiv2 exifMetadata;
    if ( exifMetadata.loadFromData( fileData ) ) {
        exifMetadata.rotateExifQImage(m_img, exifMetadata.getImageOrientation());
    }

    pagesVector.resize( 1 );

    Okular::Page * page = new Okular::Page( 0, m_img.width(), m_img.height(), Okular::Rotation0 );
    pagesVector[0] = page;

    return true;
}

KIMGIOGenerator::SwapBackingFileResult KIMGIOGenerator::swapBackingFile( QString const &/*newFileName*/, QVector<Okular::Page*> & /*newPagesVector*/ )
{
    // NOP: We don't actually need to do anything because all data has already
    // been loaded in RAM
    return SwapBackingFileNoOp;
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

