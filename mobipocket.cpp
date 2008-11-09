/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   RLE decompressor based on FBReader                                    *
 *   Copyright (C) 2004-2008 Geometer Plus <contact@geometerplus.com>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <mobipocket.h>
#include <QtCore/QIODevice>
#include <QtCore/QtEndian>
#include <QtCore/QBuffer>
#include <kdebug.h>

static unsigned char TOKEN_CODE[256] = {
	0, 1, 1, 1,		1, 1, 1, 1,		1, 0, 0, 0,		0, 0, 0, 0,
	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,
	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,
	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,
	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,
	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,
	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,
	0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,		0, 0, 0, 0,
	3, 3, 3, 3,		3, 3, 3, 3,		3, 3, 3, 3,		3, 3, 3, 3,
	3, 3, 3, 3,		3, 3, 3, 3,		3, 3, 3, 3,		3, 3, 3, 3,
	3, 3, 3, 3,		3, 3, 3, 3,		3, 3, 3, 3,		3, 3, 3, 3,
	3, 3, 3, 3,		3, 3, 3, 3,		3, 3, 3, 3,		3, 3, 3, 3,
	2, 2, 2, 2,		2, 2, 2, 2,		2, 2, 2, 2,		2, 2, 2, 2,
	2, 2, 2, 2,		2, 2, 2, 2,		2, 2, 2, 2,		2, 2, 2, 2,
	2, 2, 2, 2,		2, 2, 2, 2,		2, 2, 2, 2,		2, 2, 2, 2,
	2, 2, 2, 2,		2, 2, 2, 2,		2, 2, 2, 2,		2, 2, 2, 2,
};

namespace Mobipocket {

class NOOPDecompressor : public Decompressor
{
public:
    QByteArray decompress(const QByteArray& data) { return data; }
};


class RLEDecompressor : public Decompressor
{
public:
    QByteArray decompress(const QByteArray& data);
};

QByteArray RLEDecompressor::decompress(const QByteArray& data)
{
        QByteArray ret;
        ret.reserve(8192);

		unsigned char token;
		unsigned short copyLength, N, shift;
		unsigned short shifted;
        int i=0;
        int maxIndex=data.size()-1;

		while (i<data.size()) {
			token = data.at(i++);
			switch (TOKEN_CODE[token]) {
				case 0:
				        ret.append(token);
					break;
				case 1:
					if ((i + token > maxIndex) ) {
						goto endOfLoop;
					}
					ret.append(data.mid(i,token));
					i+=token;
					break;
				case 2:
				        ret.append(' ');
				        ret.append(token ^ 0x80);
					break;
				case 3:
					if (i + 1 > maxIndex) {
						goto endOfLoop;
					}
//					N = (token << 8) + data.at(i++);
                                        N = token;
                                        N<<=8;
                                        N+=(unsigned char)data.at(i++);
					copyLength = (N & 7) + 3;
					shift = (N & 0x3fff) / 8;
					shifted = ret.size()-shift;
					if (shifted>(ret.size()-1)) goto endOfLoop;
					for (int i=0;i<copyLength;i++) ret.append(ret.at(shifted+i));
					break;
			}
		}
endOfLoop:
    return ret;

}



/////////////////////////////////////////////


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
    DocumentPrivate(QIODevice* d) : pdb(d), valid(true) {}
    PDB pdb;
    Decompressor* dec;
    quint16 ntextrecords;
    bool valid;
    
    void init() {
        valid=pdb.isValid();
        if (!valid) return;
        QByteArray mhead=pdb.getRecord(0);
        if (mhead[0]!=(char)0) {}
        
        switch (mhead[1]) {
            case 1 : dec = new NOOPDecompressor(); break;
            case 2 : dec = new RLEDecompressor(); break;
            default : dec=0; {}
        }
        ntextrecords=(unsigned char)mhead[8];
        ntextrecords<<=8;
        ntextrecords+=(unsigned char)mhead[9];
    }
};

Document::Document(QIODevice* dev) : d(new DocumentPrivate(dev))
{
    d->init();
}

QString Document::text() const 
{
    QByteArray whole;
    for (int i=1;i<d->ntextrecords;i++) 
        whole+=d->dec->decompress(d->pdb.getRecord(i));
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
    QByteArray rec=d->pdb.getRecord(d->ntextrecords+i);
    kDebug() << "Want IMG " << i;
    kDebug() << rec[0] << rec[1] << rec[2] << rec[3];
    //FIXME: check if i is in range
    return QImage::fromData(rec);
}
}
