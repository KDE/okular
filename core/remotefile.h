/***************************************************************************
 *   Copyright (C) 2015 by Saheb Preet Singh <saheb.preet@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef OKULAR_REMOTEFILE_H
#define OKULAR_REMOTEFILE_H

#include <QIODevice>
#include <kjob.h>

#include "okular_export.h"

namespace KIO {
    class FileJob;
    class Job;
}

namespace Okular {

    class OKULAR_EXPORT RemoteFile : public QIODevice
    {
	Q_OBJECT

	public :
	    RemoteFile( KIO::FileJob * job );
	    virtual ~RemoteFile();
	    virtual bool waitForReadyRead( int msecs );
	    virtual qint64 bytesAvailable() const;
	    virtual bool seek( qint64 pos );
	    virtual bool atEnd() const;

	protected slots :
	    void slotHandleData( KIO::Job * job, const QByteArray & data );
	    void slotSetMimeType( KIO::Job * job, const QString & type );

	protected :
	    qint64 readData( char * data, qint64 maxSize );
	    qint64 writeData( const char * data, qint64 maxSize );

	private :
	    KIO::FileJob * m_fileJob;
	    QByteArray m_buffer;
	    QString m_mimeType;

    };
}

#endif