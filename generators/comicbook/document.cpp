/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "document.h"

#include <QtGui/QImage>
#include <QtGui/QImageReader>

#include <klocale.h>
#include <kmimetype.h>
#include <kzip.h>

#include <memory>

#include "unrar.h"
#include "qnatsort.h"

using namespace ComicBook;

static void imagesInArchive( const QString &prefix, const KArchiveDirectory* dir, QStringList *entries )
{
    Q_FOREACH ( const QString &entry, dir->entries() ) {
        const KArchiveEntry *e = dir->entry( entry );
        if ( e->isDirectory() ) {
            imagesInArchive( prefix + entry + "/", static_cast<const KArchiveDirectory*>( e ), entries );
        } else if ( e->isFile() ) {
            entries->append( prefix + entry );
        }
    }
}


Document::Document()
    : mUnrar( 0 ), mZip( 0 )
{
}

Document::~Document()
{
}

bool Document::open( const QString &fileName )
{
    close();

    const KMimeType::Ptr mime = KMimeType::findByFileContent( fileName );

    /**
     * We have a zip archive
     */
    if ( mime->is( "application/x-cbz" ) || mime->name() == "application/zip" ) {
        mZip = new KZip( fileName );

        if ( !mZip->open( QIODevice::ReadOnly ) ) {
            delete mZip;
            mZip = 0;

            return false;
        }

        const KArchiveDirectory *directory = mZip->directory();
        if ( !directory ) {
            delete mZip;
            mZip = 0;

            return false;
        }

        mZipDir = const_cast<KArchiveDirectory*>( directory );

        QStringList entries;
        imagesInArchive( QString(), mZipDir, &entries );

        extractImageFiles( entries );

    } else if ( mime->is( "application/x-cbr" ) || mime->name() == "application/x-rar" ) {
        if ( !Unrar::isAvailable() ) {
            mLastErrorString = i18n( "Cannot open document, unrar was not found." );
            return false;
        }

        if ( !Unrar::isSuitableVersionAvailable() ) {
            mLastErrorString = i18n( "The version of unrar on your system is not suitable for opening comicbooks." );
            return false;
        }

        /**
         * We have a rar archive
         */
        mUnrar = new Unrar();

        if ( !mUnrar->open( fileName ) ) {
            delete mUnrar;
            mUnrar = 0;

            return false;
        }

        extractImageFiles( mUnrar->list() );
    } else {
        mLastErrorString = i18n( "Unknown ComicBook format." );
        return false;
    }

    return true;
}

void Document::close()
{
    mLastErrorString.clear();

    if ( !( mZip || mUnrar ) )
        return;

    delete mZip;
    mZip = 0;
    delete mUnrar;
    mUnrar = 0;
    mPageMap.clear();
}

void Document::extractImageFiles( const QStringList &list )
{
    QStringList files( list );

    qSort( files.begin(), files.end(), caseSensitiveNaturalOrderLessThen );

    for ( int i = 0; i < files.count(); ++i ) {
        const QString lowerFile = files[ i ].toLower();

        if ( lowerFile.endsWith( ".gif" ) ||
             lowerFile.endsWith( ".jpg" ) ||
             lowerFile.endsWith( ".jpeg" ) ||
             lowerFile.endsWith( ".png" ) )
            mPageMap.append( files[ i ] );
    }
}

int Document::pages() const
{
    return mPageMap.count();
}

QStringList Document::pageTitles() const
{
    return QStringList();
}

QImage Document::pageImage( int page ) const
{
    if ( mZip ) {
        const KArchiveFile *entry = static_cast<const KArchiveFile*>( mZipDir->entry( mPageMap[ page ] ) );
        if ( entry )
            return QImage::fromData( entry->data() );

    } else {
        return QImage::fromData( mUnrar->contentOf( mPageMap[ page ] ) );
    }

    return QImage();
}

QSize Document::pageSize( int page ) const
{
    std::auto_ptr< QIODevice > dev;

    if ( mZip ) {
        const KArchiveFile *entry = static_cast<const KArchiveFile*>( mZipDir->entry( mPageMap[ page ] ) );
        if ( entry ) {
            dev.reset( entry->createDevice() );
        }

    } else {
        dev.reset( mUnrar->createDevice( mPageMap[ page ] ) );
    }

    if ( dev.get() ) {
        QImageReader reader( dev.get() );
        if ( reader.canRead() ) {
            QSize s = reader.size();
            if ( !s.isValid() ) {
                s = reader.read().size();
            }
            return s;
        }
    }

    return QSize();
}

QString Document::lastErrorString() const
{
    return mLastErrorString;
}
