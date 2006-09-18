/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <kzip.h>

#include "document.h"

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
    return false;
  }

  const KArchiveDirectory *directory = zip.directory();
  if ( !directory ) {
    return false;
  }

  const QStringList entries = directory->entries();
  if ( !entries.contains( "content.xml" ) ) {
    return false;
  }

  const KArchiveFile *file = static_cast<const KArchiveFile*>( directory->entry( "content.xml" ) );
  mContent = file->data();

  if ( !entries.contains( "styles.xml" ) ) {
    return false;
  }

  file = static_cast<const KArchiveFile*>( directory->entry( "styles.xml" ) );
  mStyles = file->data();

  if ( !entries.contains( "meta.xml" ) ) {
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
