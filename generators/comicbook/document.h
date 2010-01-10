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

#include <QtCore/QStringList>

class KArchiveDirectory;
class KArchive;
class QImage;
class QSize;
class Unrar;

namespace ComicBook {

class Document
{
    public:
        Document();
        ~Document();

        bool open( const QString &fileName );
        void close();

        int pages() const;
        QStringList pageTitles() const;

        QImage pageImage( int page ) const;
        QSize pageSize( int page ) const;

        QString lastErrorString() const;

    private:
        void extractImageFiles( const QStringList& );
        bool processArchive();

        QStringList mPageMap;
        Unrar *mUnrar;
        KArchive *mArchive;
        KArchiveDirectory *mArchiveDir;
        QString mLastErrorString;
};

}

#endif
