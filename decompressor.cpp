/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   RLE decompressor based on FBReader                                    *
 *   Copyright (C) 2004-2008 Geometer Plus <contact@geometerplus.com>      *
 *                                                                         *
 *   Huffdic decompressor based on Python code by Igor Skochinsky          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "decompressor.h"
#include "mobipocket.h"

#include <QtCore/QList>

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
    NOOPDecompressor(const PDB& p) : Decompressor(p) {}
    QByteArray decompress(const QByteArray& data) { return data; }
};


class RLEDecompressor : public Decompressor
{
public:
    RLEDecompressor(const PDB& p) : Decompressor(p) {}
    QByteArray decompress(const QByteArray& data);
};

class BitReader
{
public:
    BitReader(const QByteArray& d) : pos(0), data(d)
    {
        data.append("\000\000\000\000");    //krazy:exclude=strings
        len=data.size()*8;
    }
    
    quint32 read() {
        quint32 g=0;
        quint64 r=0;
        while (g<32) {
            r=(r << 8) | (quint8)data[(pos+g)>>3];
            g=g+8 - ((pos+g) & 7);
        }
        return (r >> (g-32));
    }
    bool eat(int n) {
        pos+=n;
        return pos <= len;
    }
    
    int left() {
        return len - pos;
    }
    
private:
    int pos;
    int len;
    QByteArray data;
};

class HuffdicDecompressor : public Decompressor
{
public:
    HuffdicDecompressor(const PDB& p);
    QByteArray decompress(const QByteArray& data);
private:
    void unpack(BitReader reader, int depth = 0);
    QList<QByteArray> dicts;
    quint32 entry_bits;
    quint32 dict1[256];
    quint32 dict2[64];
    
    QByteArray buf;
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

quint32 readBELong(const QByteArray& data, int offset)
{
    quint32 ret=0;
    for (int i=0;i<4;i++) { ret<<=8; ret+=(unsigned char)data[offset+i]; }
    return ret;
}

HuffdicDecompressor::HuffdicDecompressor(const PDB& p) : Decompressor(p)
{
    QByteArray header=p.getRecord(0);
    quint32 huff_ofs=readBELong(header,0x70);
    quint32 huff_num=readBELong(header,0x74);
    quint32 off1,off2;

    QByteArray huff1=p.getRecord(huff_ofs);
    if (huff1.isNull()) goto fail;
    for (unsigned int i=1;i<huff_num;i++) {
        QByteArray h=p.getRecord(huff_ofs+i);
        if (h.isNull()) goto fail;
        dicts.append(h);
    }

    off1=readBELong(huff1,16);
    off2=readBELong(huff1,20);

    if (!huff1.startsWith("HUFF")) goto fail;           //krazy:exclude=strings
    if (!dicts[0].startsWith("CDIC")) goto fail;        //krazy:exclude=strings

    entry_bits=readBELong(dicts[0],12);

    memcpy(dict1,huff1.data()+off1, 256*4);
    memcpy(dict2,huff1.data()+off2, 64*4);    
    return;
fail:
    valid=false;
}

QByteArray HuffdicDecompressor::decompress(const QByteArray& data)
{
    buf.clear();
    unpack(BitReader(data));
    return buf;
}

void HuffdicDecompressor::unpack(BitReader reader,int depth) 
{
    if (depth>32) goto fail;
    while (reader.left()) {
        quint32 dw=reader.read();
        quint32 v=dict1[dw>>24];
        quint8 codelen = v & 0x1F;
        if (!codelen) goto fail;
        quint32 code = dw >> (32 - codelen);
        quint32 r=(v >> 8);
        if (!( v & 0x80))  {
            while (code < dict2[(codelen-1)*2]) {
                codelen++;
                code = dw >> (32 - codelen);
            }
            r = dict2[(codelen-1)*2+1];
        }
        r-=code;
        if (!codelen) goto fail;
        if (!reader.eat(codelen)) return;
        quint32 dict_no = r >> entry_bits;
        quint32 off1 = 16 + (r - (dict_no << entry_bits))*2;
        QByteArray dict=dicts[dict_no];
        quint32 off2 = 16 + (unsigned char)dict[off1]*256 + (unsigned char)dict[off1+1];
        quint32 blen = (unsigned char)dict[off2]*256 + (unsigned char)dict[off2+1];
        QByteArray slice=dict.mid(off2+2,(blen & 0x7fff));
        if (blen & 0x8000) buf+=slice;
        else unpack(BitReader(slice),depth+1);
    }
    return;
fail:
    valid=false;
}

Decompressor* Decompressor::create(quint8 type, const PDB& pdb) 
{
        switch (type) {
            case 1 : return new NOOPDecompressor(pdb); 
            case 2 : return new RLEDecompressor(pdb); 
            case 'H' : return  new HuffdicDecompressor(pdb);
            default : return 0;
        }

}
}
