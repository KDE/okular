/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "document.h"

#include <QtCore/QScopedPointer>
#include <QtGui/QImage>
#include <QtGui/QImageReader>

#include <klocale.h>
#include <kmimetype.h>
#include <kzip.h>
#include <ktar.h>

#include <memory>

#include <core/page.h>

#include "unrar.h"
#include "directory.h"
#include "qnatsort.h"

using namespace ComicBook;

static void imagesInArchive( const QString &prefix, const KArchiveDirectory* dir, QStringList *entries )
{
    Q_FOREACH ( const QString &entry, dir->entries() ) {
        const KArchiveEntry *e = dir->entry( entry );
        if ( e->isDirectory() ) {
            imagesInArchive( prefix + entry + '/', static_cast<const KArchiveDirectory*>( e ), entries );
        } else if ( e->isFile() ) {
            entries->append( prefix + entry );
        }
    }
}


Document::Document()
    : mDirectory( 0 ), mUnrar( 0 ), mArchive( 0 )
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
        mArchive = new KZip( fileName );

        if ( !processArchive() ) {
            return false;
        }
    /**
     * We have a TAR archive
     */
    } else if ( mime->is( "application/x-cbt" ) || mime->name() == "application/x-gzip" ||
                mime->name() == "application/x-tar" || mime->name() == "application/x-bzip" ) {
        mArchive = new KTar( fileName );

        if ( !processArchive() ) {
            return false;
        }
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

        mEntries = mUnrar->list();
    } else if ( mime->is( "inode/directory" ) ) {
        mDirectory = new Directory();

        if ( !mDirectory->open( fileName ) ) {
            delete mDirectory;
            mDirectory = 0;

            return false;
        }

        mEntries = mDirectory->list();
    } else {
        mLastErrorString = i18n( "Unknown ComicBook format." );
        return false;
    }

    return true;
}

void Document::close()
{
    mLastErrorString.clear();

    if ( !( mArchive || mUnrar || mDirectory ) )
        return;

    delete mArchive;
    mArchive = 0;
    delete mDirectory;
    mDirectory = 0;
    delete mUnrar;
    mUnrar = 0;
    mPageMap.clear();
    mEntries.clear();
}

bool Document::processArchive() {
    if ( !mArchive->open( QIODevice::ReadOnly ) ) {
        delete mArchive;
        mArchive = 0;

        return false;
    }

    const KArchiveDirectory *directory = mArchive->directory();
    if ( !directory ) {
        delete mArchive;
        mArchive = 0;

        return false;
    }

    mArchiveDir = const_cast<KArchiveDirectory*>( directory );

    imagesInArchive( QString(), mArchiveDir, &mEntries );

    return true;
}

void Document::pages( QVector<Okular::Page*> * pagesVector )
{
    qSort( mEntries.begin(), mEntries.end(), caseSensitiveNaturalOrderLessThen );
    QScopedPointer< QIODevice > dev;

    int count = 0;
    pagesVector->clear();
    pagesVector->resize( mEntries.size() );
    QImageReader reader;
    foreach(const QString &file, mEntries) {
        if ( mArchive ) {
            const KArchiveFile *entry = static_cast<const KArchiveFile*>( mArchiveDir->entry( file ) );
            if ( entry ) {
                dev.reset( entry->createDevice() );
            }
        } else if ( mDirectory ) {
            dev.reset( mDirectory->createDevice( file ) );
        } else {
            dev.reset( mUnrar->createDevice( file ) );
        }

        if ( ! dev.isNull() ) {
            reader.setDevice( dev.data() );
            if ( reader.canRead() )
            {
                QSize pageSize = reader.size();
                if ( !pageSize.isValid() ) {
                    const QImage i = reader.read();
                    if ( !i.isNull() )
                        pageSize = i.size();
                }
                if ( pageSize.isValid() ) {
                    pagesVector->replace( count, new Okular::Page( count, pageSize.width(), pageSize.height(), Okular::Rotation0 ) );
                    mPageMap.append(file);
                    count++;
                } else {
                    kDebug() << "Ignoring" << file << "doesn't seem to be an image even if QImageReader::canRead returned true";
                }
            }
        }
    }
    pagesVector->resize( count );
}

QStringList Document::pageTitles() const
{
    return QStringList();
}

QImage Document::pageImage( int page ) const
{
    if ( mArchive ) {
        const KArchiveFile *entry = static_cast<const KArchiveFile*>( mArchiveDir->entry( mPageMap[ page ] ) );
        if ( entry )
            return QImage::fromData( entry->data() );
    } else if ( mDirectory ) {
        return QImage( mPageMap[ page ] );
    } else {
        return QImage::fromData( mUnrar->contentOf( mPageMap[ page ] ) );
    }

    return QImage();
}

QString Document::lastErrorString() const
{
    return mLastErrorString;
}

