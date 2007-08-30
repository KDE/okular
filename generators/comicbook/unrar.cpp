/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "unrar.h"

#include <QtCore/QFile>
#include <QtCore/QProcess>

#include <ktempdir.h>

Unrar::Unrar()
    : QObject( 0 ), mTempDir( 0 )
{
}

Unrar::~Unrar()
{
    delete mTempDir;
}

bool Unrar::open( const QString &fileName )
{
    delete mTempDir;
    mTempDir = new KTempDir();

    mFileName = fileName;

    /**
     * Extract the archive to a temporary directory
     */
    mStdOutData.clear();
    mStdErrData.clear();

    mProcess = new QProcess( this );

    connect( mProcess, SIGNAL( readyReadStandardOutput() ), SLOT( readFromStdout() ) );
    connect( mProcess, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );

    mProcess->start( "unrar", QStringList() << "e" << mFileName << mTempDir->name(), QIODevice::ReadOnly );
    bool ok = mProcess->waitForFinished( -1 );

    delete mProcess;
    mProcess = 0;

    return ok;
}

QStringList Unrar::list()
{
    mStdOutData.clear();
    mStdErrData.clear();

    mProcess = new QProcess( this );

    connect( mProcess, SIGNAL( readyReadStandardOutput() ), SLOT( readFromStdout() ) );
    connect( mProcess, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );

    mProcess->start( "unrar", QStringList() << "lb" << mFileName, QIODevice::ReadOnly );
    mProcess->waitForFinished( -1 );

    delete mProcess;
    mProcess = 0;

    return QString::fromLocal8Bit( mStdOutData ).split( "\n", QString::SkipEmptyParts );
}

QByteArray Unrar::contentOf( const QString &fileName ) const
{
    QFile file( mTempDir->name() + fileName );
    if ( !file.open( QIODevice::ReadOnly ) )
        return QByteArray();

    return file.readAll();
}

void Unrar::readFromStdout()
{
    mStdOutData += mProcess->readAllStandardOutput();
}

void Unrar::readFromStderr()
{
    mStdErrData += mProcess->readAllStandardError();
}

#include "unrar.moc"
