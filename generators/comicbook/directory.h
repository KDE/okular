/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2011 by David Palacio <dpalacio@orbitalibre.org>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <QStringList>

class QIODevice;

class Directory
{
public:
    /**
     * Creates a new directory object.
     */
    Directory();

    /**
     * Destroys the directory object.
     */
    ~Directory();

    /**
     * Opens given directory.
     */
    bool open(const QString &dirName);

    /**
     * Returns the list of files from the directory.
     */
    QStringList list() const;

    /**
     * Returns a new device for reading the file with the given path.
     */
    QIODevice *createDevice(const QString &path) const;

private:
    /**
     * Iterates over a directory and returns a file list.
     */
    QStringList recurseDir(const QString &dir, int curDepth) const;

    static const int staticMaxDepth = 1;
    QString mDir;
};

#endif
