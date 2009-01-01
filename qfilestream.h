/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef QFILESTREAM_H
#define QFILESTREAM_H

#include <QtCore/QFile>
#include "mobipocket.h"

namespace Mobipocket {

class QFileStream : public Stream
{
public:
    QFileStream(const QString& name) : d(name) { d.open(QIODevice::ReadOnly); } 
    int read(char* buf, int size) { return d.read(buf,size); }
    bool seek(int pos) { return d.seek(pos); }
private:
    QFile d;
};

}
#endif
