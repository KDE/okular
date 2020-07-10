/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "document.h"

#include <KLocalizedString>
#include <kzip.h>

using namespace OOO;

Document::Document(const QString &fileName)
    : mFileName(fileName)
    , mManifest(nullptr)
    , mAnyEncrypted(false)
{
}

bool Document::open(const QString &password)
{
    mContent.clear();
    mStyles.clear();

    KZip zip(mFileName);
    if (!zip.open(QIODevice::ReadOnly)) {
        setError(i18n("Document is not a valid ZIP archive"));
        return false;
    }

    const KArchiveDirectory *directory = zip.directory();
    if (!directory) {
        setError(i18n("Invalid document structure (main directory is missing)"));
        return false;
    }

    const QStringList entries = directory->entries();
    if (!entries.contains(QStringLiteral("META-INF"))) {
        setError(i18n("Invalid document structure (META-INF directory is missing)"));
        return false;
    }
    const KArchiveDirectory *metaInfDirectory = static_cast<const KArchiveDirectory *>(directory->entry(QStringLiteral("META-INF")));
    if (!(metaInfDirectory->entries().contains(QStringLiteral("manifest.xml")))) {
        setError(i18n("Invalid document structure (META-INF/manifest.xml is missing)"));
        return false;
    }

    const KArchiveFile *file = static_cast<const KArchiveFile *>(metaInfDirectory->entry(QStringLiteral("manifest.xml")));
    mManifest = new Manifest(mFileName, file->data(), password);

    // we should really get the file names from the manifest, but for now, we only care
    // if the manifest says the files are encrypted.

    if (!entries.contains(QStringLiteral("content.xml"))) {
        setError(i18n("Invalid document structure (content.xml is missing)"));
        return false;
    }

    file = static_cast<const KArchiveFile *>(directory->entry(QStringLiteral("content.xml")));
    if (mManifest->testIfEncrypted(QStringLiteral("content.xml"))) {
        mAnyEncrypted = true;
        mContent = mManifest->decryptFile(QStringLiteral("content.xml"), file->data());
    } else {
        mContent = file->data();
    }

    if (entries.contains(QStringLiteral("styles.xml"))) {
        file = static_cast<const KArchiveFile *>(directory->entry(QStringLiteral("styles.xml")));
        if (mManifest->testIfEncrypted(QStringLiteral("styles.xml"))) {
            mAnyEncrypted = true;
            mStyles = mManifest->decryptFile(QStringLiteral("styles.xml"), file->data());
        } else {
            mStyles = file->data();
        }
    }

    if (entries.contains(QStringLiteral("meta.xml"))) {
        file = static_cast<const KArchiveFile *>(directory->entry(QStringLiteral("meta.xml")));
        if (mManifest->testIfEncrypted(QStringLiteral("meta.xml"))) {
            mAnyEncrypted = true;
            mMeta = mManifest->decryptFile(QStringLiteral("meta.xml"), file->data());
        } else {
            mMeta = file->data();
        }
    }

    if (entries.contains(QStringLiteral("Pictures"))) {
        const KArchiveDirectory *imagesDirectory = static_cast<const KArchiveDirectory *>(directory->entry(QStringLiteral("Pictures")));

        const QStringList imagesEntries = imagesDirectory->entries();
        for (int i = 0; i < imagesEntries.count(); ++i) {
            file = static_cast<const KArchiveFile *>(imagesDirectory->entry(imagesEntries[i]));
            QString fullPath = QStringLiteral("Pictures/%1").arg(imagesEntries[i]);
            if (mManifest->testIfEncrypted(fullPath)) {
                mAnyEncrypted = true;
                mImages.insert(fullPath, mManifest->decryptFile(fullPath, file->data()));
            } else {
                mImages.insert(fullPath, file->data());
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

bool Document::anyFileEncrypted() const
{
    return mAnyEncrypted;
}

void Document::setError(const QString &error)
{
    mErrorString = error;
}
