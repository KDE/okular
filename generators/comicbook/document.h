/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COMICBOOK_DOCUMENT_H
#define COMICBOOK_DOCUMENT_H

#include <QStringList>

#include <memory>

class KArchiveDirectory;
class QIODevice;
class KArchive;
class QImage;
class Unrar;
class Directory;

namespace Okular
{
class DocumentSynopsis;
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

    void pages(QList<Okular::Page *> *pagesVector);
    QStringList pageTitles() const;

    QImage pageImage(int page) const;

    QString lastErrorString() const;

    /**
     * Returns a table of contents built from the Bookmark attributes of
     * ComicInfo.xml, or nullptr if there are none. Only valid after pages().
     */
    std::unique_ptr<Okular::DocumentSynopsis> documentSynopsis() const;

private:
    bool processArchive();
    std::unique_ptr<QIODevice> createDevice(const QString &file) const;

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
