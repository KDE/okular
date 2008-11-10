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
    DocumentPrivate(QIODevice* d) : pdb(d), valid(true), firstImageRecord(0), isUtf(false) {}
    PDB pdb;
    Decompressor* dec;
    quint16 ntextrecords;
    bool valid;
    quint16 firstImageRecord;
    QMap<Document::MetaKey, QString> metadata;
    bool isUtf;
    
    void init();
    void findFirstImage();
    void parseEXTH(const QByteArray& data);
    void parseHtmlHead(const QString& data);
    QString readEXTHRecord(const QByteArray& data, quint32& offset);
    QString decodeString(const QByteArray& data) const;
    
}; 

QString DocumentPrivate::decodeString(const QByteArray& data) const
{
    return isUtf ? QString::fromUtf8(data) : QString::fromLatin1(data);
}

void DocumentPrivate::parseHtmlHead(const QString& data)
{
    static QRegExp title("<dc:title.*>(.*)</dc:title>", Qt::CaseInsensitive);
    static QRegExp author("<dc:creator.*>(.*)</dc:creator>", Qt::CaseInsensitive);
    static QRegExp copyright("<dc:rights.*>(.*)</dc:rights>", Qt::CaseInsensitive);
    static QRegExp subject("<dc:subject.*>(.*)</dc:subject>", Qt::CaseInsensitive);
    static QRegExp description("<dc:description.*>(.*)</dc:description>", Qt::CaseInsensitive);
    title.setMinimal(true);
    author.setMinimal(true);
    copyright.setMinimal(true);
    subject.setMinimal(true);
    description.setMinimal(true);
    
    // title could have been already taken from MOBI record
    if (!metadata.contains(Document::Title) && title.indexIn(data)!=-1) metadata[Document::Title]=title.capturedTexts()[1];
    if (author.indexIn(data)!=-1) metadata[Document::Author]=author.capturedTexts()[1];
    if (copyright.indexIn(data)!=-1) metadata[Document::Copyright]=copyright.capturedTexts()[1];
    if (subject.indexIn(data)!=-1) metadata[Document::Subject]=subject.capturedTexts()[1];
    if (description.indexIn(data)!=-1) metadata[Document::Description]=description.capturedTexts()[1];
    
}

void DocumentPrivate::init()
{
    valid=pdb.isValid();
    if (!valid) return;
    QByteArray mhead=pdb.getRecord(0);
    dec = Decompressor::create(mhead[1], pdb);
    if (!dec) {
        valid=false;
        return;
    }
    ntextrecords=(unsigned char)mhead[8];
    ntextrecords<<=8;
    ntextrecords+=(unsigned char)mhead[9];
    quint32 encoding=readBELong(mhead, 28);
    if (encoding==65001) isUtf=true;
    if (mhead.size()>176) parseEXTH(mhead);
    
    // try getting metadata from HTML if nothing or only title was recovered from MOBI and EXTH records
    if (metadata.size()<2) parseHtmlHead(decodeString(dec->decompress(pdb.getRecord(1))));
}

void DocumentPrivate::findFirstImage() {
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

QString DocumentPrivate::readEXTHRecord(const QByteArray& data, quint32& offset)
{
    quint32 len=readBELong(data,offset);
    offset+=4;
    len-=8;
    QString ret=decodeString(data.mid(offset,len));
    offset+=len;
    return ret;
}

void DocumentPrivate::parseEXTH(const QByteArray& data) 
{
    // try to get name 
    if (data.size()>=92) {
        quint32 nameoffset=readBELong(data,84);
        quint32 namelen=readBELong(data,88);
        if ( (nameoffset + namelen) < data.size() ) {
            metadata[Document::Title]=decodeString(data.mid(nameoffset, namelen));
        }
    }

    quint32 exthoffs=readBELong(data,20)+16;

    if (data.mid(exthoffs,4)!="EXTH") return;
    quint32 records=readBELong(data,exthoffs+8);
    quint32 offset=exthoffs+12;
    for (unsigned int i=0;i<records;i++) {
        quint32 type=readBELong(data,offset);
        offset+=4;
        switch (type) {
            case 100: metadata[Document::Author]=readEXTHRecord(data,offset); break;
            case 103: metadata[Document::Description]=readEXTHRecord(data,offset); break;
            case 105: metadata[Document::Subject]=readEXTHRecord(data,offset); break;
            case 109: metadata[Document::Copyright]=readEXTHRecord(data,offset); break;
            default: readEXTHRecord(data,offset);
        }
    }
            
    
}

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
    return d->decodeString(whole);
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

QMap<Document::MetaKey,QString> Document::metadata() const
{
    return d->metadata;
}

}
