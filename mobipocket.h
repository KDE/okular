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

/** 
Minimalistic stream abstraction. It is supposed to allow mobipocket document classes to be
used with both QIODevice (for Okular generator) and InputStream for Strigi analyzer.
*/
class Stream {
public:
    virtual int read(char* buf, int size)=0;
    virtual bool seek(int pos)=0;

    QByteArray readAll();
    QByteArray read(int len);
    virtual ~Stream() {}
};

struct PDBPrivate;
class PDB {
public:
    PDB(Stream* s);
    QString fileType() const;
    int recordCount() const;
    QByteArray getRecord(int i) const;
    bool isValid() const;
private:
    PDBPrivate* const d;
};

struct DocumentPrivate;
class Document {
public:
    enum MetaKey { Title, Author, Copyright, Description, Subject };
    Document(Stream* s);
    QMap<MetaKey,QString> metadata() const;
    QString text(int size=-1) const;
    int imageCount() const;
    QImage getImage(int i) const;
    QImage thumbnail() const;
    bool isValid() const;
    
    // if true then it is impossible to get text of book. Images should still be readable
    bool hasDRM() const;
private:
    DocumentPrivate* const d;
};
}
#endif
