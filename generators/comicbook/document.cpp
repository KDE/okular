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

#include <kzip.h>

#include "unrar.h"
#include "qnatsort.h"

using namespace ComicBook;

Document::Document()
    : mUnrar( 0 ), mZip( 0 )
{
}

Document::~Document()
{
    delete mUnrar;
    delete mZip;
}

bool Document::open( const QString &fileName )
{
    delete mZip;
    delete mUnrar;
    mPageMap.clear();

    /**
     * We have a zip archive
     */
    if ( fileName.toLower().endsWith( ".cbz" ) ) {
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
        QStringList entries = directory->entries();
        if ( entries.count() == 1 ) {
            // seems to be a nested directory
            const KArchiveEntry *entry = directory->entry( entries[ 0 ] );
            if ( entry->isDirectory() ) {
                entries = static_cast<const KArchiveDirectory*>( entry )->entries();
                mZipDir = const_cast<KArchiveDirectory*>( static_cast<const KArchiveDirectory*>( entry ) );
            }
        }

        extractImageFiles( entries );

    } else {
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
    }

    return true;
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
