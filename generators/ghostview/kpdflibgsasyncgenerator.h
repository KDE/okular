/***************************************************************************
 *   Copyright (C) 2005 by Piotr Szymanski <niedakh@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _LIBGS_ASYNC_GENERATOR_
#define _LIBGS_ASYNC_GENERATOR_

#include <qpixmap.h>
#include <interpreter.h>
#include <core/generator.h>

class PixHandler : public QObject
{
    Q_OBJECT
    public slots:
        void slotPixmap(PixmapRequest *req);
};

struct PageInfo
{
    GSInterpreterLib::Position pos;
    bool sync;
    Qt::HANDLE handle;
};


#endif
