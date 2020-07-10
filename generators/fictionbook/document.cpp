/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "document.h"

#include <QFile>

#include <KLocalizedString>
#include <kzip.h>

using namespace FictionBook;

Document::Document(const QString &fileName)
    : mFileName(fileName)
{
}

bool Document::open()
{
    QIODevice *device;

    QFile file(mFileName);
    KZip zip(mFileName);
    if (mFileName.endsWith(QLatin1String(".fb")) || mFileName.endsWith(QLatin1String(".fb2"))) {
        if (!file.open(QIODevice::ReadOnly)) {
            setError(i18n("Unable to open document: %1", file.errorString()));
            return false;
        }

        device = &file;
    } else {
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

        QString documentFile;
        for (int i = 0; i < entries.count(); ++i) {
            if (entries[i].endsWith(QLatin1String(".fb2"))) {
                documentFile = entries[i];
                break;
            }
        }

        if (documentFile.isEmpty()) {
            setError(i18n("No content found in the document"));
            return false;
        }

        const KArchiveFile *entry = static_cast<const KArchiveFile *>(directory->entry(documentFile));
        // FIXME delete 'deviceì somewhen
        device = entry->createDevice();
    }

    QString errorMsg;
    if (!mDocument.setContent(device, true, &errorMsg)) {
        setError(i18n("Invalid XML document: %1", errorMsg));
        return false;
    }

    return true;
}

QDomDocument Document::content() const
{
    return mDocument;
}

QString Document::lastErrorString() const
{
    return mErrorString;
}

void Document::setError(const QString &error)
{
    mErrorString = error;
}
