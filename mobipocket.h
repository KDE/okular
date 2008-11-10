/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef MOBIPOCKET_H
#define MOBIPOCKET_H

#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtGui/QImage>

class QIODevice;

namespace Mobipocket {

struct PDBPrivate;

class PDB {
public:
    PDB(QIODevice* dev);
    QString fileType() const;
    int recordCount() const;
    QByteArray getRecord(int i) const;
    QString name() const;
    bool isValid() const;
private:
    PDBPrivate* const d;
};

struct DocumentPrivate;
class Document {
public:
    enum MetaKey { Title, Author, Copyright, Description, Subject };
    Document(QIODevice* dev);
    QMap<MetaKey,QString> metadata() const;
    QString text() const; 
    int imageCount() const;
    QImage getImage(int i) const;
    bool isValid() const;

    // if true then isValid() will return false
    bool hasDRM() const;
private:
    DocumentPrivate* const d;
};
}
#endif
