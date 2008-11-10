/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "mobipocket.h"
#include "decompressor.h"

#include <QtCore/QIODevice>
#include <QtCore/QtEndian>
#include <QtCore/QBuffer>
#include <QtGui/QImageReader>
#include <kdebug.h>

namespace Mobipocket {

struct PDBPrivate {
    QList<quint32> recordOffsets;
    QIODevice* device;
    QString fileType;
    QString name;
    quint16 nrecords;
    bool valid;
    
    void init();
};

void PDBPrivate::init() 
{
        valid=true;
        quint16 word;
        quint32 dword;
        device->seek(0);
        name=QString::fromLatin1(device->read(32));
        device->seek(0x3c);
        fileType=QString::fromLatin1(device->read(8));
        
        device->seek(0x4c);
        device->read((char*)&word,2);
        nrecords=qFromBigEndian(word);
        
        for (int i=0;i<nrecords;i++) {
            device->read((char*)&dword,4);
            recordOffsets.append(qFromBigEndian(dword)); 
            device->read((char*)&dword,4);
        }
}

PDB::PDB(QIODevice* dev) : d(new PDBPrivate)
{
    d->device=dev;
    d->init();
}

QByteArray PDB::getRecord(int i) const
{
    if (i>=d->nrecords) return QByteArray();
    quint32 offset=d->recordOffsets[i];
    bool last=(i==(d->nrecords-1));
    quint32 size=0;
    if (last) size=d->device->size()-offset;
    else size=d->recordOffsets[i+1]-offset;
    d->device->seek(offset);
    return d->device->read(size);
}

QString PDB::name() const 
{
    return d->name;
}

bool PDB::isValid() const
{
    return d->valid;
}

int PDB::recordCount() const
{
    return d->nrecords;
}

////////////////////////////////////////////
struct DocumentPrivate 
{
    DocumentPrivate(QIODevice* d) : pdb(d), valid(true), firstImageRecord(0) {}
    PDB pdb;
    Decompressor* dec;
    quint16 ntextrecords;
    bool valid;
    quint16 firstImageRecord;
    
    void init() {
        valid=pdb.isValid();
        if (!valid) return;
        QByteArray mhead=pdb.getRecord(0);
        kDebug() << "MHEAD" << (int)mhead[0];
//        if (mhead[0]!=(char)0) goto fail;

        kDebug() << "MHEAD" << (int)mhead[1];
        dec = Decompressor::create(mhead[1], pdb);
        if (!dec) goto fail;
        ntextrecords=(unsigned char)mhead[8];
        ntextrecords<<=8;
        ntextrecords+=(unsigned char)mhead[9];
        return;
    fail:
        valid=false;

    }
    void findFirstImage() {
        firstImageRecord=ntextrecords+1;
        while (firstImageRecord<pdb.recordCount()) {
            QByteArray rec=pdb.getRecord(firstImageRecord);
            if (rec.isNull()) return;
            QBuffer buf(&rec);
            buf.open(QIODevice::ReadOnly);
            QImageReader r(&buf);
            if (r.canRead()) return;
            firstImageRecord++;
        }
    }
};

Document::Document(QIODevice* dev) : d(new DocumentPrivate(dev))
{
    d->init();
}

QString Document::text() const 
{
    QByteArray whole;
    for (int i=1;i<d->ntextrecords+1;i++) { 
        whole+=d->dec->decompress(d->pdb.getRecord(i));
        if (!d->dec->isValid()) {
            d->valid=false;
            return QString::null;
        }
    }
    return QString::fromUtf8(whole);
}

int Document::imageCount() const 
{
    //FIXME: don't count FLIS and FCIS records
    return d->pdb.recordCount()-d->ntextrecords;
}

bool Document::isValid() const
{
    return d->pdb.isValid();
}

QImage Document::getImage(int i) const 
{
    if (!d->firstImageRecord) d->findFirstImage();
    QByteArray rec=d->pdb.getRecord(d->firstImageRecord+i-1);
    //FIXME: check if i is in range
    return QImage::fromData(rec);
}
}
