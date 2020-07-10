/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef COMICBOOK_DOCUMENT_H
#define COMICBOOK_DOCUMENT_H

#include <QStringList>

class KArchiveDirectory;
class KArchive;
class QImage;
class Unrar;
class Directory;

namespace Okular
{
class Page;
}

namespace ComicBook
{
class Document
{
public:
    Document();
    ~Document();

    bool open(const QString &fileName);
    void close();

    void pages(QVector<Okular::Page *> *pagesVector);
    QStringList pageTitles() const;

    QImage pageImage(int page) const;

    QString lastErrorString() const;

private:
    bool processArchive();

    QStringList mPageMap;
    Directory *mDirectory;
    Unrar *mUnrar;
    KArchive *mArchive;
    const KArchiveDirectory *mArchiveDir;
    QString mLastErrorString;
    QStringList mEntries;
};

}

#endif
