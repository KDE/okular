/***************************************************************************
 *   Copyright (C) 2007, 2009 by Brad Hards <bradh@frogmouth.net>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "manifest.h"

#include <QBuffer>
#include <QXmlStreamReader>

#include <KFilterDev>
#include <KLocalizedString>
#include <KMessageBox>

using namespace OOO;

Q_LOGGING_CATEGORY(OkularOooDebug, "org.kde.okular.generators.ooo", QtWarningMsg)
//---------------------------------------------------------------------

ManifestEntry::ManifestEntry(const QString &fileName)
    : m_fileName(fileName)
{
}

void ManifestEntry::setMimeType(const QString &mimeType)
{
    m_mimeType = mimeType;
}

void ManifestEntry::setSize(const QString &size)
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

void ManifestEntry::setChecksumType(const QString &checksumType)
{
    m_checksumType = checksumType;
}

QString ManifestEntry::checksumType() const
{
    return m_checksumType;
}

void ManifestEntry::setChecksum(const QString &checksum)
{
    m_checksum = QByteArray::fromBase64(checksum.toLatin1());
}

QByteArray ManifestEntry::checksum() const
{
    return m_checksum;
}

void ManifestEntry::setAlgorithm(const QString &algorithm)
{
    m_algorithm = algorithm;
}

QString ManifestEntry::algorithm() const
{
    return m_algorithm;
}

void ManifestEntry::setInitialisationVector(const QString &initialisationVector)
{
    m_initialisationVector = QByteArray::fromBase64(initialisationVector.toLatin1());
}

QByteArray ManifestEntry::initialisationVector() const
{
    return m_initialisationVector;
}

void ManifestEntry::setKeyDerivationName(const QString &keyDerivationName)
{
    m_keyDerivationName = keyDerivationName;
}

QString ManifestEntry::keyDerivationName() const
{
    return m_keyDerivationName;
}

void ManifestEntry::setIterationCount(const QString &iterationCount)
{
    m_iterationCount = iterationCount.toInt();
}

int ManifestEntry::iterationCount() const
{
    return m_iterationCount;
}

void ManifestEntry::setSalt(const QString &salt)
{
    m_salt = QByteArray::fromBase64(salt.toLatin1());
}

QByteArray ManifestEntry::salt() const
{
    return m_salt;
}

//---------------------------------------------------------------------

Manifest::Manifest(const QString &odfFileName, const QByteArray &manifestData, const QString &password)
    : m_odfFileName(odfFileName)
    , m_haveGoodPassword(false)
    , m_password(password)
{
    // I don't know why the parser barfs on this.
    QByteArray manifestCopy = manifestData;
    manifestCopy.replace(QByteArray("DOCTYPE manifest:manifest"), QByteArray("DOCTYPE manifest"));

    QXmlStreamReader xml(manifestCopy);

    ManifestEntry *currentEntry = nullptr;
    while (!xml.atEnd()) {
        xml.readNext();
        if ((xml.tokenType() == QXmlStreamReader::NoToken) || (xml.tokenType() == QXmlStreamReader::Invalid) || (xml.tokenType() == QXmlStreamReader::StartDocument) || (xml.tokenType() == QXmlStreamReader::EndDocument) ||
            (xml.tokenType() == QXmlStreamReader::DTD) || (xml.tokenType() == QXmlStreamReader::Characters)) {
            continue;
        }
        if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name().toString() == QLatin1String("manifest")) {
                continue;
            } else if (xml.name().toString() == QLatin1String("file-entry")) {
                QXmlStreamAttributes attributes = xml.attributes();
                if (currentEntry != nullptr) {
                    qCWarning(OkularOooDebug) << "Got new StartElement for new file-entry, but haven't finished the last one yet!";
                    qCWarning(OkularOooDebug) << "processing" << currentEntry->fileName() << ", got" << attributes.value(QStringLiteral("manifest:full-path")).toString();
                }
                currentEntry = new ManifestEntry(attributes.value(QStringLiteral("manifest:full-path")).toString());
                currentEntry->setMimeType(attributes.value(QStringLiteral("manifest:media-type")).toString());
                currentEntry->setSize(attributes.value(QStringLiteral("manifest:size")).toString());
            } else if (xml.name().toString() == QLatin1String("encryption-data")) {
                if (currentEntry == nullptr) {
                    qCWarning(OkularOooDebug) << "Got encryption-data without valid file-entry at line" << xml.lineNumber();
                    continue;
                }
                QXmlStreamAttributes encryptionAttributes = xml.attributes();
                currentEntry->setChecksumType(encryptionAttributes.value(QStringLiteral("manifest:checksum-type")).toString());
                currentEntry->setChecksum(encryptionAttributes.value(QStringLiteral("manifest:checksum")).toString());
            } else if (xml.name().toString() == QLatin1String("algorithm")) {
                if (currentEntry == nullptr) {
                    qCWarning(OkularOooDebug) << "Got algorithm without valid file-entry at line" << xml.lineNumber();
                    continue;
                }
                QXmlStreamAttributes algorithmAttributes = xml.attributes();
                currentEntry->setAlgorithm(algorithmAttributes.value(QStringLiteral("manifest:algorithm-name")).toString());
                currentEntry->setInitialisationVector(algorithmAttributes.value(QStringLiteral("manifest:initialisation-vector")).toString());
            } else if (xml.name().toString() == QLatin1String("key-derivation")) {
                if (currentEntry == nullptr) {
                    qCWarning(OkularOooDebug) << "Got key-derivation without valid file-entry at line" << xml.lineNumber();
                    continue;
                }
                QXmlStreamAttributes kdfAttributes = xml.attributes();
                currentEntry->setKeyDerivationName(kdfAttributes.value(QStringLiteral("manifest:key-derivation-name")).toString());
                currentEntry->setIterationCount(kdfAttributes.value(QStringLiteral("manifest:iteration-count")).toString());
                currentEntry->setSalt(kdfAttributes.value(QStringLiteral("manifest:salt")).toString());
            } else {
                // handle other StartDocument types here
                qCWarning(OkularOooDebug) << "Unexpected start document type: " << xml.name().toString();
            }
        } else if (xml.tokenType() == QXmlStreamReader::EndElement) {
            if (xml.name().toString() == QLatin1String("manifest")) {
                continue;
            } else if (xml.name().toString() == QLatin1String("file-entry")) {
                if (currentEntry == nullptr) {
                    qCWarning(OkularOooDebug) << "Got EndElement for file-entry without valid StartElement at line" << xml.lineNumber();
                    continue;
                }
                // we're finished processing that file entry
                if (mEntries.contains(currentEntry->fileName())) {
                    qCWarning(OkularOooDebug) << "Can't insert entry because of duplicate name:" << currentEntry->fileName();
                    delete currentEntry;
                } else {
                    mEntries.insert(currentEntry->fileName(), currentEntry);
                }
                currentEntry = nullptr;
            }
        }
    }
    if (xml.hasError()) {
        qCWarning(OkularOooDebug) << "error: " << xml.errorString() << xml.lineNumber() << xml.columnNumber();
    }
}

Manifest::~Manifest()
{
    qDeleteAll(mEntries);
}

ManifestEntry *Manifest::entryByName(const QString &filename)
{
    return mEntries.value(filename, 0);
}

bool Manifest::testIfEncrypted(const QString &filename)
{
    ManifestEntry *entry = entryByName(filename);

    if (entry) {
        return (entry->salt().length() > 0);
    }

    return false;
}

void Manifest::checkPassword(ManifestEntry *entry, const QByteArray &fileData, QByteArray *decryptedData)
{
#ifdef QCA2
    QCA::SymmetricKey key = QCA::PBKDF2(QStringLiteral("sha1"))
                                .makeKey(QCA::Hash(QStringLiteral("sha1")).hash(m_password.toLocal8Bit()),
                                         QCA::InitializationVector(entry->salt()),
                                         16, // 128 bit key
                                         entry->iterationCount());

    QCA::Cipher decoder(QStringLiteral("blowfish"), QCA::Cipher::CFB, QCA::Cipher::DefaultPadding, QCA::Decode, key, QCA::InitializationVector(entry->initialisationVector()));
    *decryptedData = decoder.update(QCA::MemoryRegion(fileData)).toByteArray();
    *decryptedData += decoder.final().toByteArray();

    QByteArray csum;
    if (entry->checksumType() == QLatin1String("SHA1/1K")) {
        csum = QCA::Hash(QStringLiteral("sha1")).hash(decryptedData->left(1024)).toByteArray();
    } else if (entry->checksumType() == QLatin1String("SHA1")) {
        csum = QCA::Hash(QStringLiteral("sha1")).hash(*decryptedData).toByteArray();
    } else {
        qCDebug(OkularOooDebug) << "unknown checksum type: " << entry->checksumType();
        // we can only assume it will be OK.
        m_haveGoodPassword = true;
        return;
    }

    if (entry->checksum() == csum) {
        m_haveGoodPassword = true;
    } else {
        m_haveGoodPassword = false;
    }
#else
    m_haveGoodPassword = false;
    Q_UNUSED(entry);
    Q_UNUSED(fileData);
    Q_UNUSED(decryptedData);
#endif
}

QByteArray Manifest::decryptFile(const QString &filename, const QByteArray &fileData)
{
#ifdef QCA2
    ManifestEntry *entry = entryByName(filename);

    if (!QCA::isSupported("sha1")) {
        KMessageBox::error(nullptr, i18n("This document is encrypted, and crypto support is compiled in, but a hashing plugin could not be located"));
        // in the hope that it wasn't really encrypted...
        return QByteArray(fileData);
    }

    if (!QCA::isSupported("pbkdf2(sha1)")) {
        KMessageBox::error(nullptr, i18n("This document is encrypted, and crypto support is compiled in, but a key derivation plugin could not be located"));
        // in the hope that it wasn't really encrypted...
        return QByteArray(fileData);
    }

    if (!QCA::isSupported("blowfish-cfb")) {
        KMessageBox::error(nullptr, i18n("This document is encrypted, and crypto support is compiled in, but a cipher plugin could not be located"));
        // in the hope that it wasn't really encrypted...
        return QByteArray(fileData);
    }

    QByteArray decryptedData;
    checkPassword(entry, fileData, &decryptedData);

    if (!m_haveGoodPassword) {
        return QByteArray();
    }

    QIODevice *decompresserDevice = new KCompressionDevice(new QBuffer(&decryptedData, nullptr), true, KCompressionDevice::GZip);
    if (!decompresserDevice) {
        qCDebug(OkularOooDebug) << "Couldn't create decompressor";
        // hopefully it isn't compressed then!
        return QByteArray(fileData);
    }

    static_cast<KFilterDev *>(decompresserDevice)->setSkipHeaders();

    decompresserDevice->open(QIODevice::ReadOnly);

    return decompresserDevice->readAll();

#else
    // TODO: This should have a proper parent
    KMessageBox::error(0, i18n("This document is encrypted, but Okular was compiled without crypto support. This document will probably not open."));
    // this is equivalent to what happened before all this Manifest stuff :-)
    return QByteArray(fileData);
    Q_UNUSED(filename);
#endif
}
