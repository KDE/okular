/***************************************************************************
 *   Copyright (C) 2007, 2009 by Brad Hards <bradh@frogmouth.net>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "manifest.h"
#include "debug.h"

#include <QBuffer>
#include <QXmlStreamReader>

#include <KFilterDev>
#include <KLocale>
#include <KMessageBox>
#include <KPasswordDialog>
#include <KWallet/Wallet>

using namespace OOO;

//---------------------------------------------------------------------

ManifestEntry::ManifestEntry( const QString &fileName ) :
  m_fileName( fileName )
{
}

void ManifestEntry::setMimeType( const QString &mimeType )
{
  m_mimeType = mimeType;
}

void ManifestEntry::setSize( const QString &size )
{
  m_size = size;
}

QString ManifestEntry::fileName() const
{
  return m_fileName;
}

QString ManifestEntry::mimeType() const
{
  return m_mimeType;
}

QString ManifestEntry::size() const
{
  return m_size;
}

void ManifestEntry::setChecksumType( const QString &checksumType )
{
  m_checksumType = checksumType;
}

QString ManifestEntry::checksumType() const
{
  return m_checksumType;
}

void ManifestEntry::setChecksum( const QString &checksum )
{
  m_checksum = QByteArray::fromBase64( checksum.toAscii() );
}

QByteArray ManifestEntry::checksum() const
{
  return m_checksum;
}

void ManifestEntry::setAlgorithm( const QString &algorithm )
{
  m_algorithm = algorithm;
}

QString ManifestEntry::algorithm() const
{
  return m_algorithm;
}

void ManifestEntry::setInitialisationVector( const QString &initialisationVector )
{
  m_initialisationVector = QByteArray::fromBase64( initialisationVector.toAscii() );
}

QByteArray ManifestEntry::initialisationVector() const
{
  return m_initialisationVector;
}

void ManifestEntry::setKeyDerivationName( const QString &keyDerivationName )
{
  m_keyDerivationName = keyDerivationName;
}

QString ManifestEntry::keyDerivationName() const
{
  return m_keyDerivationName;
}

void ManifestEntry::setIterationCount( const QString &iterationCount )
{
  m_iterationCount = iterationCount.toInt();
}

int ManifestEntry::iterationCount() const
{
  return m_iterationCount;
}

void ManifestEntry::setSalt( const QString &salt )
{
  m_salt = QByteArray::fromBase64( salt.toAscii() );
}

QByteArray ManifestEntry::salt() const
{
  return m_salt;
}

//---------------------------------------------------------------------

Manifest::Manifest( const QString &odfFileName, const QByteArray &manifestData )
  : m_odfFileName( odfFileName ), m_haveGoodPassword( false ), m_userCancelled( false )
{
  // I don't know why the parser barfs on this.
  QByteArray manifestCopy = manifestData;
  manifestCopy.replace(QByteArray("DOCTYPE manifest:manifest"), QByteArray("DOCTYPE manifest"));

  QXmlStreamReader xml( manifestCopy );

  ManifestEntry *currentEntry = 0;
  while ( ! xml.atEnd() ) {
    xml.readNext();
    if ( (xml.tokenType() == QXmlStreamReader::NoToken) ||
	 (xml.tokenType() == QXmlStreamReader::Invalid) ||
	 (xml.tokenType() == QXmlStreamReader::StartDocument) ||
	 (xml.tokenType() == QXmlStreamReader::EndDocument) ||
	 (xml.tokenType() == QXmlStreamReader::DTD) || 
	 (xml.tokenType() == QXmlStreamReader::Characters) ) {
      continue;
    }
    if (xml.tokenType() == QXmlStreamReader::StartElement) {
      if ( xml.name().toString() == "manifest" ) {
	continue;
      } else if ( xml.name().toString() == "file-entry" ) {
	QXmlStreamAttributes attributes = xml.attributes();
	if (currentEntry != 0) {
	  kWarning(OooDebug) << "Got new StartElement for new file-entry, but haven't finished the last one yet!";
	  kWarning(OooDebug) << "processing" << currentEntry->fileName() << ", got" << attributes.value("manifest:full-path").toString();
	}
	currentEntry = new ManifestEntry( attributes.value("manifest:full-path").toString() );
	currentEntry->setMimeType( attributes.value("manifest:media-type").toString() );
	currentEntry->setSize( attributes.value("manifest:size").toString() );
      } else if ( xml.name().toString() == "encryption-data" ) {
	if (currentEntry == 0) {
	  kWarning(OooDebug) << "Got encryption-data without valid file-entry at line" << xml.lineNumber();
	  continue;
	}
	QXmlStreamAttributes encryptionAttributes = xml.attributes();
	currentEntry->setChecksumType( encryptionAttributes.value("manifest:checksum-type").toString() );
	currentEntry->setChecksum( encryptionAttributes.value("manifest:checksum").toString() );
      } else if ( xml.name().toString() == "algorithm" ) {
	if (currentEntry == 0) {
	  kWarning(OooDebug) << "Got algorithm without valid file-entry at line" << xml.lineNumber();
	  continue;
	}
	QXmlStreamAttributes algorithmAttributes = xml.attributes();
	currentEntry->setAlgorithm( algorithmAttributes.value("manifest:algorithm-name").toString() );
	currentEntry->setInitialisationVector( algorithmAttributes.value("manifest:initialisation-vector").toString() );
      } else if ( xml.name().toString() == "key-derivation" ) {
	if (currentEntry == 0) {
	  kWarning(OooDebug) << "Got key-derivation without valid file-entry at line" << xml.lineNumber();
	  continue;
	}
	QXmlStreamAttributes kdfAttributes = xml.attributes();
	currentEntry->setKeyDerivationName( kdfAttributes.value("manifest:key-derivation-name").toString() );
	currentEntry->setIterationCount( kdfAttributes.value("manifest:iteration-count").toString() );
	currentEntry->setSalt( kdfAttributes.value("manifest:salt").toString() );
      } else {
	// handle other StartDocument types here 
	kWarning(OooDebug) << "Unexpected start document type: " << xml.name().toString();
      }
    } else if ( xml.tokenType() == QXmlStreamReader::EndElement ) {
      if ( xml.name().toString() == "manifest" ) {
	continue;
      } else if ( xml.name().toString() == "file-entry") {
	if (currentEntry == 0) {
	  kWarning(OooDebug) << "Got EndElement for file-entry without valid StartElement at line" << xml.lineNumber();
	  continue;
	}
	// we're finished processing that file entry
	if ( mEntries.contains( currentEntry->fileName() ) ) {
	  kWarning(OooDebug) << "Can't insert entry because of duplicate name:" << currentEntry->fileName();
	  delete currentEntry;
	} else {
	  mEntries.insert( currentEntry->fileName(), currentEntry);
	}
        currentEntry = 0;
      }
    }
  }
  if (xml.hasError()) {
    kWarning(OooDebug) << "error: " << xml.errorString() << xml.lineNumber() << xml.columnNumber();
  }
}

Manifest::~Manifest()
{
  savePasswordToWallet();

  qDeleteAll( mEntries );
}

ManifestEntry* Manifest::entryByName( const QString &filename )
{
  return mEntries.value( filename, 0 );
}

bool Manifest::testIfEncrypted( const QString &filename )
{
  ManifestEntry *entry = entryByName( filename );

  if (entry) {
    return ( entry->salt().length() > 0 );
  }

  return false;
}

void Manifest::getPasswordFromUser()
{
  // TODO: This should have a proper parent
  KPasswordDialog dlg( 0, KPasswordDialog::KPasswordDialogFlags() );
  dlg.setCaption( i18n( "Document Password" ) );
  dlg.setPrompt( i18n( "Please enter the password to read the document:" ) );
  if( ! dlg.exec() ) {
    // user cancel
    m_userCancelled = true;
  } else {
    m_password = dlg.password();
  }
}

void Manifest::getPasswordFromWallet()
{
  if ( KWallet::Wallet::folderDoesNotExist( KWallet::Wallet::LocalWallet(), KWallet::Wallet::PasswordFolder() ) ) {
    return;
  }

  if ( m_odfFileName.isEmpty() ) {
    return;
  }
  // This naming is consistent with how KOffice does it.
  QString entryKey = m_odfFileName + "/opendocument";

  if ( KWallet::Wallet::keyDoesNotExist( KWallet::Wallet::LocalWallet(), KWallet::Wallet::PasswordFolder(), entryKey ) ) {
    return;
  }

  // TODO: this should have a proper parent. I can't see a way to get one though...
  KWallet::Wallet *wallet = KWallet::Wallet::openWallet( KWallet::Wallet::LocalWallet(), 0 );
  if ( ! wallet ) {
    return;
  }

  if ( ! wallet->setFolder( KWallet::Wallet::PasswordFolder() ) ) {
    delete wallet;
    return;
  }

  wallet->readPassword( entryKey, m_password );
  delete wallet;
}

void Manifest::savePasswordToWallet()
{
  if ( ! m_haveGoodPassword ) {
    return;
  }

  if ( m_odfFileName.isEmpty() ) {
    return;
  }

  // TODO: this should have a proper parent. I can't see a way to get one though...
  KWallet::Wallet *wallet = KWallet::Wallet::openWallet( KWallet::Wallet::LocalWallet(), 0 );
  if ( ! wallet ) {
    return;
  }

  if ( ! wallet->hasFolder( KWallet::Wallet::PasswordFolder() ) ) {
    wallet->createFolder( KWallet::Wallet::PasswordFolder() );
  }

  if ( ! wallet->setFolder( KWallet::Wallet::PasswordFolder() ) ) {
    delete wallet;
    return;
  }

  // This naming is consistent with how KOffice does it.
  QString entryKey = m_odfFileName + "/opendocument";

  if ( wallet->hasEntry( entryKey ) ) {
    wallet->removeEntry( entryKey );
  }

  wallet->writePassword( entryKey, m_password );

  delete wallet;
}

void Manifest::checkPassword( ManifestEntry *entry, const QByteArray &fileData, QByteArray *decryptedData )
{
#ifdef QCA2
  QCA::SymmetricKey key = QCA::PBKDF2( "sha1" ).makeKey( QCA::Hash( "sha1" ).hash( m_password.toLocal8Bit() ),
							 QCA::InitializationVector( entry->salt() ),
							 16, //128 bit key
							 entry->iterationCount() );

  QCA::Cipher decoder( "blowfish", QCA::Cipher::CFB, QCA::Cipher::DefaultPadding,
		       QCA::Decode, key, QCA::InitializationVector( entry->initialisationVector() ) );
  *decryptedData = decoder.update( QCA::MemoryRegion(fileData) ).toByteArray();
  *decryptedData += decoder.final().toByteArray();

  QByteArray csum;
  if ( entry->checksumType() == "SHA1/1K" ) {
    csum = QCA::Hash( "sha1").hash( decryptedData->left(1024) ).toByteArray();
  } else if ( entry->checksumType() == "SHA1" ) {
    csum = QCA::Hash( "sha1").hash( *decryptedData ).toByteArray();
  } else {
    kDebug(OooDebug) << "unknown checksum type: " << entry->checksumType();
    // we can only assume it will be OK.
    m_haveGoodPassword = true;
    return;
  }

  if ( entry->checksum() == csum ) {
    m_haveGoodPassword = true;
  } else {
    m_haveGoodPassword = false;
  }
#else
  m_haveGoodPassword = false;
#endif
}

QByteArray Manifest::decryptFile( const QString &filename, const QByteArray &fileData )
{
#ifdef QCA2
  ManifestEntry *entry = entryByName( filename );

  if ( ! QCA::isSupported( "sha1" ) ) {
    KMessageBox::error( 0, i18n("This document is encrypted, and crypto support is compiled in, but a hashing plugin could not be located") );
    // in the hope that it wasn't really encrypted...
    return QByteArray( fileData );
  }

  if ( ! QCA::isSupported( "pbkdf2(sha1)") )  {
    KMessageBox::error( 0, i18n("This document is encrypted, and crypto support is compiled in, but a key derivation plugin could not be located") );
    // in the hope that it wasn't really encrypted...
    return QByteArray( fileData );
  }

  if ( ! QCA::isSupported( "blowfish-cfb") )  {
    KMessageBox::error( 0, i18n("This document is encrypted, and crypto support is compiled in, but a cipher plugin could not be located") );
    // in the hope that it wasn't really encrypted...
    return QByteArray( fileData );
  }

  if (m_userCancelled) {
    return QByteArray();
  }


  QByteArray decryptedData;
  if (! m_haveGoodPassword ) {
    getPasswordFromWallet();
    checkPassword( entry, fileData, &decryptedData );
  }

  do {
    if (! m_haveGoodPassword ) {
      getPasswordFromUser();
    }

    if (m_userCancelled) {
      return QByteArray();
    }

    checkPassword( entry, fileData, &decryptedData );
    if ( !m_haveGoodPassword ) {
      KMessageBox::information( 0,  i18n("The password is not correct."), i18n("Incorrect password") );
    } else {
      // kDebug(OooDebug) << "Have good password";
    }

  } while ( ( ! m_haveGoodPassword ) && ( ! m_userCancelled ) );

  if ( m_haveGoodPassword ) {
    QIODevice *decompresserDevice = KFilterDev::device( new QBuffer( &decryptedData, 0 ), "application/x-gzip", true );
    if( !decompresserDevice ) {
      kDebug(OooDebug) << "Couldn't create decompressor";
      // hopefully it isn't compressed then!
      return QByteArray( fileData );
    }

    static_cast<KFilterDev*>( decompresserDevice )->setSkipHeaders( );

    decompresserDevice->open( QIODevice::ReadOnly );

    return decompresserDevice->readAll();

  } else {
    return QByteArray( fileData );
  }    
#else
  // TODO: This should have a proper parent
  KMessageBox::error( 0, i18n("This document is encrypted, but Okular was compiled without crypto support. This document will probably not open.") );
  // this is equivalent to what happened before all this Manifest stuff :-)
  return QByteArray( fileData );
#endif
}
