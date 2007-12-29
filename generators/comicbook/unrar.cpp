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
#include <QtCore/QRegExp>

#include <kdebug.h>
#include <kglobal.h>
#include <ktempdir.h>

#include "unrarflavours.h"

struct UnrarHelper
{
    UnrarHelper();
    ~UnrarHelper();

    UnrarFlavour *kind;
};

K_GLOBAL_STATIC( UnrarHelper, helper )

UnrarHelper::UnrarHelper()
   : kind( 0 )
{
    QProcess proc;
    proc.start( "unrar", QStringList() << "--version" );
    bool ok = proc.waitForFinished( -1 );
    Q_UNUSED( ok )
    const QStringList lines = QString::fromLocal8Bit( proc.readAllStandardOutput() ).split( "\n", QString::SkipEmptyParts );
    if ( !lines.isEmpty() )
    {
        if ( lines.first().startsWith( "UNRAR " ) )
            kind = new NonFreeUnrarFlavour();
        else if ( lines.first().startsWith( "unrar " ) )
            kind = new FreeUnrarFlavour();
    }

    if ( !kind )
    {
        // no luck so far, assume unrar-nonfree
        kind = new NonFreeUnrarFlavour();
    }
    kDebug() << "detected:" << kind->name();
}

UnrarHelper::~UnrarHelper()
{
    delete kind;
}


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

    return helper->kind->processListing( QString::fromLocal8Bit( mStdOutData ).split( "\n", QString::SkipEmptyParts ) );
}

QByteArray Unrar::contentOf( const QString &fileName ) const
{
    QFile file( mTempDir->name() + fileName );
    if ( !file.open( QIODevice::ReadOnly ) )
        return QByteArray();

    return file.readAll();
}

bool Unrar::isAvailable()
{
    return dynamic_cast< NonFreeUnrarFlavour * >( helper->kind );
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
