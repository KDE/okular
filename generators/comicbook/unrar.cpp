/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "unrar.h"

#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QRegExp>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kpassworddialog.h>
#include <kstandarddirs.h>
#include <ktempdir.h>
#if !defined(Q_OS_WIN)
#include <kptyprocess.h>
#include <kptydevice.h>
#endif

#include "unrarflavours.h"

struct UnrarHelper
{
    UnrarHelper();
    ~UnrarHelper();

    UnrarFlavour *kind;
    QString unrarPath;
};

K_GLOBAL_STATIC( UnrarHelper, helper )

static UnrarFlavour* detectUnrar( const QString &unrarPath )
{
    UnrarFlavour* kind = 0;
    QProcess proc;
    proc.start( unrarPath, QStringList() << "--version" );
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
    return kind;
}

UnrarHelper::UnrarHelper()
   : kind( 0 )
{
    QString path = KStandardDirs::findExe( "unrar-nonfree" );
    if ( path.isEmpty() )
        path = KStandardDirs::findExe( "unrar" );

    if ( !path.isEmpty() )
        kind = detectUnrar( path );

    if ( !kind )
    {
        // no luck, print that
        kDebug() << "No unrar detected.";
    }
    else
    {
        unrarPath = path;
        kDebug() << "detected:" << path << "(" << kind->name() << ")";
    }
}

UnrarHelper::~UnrarHelper()
{
    delete kind;
}


Unrar::Unrar()
    : QObject( 0 ), mLoop( 0 ), mTempDir( 0 )
{
}

Unrar::~Unrar()
{
    delete mTempDir;
}

bool Unrar::open( const QString &fileName )
{
    if ( !isSuitableVersionAvailable() )
        return false;

    delete mTempDir;
    mTempDir = new KTempDir();

    mFileName = fileName;

    /**
     * Extract the archive to a temporary directory
     */
    mStdOutData.clear();
    mStdErrData.clear();

    int ret = startSyncProcess( QStringList() << "e" << mFileName << mTempDir->name() );
    bool ok = ret == 0;

    return ok;
}

QStringList Unrar::list()
{
    mStdOutData.clear();
    mStdErrData.clear();

    if ( !isSuitableVersionAvailable() )
        return QStringList();

    startSyncProcess( QStringList() << "lb" << mFileName );

    return helper->kind->processListing( QString::fromLocal8Bit( mStdOutData ).split( "\n", QString::SkipEmptyParts ) );
}

QByteArray Unrar::contentOf( const QString &fileName ) const
{
    if ( !isSuitableVersionAvailable() )
        return QByteArray();

    QFile file( mTempDir->name() + fileName );
    if ( !file.open( QIODevice::ReadOnly ) )
        return QByteArray();

    return file.readAll();
}

bool Unrar::isAvailable()
{
    return helper->kind;
}

bool Unrar::isSuitableVersionAvailable()
{
    if ( !isAvailable() )
        return false;

    return dynamic_cast< NonFreeUnrarFlavour * >( helper->kind );
}

void Unrar::readFromStdout()
{
    if ( !mProcess )
        return;

    mStdOutData += mProcess->readAllStandardOutput();
}

void Unrar::readFromStderr()
{
    if ( !mProcess )
        return;

    mStdErrData += mProcess->readAllStandardError();
    if ( !mStdErrData.isEmpty() )
    {
        mProcess->kill();
        return;
    }
}

void Unrar::finished( int exitCode, QProcess::ExitStatus exitStatus )
{
    Q_UNUSED( exitCode )
    if ( mLoop )
    {
        mLoop->exit( exitStatus == QProcess::CrashExit ? 1 : 0 );
    }
}

int Unrar::startSyncProcess( const QStringList &args )
{
    int ret = 0;

#if defined(Q_OS_WIN)
    mProcess = new QProcess( this );
#else
    mProcess = new KPtyProcess( this );
    mProcess->setOutputChannelMode( KProcess::SeparateChannels );
#endif

    connect( mProcess, SIGNAL( readyReadStandardOutput() ), SLOT( readFromStdout() ) );
    connect( mProcess, SIGNAL( readyReadStandardError() ), SLOT( readFromStderr() ) );
    connect( mProcess, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finished( int, QProcess::ExitStatus ) ) );

#if defined(Q_OS_WIN)
    mProcess->start( helper->unrarPath, args, QIODevice::ReadWrite | QIODevice::Unbuffered );
    ret = mProcess->waitForFinished( -1 ) ? 0 : 1;
#else
    mProcess->setProgram( helper->unrarPath, args );
    mProcess->setNextOpenMode( QIODevice::ReadWrite | QIODevice::Unbuffered );
    mProcess->start();
    QEventLoop loop;
    mLoop = &loop;
    ret = loop.exec( QEventLoop::WaitForMoreEvents );
    mLoop = 0;
#endif

    delete mProcess;
    mProcess = 0;

    return ret;
}

void Unrar::writeToProcess( const QByteArray &data )
{
    if ( !mProcess || data.isNull() )
        return;

#if defined(Q_OS_WIN)
    mProcess->write( data );
#else
    mProcess->pty()->write( data );
#endif
}

#include "unrar.moc"
