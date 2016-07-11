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
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QGlobalStatic>
#include <QTemporaryDir>

#include <QtCore/qloggingcategory.h>
#if !defined(Q_OS_WIN)
#include <KPty/kptyprocess.h>
#include <KPty/kptydevice.h>
#endif

#include "unrarflavours.h"
#include "debug_comicbook.h"

#include <memory>
#include <QStandardPaths>

struct UnrarHelper
{
    UnrarHelper();
    ~UnrarHelper();

    UnrarFlavour *kind;
    QString unrarPath;
};

Q_GLOBAL_STATIC( UnrarHelper, helper )

static UnrarFlavour* detectUnrar( const QString &unrarPath, const QString &versionCommand )
{
    UnrarFlavour* kind = 0;
    QProcess proc;
    proc.start( unrarPath, QStringList() << versionCommand );
    bool ok = proc.waitForFinished( -1 );
    Q_UNUSED( ok )
    const QStringList lines = QString::fromLocal8Bit( proc.readAllStandardOutput() ).split( QLatin1Char('\n'), QString::SkipEmptyParts );
    if ( !lines.isEmpty() )
    {
        if ( lines.first().startsWith( QLatin1String("UNRAR ") ) )
            kind = new NonFreeUnrarFlavour();
        else if ( lines.first().startsWith( QLatin1String("RAR ") ) )
            kind = new NonFreeUnrarFlavour();
        else if ( lines.first().startsWith( QLatin1String("unrar ") ) )
            kind = new FreeUnrarFlavour();
    }
    return kind;
}

UnrarHelper::UnrarHelper()
   : kind( 0 )
{
    QString path = QStandardPaths::findExecutable( QStringLiteral("unrar-nonfree") );
    if ( path.isEmpty() )
        path = QStandardPaths::findExecutable( QStringLiteral("unrar") );
    if ( path.isEmpty() )
        path = QStandardPaths::findExecutable( QStringLiteral("rar") );

    if ( !path.isEmpty() )
        kind = detectUnrar( path, QStringLiteral("--version") );

    if ( !kind )
        kind = detectUnrar( path, QStringLiteral("-v") );

    if ( !kind )
    {
        // no luck, print that
        qWarning() << "No unrar detected.";
    }
    else
    {
        unrarPath = path;
        qCDebug(OkularComicbookDebug) << "detected:" << path << "(" << kind->name() << ")";
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
    mTempDir = new QTemporaryDir();

    mFileName = fileName;

    /**
     * Extract the archive to a temporary directory
     */
    mStdOutData.clear();
    mStdErrData.clear();

    int ret = startSyncProcess( QStringList() << QStringLiteral("e") << mFileName << mTempDir->path() +  QLatin1Char('/') );
    bool ok = ret == 0;

    return ok;
}

QStringList Unrar::list()
{
    mStdOutData.clear();
    mStdErrData.clear();

    if ( !isSuitableVersionAvailable() )
        return QStringList();

    startSyncProcess( QStringList() << QStringLiteral("lb") << mFileName );

    const QStringList listFiles = helper->kind->processListing( QString::fromLocal8Bit( mStdOutData ).split( QLatin1Char('\n'), QString::SkipEmptyParts ) );
    QStringList newList;
    Q_FOREACH ( const QString &f, listFiles ) {
        // Extract all the files to mTempDir regardless of their path inside the archive
        // This will break if ever an arvhice with two files with the same name in different subfolders
        QFileInfo fi( f );
        if ( QFile::exists( mTempDir->path() + QLatin1Char('/') + fi.fileName() ) ) {
            newList.append( fi.fileName() );
        }
    }
    return newList;
}

QByteArray Unrar::contentOf( const QString &fileName ) const
{
    if ( !isSuitableVersionAvailable() )
        return QByteArray();

    QFile file( mTempDir->path() + QLatin1Char('/') + fileName );
    if ( !file.open( QIODevice::ReadOnly ) )
        return QByteArray();

    return file.readAll();
}

QIODevice* Unrar::createDevice( const QString &fileName ) const
{
    if ( !isSuitableVersionAvailable() )
        return 0;

    std::unique_ptr< QFile> file( new QFile( mTempDir->path() + QLatin1Char('/') + fileName ) );
    if ( !file->open( QIODevice::ReadOnly ) )
        return 0;

    return file.release();
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
    connect(mProcess, &QProcess::readyReadStandardOutput, this, &Unrar::readFromStdout);
    connect(mProcess, &QProcess::readyReadStandardError, this, &Unrar::readFromStderr);
    connect(mProcess, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &Unrar::finished);

#else
    mProcess = new KPtyProcess( this );
    mProcess->setOutputChannelMode( KProcess::SeparateChannels );    
    connect(mProcess, &KPtyProcess::readyReadStandardOutput, this, &Unrar::readFromStdout);
    connect(mProcess, &KPtyProcess::readyReadStandardError, this, &Unrar::readFromStderr);
    connect(mProcess, static_cast<void (KPtyProcess::*)(int, QProcess::ExitStatus)>(&KPtyProcess::finished), this, &Unrar::finished);

#endif

#if defined(Q_OS_WIN)
    mProcess->start( helper->unrarPath, args, QIODevice::ReadWrite | QIODevice::Unbuffered );
    ret = mProcess->waitForFinished( -1 ) ? 0 : 1;
#else
    mProcess->setProgram( helper->unrarPath, args );
    mProcess->setNextOpenMode( QIODevice::ReadWrite | QIODevice::Unbuffered );
    mProcess->start();
    QEventLoop loop;
    mLoop = &loop;
    ret = loop.exec( QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeUserInputEvents );
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

