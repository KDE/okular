/***************************************************************************
 *   Copyright (C) 2008 by Jakub Stachowski <qbast@go2.pl>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "mobithumbnail.h"
#include "mobipocket.h"
#include "qfilestream.h"

#include <QtCore/QFile>

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new MobiThumbnail;
    }
}

bool MobiThumbnail::create(const QString &path, int width, int height, QImage &img)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    
    Mobipocket::QFileStream f(path);
    Mobipocket::Document doc(&f);
    if (!doc.isValid()) return false;
    img=doc.thumbnail();
    return !img.isNull();
}

ThumbCreator::Flags MobiThumbnail::flags() const
{
    return static_cast<Flags>(None);
}

