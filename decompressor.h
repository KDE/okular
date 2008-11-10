/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef MOBI_DECOMPRESSOR_H
#define MOBI_DECOMPRESSOR_H

#include <QtCore/QByteArray>
namespace Mobipocket {

class PDB;

class Decompressor {
public:
    Decompressor(const PDB& p) : pdb(p), valid(true) {}
    virtual QByteArray decompress(const QByteArray& data) = 0; 
    virtual ~Decompressor() {}
    bool isValid() const { return valid; }

    static Decompressor* create(quint8 type, const PDB& pdb);
protected:
    const PDB& pdb;
    bool valid;
};

quint32 readBELong(const QByteArray& data, int offset);
}
#endif
