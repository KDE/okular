/***************************************************************************
 *   Copyright (C) 2005   by Piotr Szymanski <niedakh@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QTextStream>

#include "generator.h"

QTextStream& operator<< (QTextStream& str, const PixmapRequest *req)
{
    QString s;
    s += QString(req->async ? "As" : "S") + QString("ync PixmapRequest (id: %1) (%2x%3) ").arg(req->id,req->width,req->height);
    s += QString("prio: %1, pageNo: %2) ").arg(req->priority,req->pageNumber);
    return (str << s);
}

#include "generator.moc"
