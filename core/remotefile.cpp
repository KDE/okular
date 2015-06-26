/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// system includes
#include <kio/jobclasses.h>
#include <kio/filejob.h>
#include <QIODevice>
#include <QEventLoop>
#include <QTimer>

// local includes
#include "remotefile.h"

namespace Okular {

    RemoteFile::RemoteFile( KIO::FileJob * job ) : QIODevice( ), m_fileJob( job )
    {
	connect( m_fileJob, SIGNAL(open(KIO::Job*)), this, SIGNAL(readyRead()) );
	connect( m_fileJob, SIGNAL(data(KIO::Job*,const QByteArray&)), this, SLOT(slotHandleData(KIO::Job*,const QByteArray&)) );
	connect( m_fileJob, SIGNAL(mimetype(KIO::Job*,const QString&)), this, SLOT(slotSetMimeType(KIO::Job*,const QString&)) );
	connect( m_fileJob, SIGNAL(close(KIO::Job*)), this, SIGNAL(readyRead()) );
	waitForReadyRead( -1 );
    }

    RemoteFile::~RemoteFile()
    {
	m_fileJob->close();
	waitForReadyRead( -1 );
	if( isOpen() )
	    close();
	delete m_fileJob;
    }

    bool RemoteFile::waitForReadyRead( int msecs )
    {
	QEventLoop pause;
	if( msecs >= 0 )
	{ 
	    QTimer timer;
	    timer.setSingleShot( true );
	    timer.setInterval( msecs );
	    connect( &timer, SIGNAL(timeout()), &pause, SLOT(quit()) );
	    timer.start();
	}
	connect( this, SIGNAL(readyRead()), &pause, SLOT(quit()) );
	pause.exec();
	return QIODevice::waitForReadyRead( msecs );
    }

    qint64 RemoteFile::bytesAvailable() const
    {
	return QIODevice::bytesAvailable() + m_fileJob->size();
    }

    bool RemoteFile::seek( qint64 pos )
    {
	m_fileJob->seek( pos );
	QEventLoop pause;
	qRegisterMetaType<KIO::filesize_t>( "KIO::filesize_t" );
	connect( m_fileJob, SIGNAL(position(KIO::Job*,KIO::filesize_t)), &pause, SLOT(quit()) );
	pause.exec();
	return QIODevice::seek(pos);
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
	emit readyRead();
    }

    void RemoteFile::slotSetMimeType( KIO::Job * /*job*/, const QString & type )
    {
	m_mimeType = type;
    }

    qint64 RemoteFile::readData( char * data, qint64 maxSize )
    {
	m_buffer.clear();
	m_fileJob->read( maxSize );
	waitForReadyRead( -1 );
	for( qint64 i = 0; i < m_buffer.size(); i ++ )
	{
	    *( data + i ) = m_buffer.at( i );
	}
	return m_buffer.size();
    }

    qint64 RemoteFile::writeData( const char * /*data*/, qint64 /*maxSize*/ )
    {
	return -1;
    }

}

#include "remotefile.moc"