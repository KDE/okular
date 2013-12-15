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
  : mFileName( fileName ), mManifest( 0 )
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
  if ( !entries.contains( "META-INF" ) ) {
    setError( i18n( "Invalid document structure (META-INF directory is missing)" ) );
    return false;
  }
  const KArchiveDirectory *metaInfDirectory = static_cast<const KArchiveDirectory*>( directory->entry( "META-INF" ) );
  if ( !(metaInfDirectory->entries().contains( "manifest.xml" ) ) ) {
    setError( i18n( "Invalid document structure (META-INF/manifest.xml is missing)" ) );
    return false;
  }

  const KArchiveFile *file = static_cast<const KArchiveFile*>( metaInfDirectory->entry( "manifest.xml" ) );
  mManifest = new Manifest( mFileName, file->data() );

  // we should really get the file names from the manifest, but for now, we only care
  // if the manifest says the files are encrypted.

  if ( !entries.contains( "content.xml" ) ) {
    setError( i18n( "Invalid document structure (content.xml is missing)" ) );
    return false;
  }

  file = static_cast<const KArchiveFile*>( directory->entry( "content.xml" ) );
  if ( mManifest->testIfEncrypted( "content.xml" )  ) {
    mContent = mManifest->decryptFile( "content.xml", file->data() );
  } else {
    mContent = file->data();
  }

  if ( entries.contains( "styles.xml" ) ) {
    file = static_cast<const KArchiveFile*>( directory->entry( "styles.xml" ) );
    if ( mManifest->testIfEncrypted( "styles.xml" )  ) {
      mStyles = mManifest->decryptFile( "styles.xml", file->data() );
    } else {
      mStyles = file->data();
    }
  }

  if ( entries.contains( "meta.xml" ) ) {
    file = static_cast<const KArchiveFile*>( directory->entry( "meta.xml" ) );
    if ( mManifest->testIfEncrypted( "meta.xml" )  ) {
      mMeta = mManifest->decryptFile( "meta.xml", file->data() );
    } else {
      mMeta = file->data();
    }
  }

  if ( entries.contains( "Pictures" ) ) {
    const KArchiveDirectory *imagesDirectory = static_cast<const KArchiveDirectory*>( directory->entry( "Pictures" ) );

    const QStringList imagesEntries = imagesDirectory->entries();
    for ( int i = 0; i < imagesEntries.count(); ++i ) {
      file = static_cast<const KArchiveFile*>( imagesDirectory->entry( imagesEntries[ i ] ) );
      QString fullPath = QString( "Pictures/%1" ).arg( imagesEntries[ i ] );
      if ( mManifest->testIfEncrypted( fullPath ) ) {
        mImages.insert( fullPath, mManifest->decryptFile( fullPath, file->data() ) );
      } else {
        mImages.insert( fullPath, file->data() );
      }
    }
  }

  zip.close();

  return true;
}

Document::~Document()
{
  delete mManifest;
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
