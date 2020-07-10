/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OOO_DOCUMENT_H
#define OOO_DOCUMENT_H

#include <QByteArray>
#include <QMap>
#include <QString>

#include "manifest.h"

namespace OOO
{
class Document
{
public:
    explicit Document(const QString &fileName);
    ~Document();

    Document(const Document &) = delete;
    Document &operator=(const Document &) = delete;

    bool open(const QString &password);

    QString lastErrorString() const;

    QByteArray content() const;
    QByteArray meta() const;
    QByteArray styles() const;
    QMap<QString, QByteArray> images() const;
    bool anyFileEncrypted() const;

private:
    void setError(const QString &);

    QString mFileName;
    QByteArray mContent;
    QByteArray mMeta;
    QByteArray mStyles;
    QMap<QString, QByteArray> mImages;
    Manifest *mManifest;
    QString mErrorString;
    bool mAnyEncrypted;
};

}

#endif
