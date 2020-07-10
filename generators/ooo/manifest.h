/***************************************************************************
 *   Copyright (C) 2007 by Brad Hards <bradh@frogmouth.net>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OOO_MANIFEST_H
#define OOO_MANIFEST_H

#include <QByteArray>
#include <QMap>
#include <QString>

#ifdef QCA2
#include <QtCrypto>
#endif

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(OkularOooDebug)

namespace OOO
{
/**
   OOO document manifest entry

   This class holds the manifest information for a single file-entry element.
   An example of the XML that could be passed in is shown below, although
   not all manifest entries will have encryption-data.
\code
<manifest:file-entry manifest:media-type="image/jpeg" manifest:full-path="Pictures/100000000000032000000258912EB1C3.jpg" manifest:size="66704">
  <manifest:encryption-data>
    <manifest:algorithm manifest:algorithm-name="Blowfish CFB" manifest:initialisation-vector="T+miu403484="/>
    <manifest:key-derivation manifest:key-derivation-name="PBKDF2" manifest:iteration-count="1024" manifest:salt="aNYdmqv4cObAJSJjm4RzqA=="/>
  </manifest:encryption-data>
</manifest:file-entry>
\endcode
*/
class ManifestEntry
{
public:
    /**
       Create a new manifest entry
    */
    explicit ManifestEntry(const QString &fileName);

    /**
       Set the mimetype of the file.
    */
    void setMimeType(const QString &mimeType);

    /**
       Get the file name
    */
    QString fileName() const;

    /**
       Set the file size.

       This is only required for files with encryption
    */
    void setSize(const QString &size);

    /**
       Get the mime type
    */
    QString mimeType() const;

    /**
       Get the file size

       This is only meaningful for files with encryption
    */
    QString size() const;

    /**
       Set the checksum type.

       This is only required for files with encryption
    */
    void setChecksumType(const QString &checksumType);

    /**
       Set the checksum.

       \param checksum the checksum in base64 encoding.

       This is only required for files with encryption
    */
    void setChecksum(const QString &checksum);

    /**
       Get the checksum type

       This is only meaningful for files with encryption
    */
    QString checksumType() const;

    /**
       Get the checksum

       This is only meaningful for files with encryption
    */
    QByteArray checksum() const;

    /**
       Set the algorithm name

       This is only required for files with encryption
    */
    void setAlgorithm(const QString &algorithm);

    /**
       Get the algorithm name

       This is only meaningful for files with encryption
    */
    QString algorithm() const;

    /**
       Set the initialisation vector for the cipher algorithm

       \param initialisationVector the IV in base64 encoding.

       This is only required for files with encryption
    */
    void setInitialisationVector(const QString &initialisationVector);

    /**
       Get the initialisation vector for the cipher algorithm

       This is only meaningful for files with encryption
    */
    QByteArray initialisationVector() const;

    /**
       Set the name of the key derivation function

       This is only required for files with encryption
    */
    void setKeyDerivationName(const QString &keyDerivationName);

    /**
       Get the name of the key derivation function

       This is only meaningful for files with encryption
    */
    QString keyDerivationName() const;

    /**
       Set the iteration count for the key derivation function

       This is only required for files with encryption
    */
    void setIterationCount(const QString &iterationCount);

    /**
       Get the iteration count for the key derivation function

       This is only meaningful for files with encryption
    */
    int iterationCount() const;

    /**
       Set the salt for the key derivation function

       \param salt the salt in base64 encoding.

       This is only required for files with encryption
    */
    void setSalt(const QString &salt);

    /**
       Get the salt for the key derivation function

       This is only meaningful for files with encryption
    */
    QByteArray salt() const;

private:
    const QString m_fileName;
    QString m_mimeType;
    QString m_size;
    QString m_checksumType;
    QByteArray m_checksum;
    QString m_algorithm;
    QByteArray m_initialisationVector;
    QString m_keyDerivationName;
    int m_iterationCount;
    QByteArray m_salt;
};

/**
   OOO document manifest

   The document manifest is described in the OASIS Open Office Specification
   (Version 1.1) section 17.7. This class represents the same data.
*/
class Manifest
{
public:
    /**
       Create a new manifest.

       \param odfFileName the name of the parent ODF file
       \param manifestData the data from the manifest file, as a byte array
       of the XML format data.
       \param password to decrypt the file
    */
    Manifest(const QString &odfFileName, const QByteArray &manifestData, const QString &password);

    ~Manifest();

    Manifest(const Manifest &) = delete;
    Manifest &operator=(const Manifest &) = delete;

    /**
       Check if the manifest indicates that a file is encrypted
    */
    bool testIfEncrypted(const QString &filename);

    /**
       Decrypt data associated with a specific file
    */
    QByteArray decryptFile(const QString &filename, const QByteArray &fileData);

private:
    /**
       Retrieve the manifest data for a particular file
    */
    ManifestEntry *entryByName(const QString &filename);

    /**
       Verify whether the password we have is the right one.

       This verifies the provided checksum
    */
    void checkPassword(ManifestEntry *entry, const QByteArray &fileData, QByteArray *decryptedData);

#ifdef QCA2
    QCA::Initializer m_init;
#endif
    const QString m_odfFileName;
    QMap<QString, ManifestEntry *> mEntries;
    bool m_haveGoodPassword;
    QString m_password;
};

}

#endif
