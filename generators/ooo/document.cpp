/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "document.h"

#include <klocale.h>
#include <kzip.h>

using namespace OOO;

Document::Document( const QString &fileName )
  : mFileName( fileName )
{
}

bool Document::open()
{
  mContent.clear();
  mStyles.clear();

  KZip zip( mFileName );
  if ( !zip.open( QIODevice::ReadOnly ) ) {
    setError( i18n( "Document is not a valid ZIP archive" ) );
    return false;
  }

  const KArchiveDirectory *directory = zip.directory();
  if ( !directory ) {
    setError( i18n( "Invalid document structure (main directory is missing)" ) );
    return false;
  }

  const QStringList entries = directory->entries();
  if ( !entries.contains( "content.xml" ) ) {
    setError( i18n( "Invalid document structure (content.xml is missing)" ) );
    return false;
  }

  const KArchiveFile *file = static_cast<const KArchiveFile*>( directory->entry( "content.xml" ) );
  mContent = file->data();

  if ( !entries.contains( "styles.xml" ) ) {
    setError( i18n( "Invalid document structure (styles.xml is missing)" ) );
    return false;
  }

  file = static_cast<const KArchiveFile*>( directory->entry( "styles.xml" ) );
  mStyles = file->data();

  if ( !entries.contains( "meta.xml" ) ) {
    setError( i18n( "Invalid document structure (meta.xml is missing)" ) );
    return false;
  }

  file = static_cast<const KArchiveFile*>( directory->entry( "meta.xml" ) );
  mMeta = file->data();

  if ( entries.contains( "Pictures" ) ) {
    const KArchiveDirectory *imagesDirectory = static_cast<const KArchiveDirectory*>( directory->entry( "Pictures" ) );

    const QStringList imagesEntries = imagesDirectory->entries();
    for ( int i = 0; i < imagesEntries.count(); ++i ) {
      file = static_cast<const KArchiveFile*>( imagesDirectory->entry( imagesEntries[ i ] ) );
      mImages.insert( QString( "Pictures/%1" ).arg( imagesEntries[ i ] ), file->data() );
    }
  }

  zip.close();

  return true;
}

QString Document::lastErrorString() const
{
  return mErrorString;
}

QByteArray Document::content() const
{
  return mContent;
}

QByteArray Document::meta() const
{
  return mMeta;
}

QByteArray Document::styles() const
{
  return mStyles;
}

QMap<QString, QByteArray> Document::images() const
{
  return mImages;
}

void Document::setError( const QString &error )
{
  mErrorString = error;
}
