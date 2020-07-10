/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef FICTIONBOOK_DOCUMENT_H
#define FICTIONBOOK_DOCUMENT_H

#include <QByteArray>
#include <QDomDocument>
#include <QMap>
#include <QString>

namespace FictionBook
{
class Document
{
public:
    explicit Document(const QString &fileName);

    bool open();

    QDomDocument content() const;

    QString lastErrorString() const;

private:
    void setError(const QString &);

    QString mFileName;
    QDomDocument mDocument;
    QString mErrorString;
};

}

#endif
