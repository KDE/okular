/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// system includes
#include <QEventLoop>
#include <kjob.h>
#include <kio/job.h>
#include <kio/scheduler.h>

// local includes
#include "remotefile.h"

namespace Okular {

    RemoteFile::RemoteFile( KUrl url )
	: QIODevice( ), m_url( url ), m_offset( 0 )
    {
	KIO::TransferJob * transferJob = KIO::get( url, KIO::Reload );
	connect( transferJob, SIGNAL(totalSize(KJob*,qulonglong)), this, SLOT(slotTotalSize(KJob*,qulonglong)), Qt::QueuedConnection );
	connect( transferJob, SIGNAL(mimetype(KIO::Job*,QString)), this, SLOT(slotSetMimeType(KIO::Job*,QString)), Qt::QueuedConnection );
	waitForReadyRead( -1 );
    }

    RemoteFile::~RemoteFile()
    {
    }

    bool RemoteFile::waitForReadyRead( int msecs )
    {
	QEventLoop pause;
	if( msecs >= 0 )
	{ 
	    QTimer timer;
	    timer.setSingleShot( true );
	    timer.setInterval( msecs );
	    connect( &timer, SIGNAL(timeout()), &pause, SLOT(quit()), Qt::QueuedConnection );
	    timer.start();
	}
	connect( this, SIGNAL(readyRead()), &pause, SLOT(quit()), Qt::QueuedConnection );
	pause.exec();
	return QIODevice::waitForReadyRead( msecs );
    }

    qint64 RemoteFile::bytesAvailable() const
    {
	return QIODevice::bytesAvailable() + m_fileSize;
    }

    bool RemoteFile::seek( qint64 pos )
    {
	m_offset = pos;
	return QIODevice::seek(pos);
    }

    QString RemoteFile::mimeType() const
    {
	return m_mimeType;
    }

    bool RemoteFile::atEnd() const
    {
	if( pos() == bytesAvailable() )
	    return true;
	return QIODevice::atEnd();
    }

    void RemoteFile::slotHandleData( KIO::Job * /*job*/, const QByteArray & data )
    {
	m_buffer.append( data );
	if( ! data.size() )
	    emit readyRead();
    }

    void RemoteFile::slotTotalSize( KJob * job, qulonglong size )
    {
	if( ! job->error() )
	{
	    m_fileSize = size;
	    emit readyRead();
	}
	( ( KIO::TransferJob * )job )->putOnHold();
	KIO::Scheduler::publishSlaveOnHold();
    }

    void RemoteFile::slotSetMimeType( KIO::Job * /*job*/, const QString & type )
    {
	m_mimeType = type;
    }

    qint64 RemoteFile::readData( char * data, qint64 maxSize )
    {
	m_buffer.clear();
	KIO::TransferJob * job = KIO::get( m_url );
	job->addMetaData( QString( "cache" ), QString( "reload" ) );
	job->addMetaData( QString( "resume" ), QString::number( m_offset ) );
	job->addMetaData( QString( "resume_until" ), QString::number( ( m_offset + maxSize - 1 ) < m_fileSize ? m_offset + maxSize - 1 : m_fileSize ) );
	connect( job, SIGNAL(data(KIO::Job*,QByteArray)), this, SLOT(slotHandleData(KIO::Job*,QByteArray)), Qt::QueuedConnection );
	waitForReadyRead( -1 );
	memcpy( data, m_buffer.data(), m_buffer.size() );
	return m_buffer.size();
    }

    qint64 RemoteFile::writeData( const char * /*data*/, qint64 /*maxSize*/ )
    {
	return -1;
    }

}

#include "remotefile.moc"