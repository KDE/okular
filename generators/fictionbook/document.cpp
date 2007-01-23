/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QFile>

#include <kzip.h>

#include "document.h"

using namespace FictionBook;

Document::Document( const QString &fileName )
    : mFileName( fileName )
{
}

bool Document::open()
{
    QIODevice *device;

    QFile file( mFileName );
    KZip zip( mFileName );
    if ( mFileName.endsWith( ".fb" ) || mFileName.endsWith( ".fb2" ) ) {
        if ( !file.open( QIODevice::ReadOnly ) )
            return false;

        device = &file;
    } else {
        if ( !zip.open( QIODevice::ReadOnly ) )
            return false;

        const KArchiveDirectory *directory = zip.directory();
        if ( !directory )
            return false;

        const QStringList entries = directory->entries();

        QString documentFile;
        for ( int i = 0; i < entries.count(); ++i ) {
            if ( entries[ i ].endsWith( ".fb2" ) ) {
                documentFile = entries[ i ];
                break;
            }
        }

        if ( documentFile.isEmpty() )
            return false;

        const KArchiveFile *entry = static_cast<const KArchiveFile*>( directory->entry( documentFile ) );
        // FIXME delete 'deviceì somewhen
        device = entry->createDevice();
    }

    QString errorMsg;
    int errorRow, errorColumn;

    if ( !mDocument.setContent( device, true, &errorMsg, &errorRow, &errorColumn ) ) {
        qDebug( "%s at (%d,%d)", qPrintable( errorMsg ), errorRow, errorColumn );
        return false;
    }

    return true;
}

QDomDocument Document::content() const
{
    return mDocument;
}
