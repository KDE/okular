/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef UNRAR_H
#define UNRAR_H

#include <QtCore/QObject>
#include <QtCore/QStringList>

class QProcess;
class KTempDir;

class Unrar : public QObject
{
    Q_OBJECT

    public:
        /**
         * Creates a new unrar object.
         */
        Unrar();

        /**
         * Destroys the unrar object.
         */
        ~Unrar();

        /**
         * Opens given rar archive.
         */
        bool open( const QString &fileName );

        /**
         * Returns the list of files from the archive.
         */
        QStringList list();

        /**
         * Returns the content of the file with the given name.
         */
        QByteArray contentOf( const QString &fileName ) const;

    private Q_SLOTS:
        void readFromStdout();
        void readFromStderr();

    private:
        QProcess *mProcess;
        QString mFileName;
        QByteArray mStdOutData;
        QByteArray mStdErrData;
        KTempDir *mTempDir;
};

#endif

