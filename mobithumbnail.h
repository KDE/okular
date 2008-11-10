/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef MOBITHUMBNAIL_H
#define MOBITHUMBNAIL_H

#include <kio/thumbcreator.h>

class MobiThumbnail : public ThumbCreator
{
public:
    MobiThumbnail() {}
    virtual bool create(const QString &path, int, int, QImage &img);
    virtual Flags flags() const;
};

#endif
