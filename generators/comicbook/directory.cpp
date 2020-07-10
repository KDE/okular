/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2011 by David Palacio <dpalacio@orbitalibre.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "directory.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include <memory>

Directory::Directory()
{
}

Directory::~Directory()
{
}

bool Directory::open(const QString &dirName)
{
    mDir = dirName;
    QFileInfo dirTest(dirName);
    return dirTest.isDir() && dirTest.isReadable();
}

QStringList Directory::recurseDir(const QString &dirPath, int curDepth) const
{
    QDir dir(dirPath);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    QStringList fileList;
    QDirIterator it(dir);
    QFileInfo info;
    while (it.hasNext()) {
        it.next();
        info = it.fileInfo();
        if (info.isDir() && curDepth < staticMaxDepth) {
            fileList.append(recurseDir(info.filePath(), curDepth + 1));
        } else if (info.isFile()) {
            fileList.append(info.filePath());
        }
    }
    return fileList;
}

QStringList Directory::list() const
{
    return recurseDir(mDir, 0);
}

QIODevice *Directory::createDevice(const QString &path) const
{
    std::unique_ptr<QFile> file(new QFile(path));
    if (!file->open(QIODevice::ReadOnly))
        return nullptr;

    return file.release();
}
