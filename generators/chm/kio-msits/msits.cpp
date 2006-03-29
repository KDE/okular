/*  This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kurl.h>
#include <kmimemagic.h>

#include <qfile.h>
#include <qbitarray.h>
#include <qvector.h>

#include "msits.h"


using namespace KIO;

extern "C"
{
    int kdemain( int argc, char **argv )
    {
		kDebug() << "*** kio_msits Init" << endl;

        KInstance instance( "kio_msits" );

        if ( argc != 4 )
		{
            kDebug() << "Usage: kio_msits protocol domain-socket1 domain-socket2" << endl;
            exit (-1);
        }

        ProtocolMSITS slave ( argv[2], argv[3] );
        slave.dispatchLoop();

        kDebug() << "*** kio_msits Done" << endl;
        return 0;
    }
}

ProtocolMSITS::ProtocolMSITS (const Q3CString &pool_socket, const Q3CString &app_socket)
	: SlaveBase ("kio_msits", pool_socket, app_socket)
{
	m_chmFile = 0;
}

ProtocolMSITS::~ProtocolMSITS()
{
	if ( !m_chmFile )
		return;

	chm_close (m_chmFile);
	m_chmFile = 0;
}

// A simple stat() wrapper
static bool isDirectory ( const QString & filename )
{
	return filename[filename.length() - 1] == '/';
}


void ProtocolMSITS::get( const KUrl& url )
{
	QString fileName;
	chmUnitInfo ui;

    kDebug() << "kio_msits::get() " << url.path() << endl;

	if ( !parseLoadAndLookup ( url, fileName ) )
		return;	// error() has been called by parseLoadAndLookup

	kDebug() << "kio_msits::get: parseLoadAndLookup returned " << fileName << endl;

	if ( isDirectory (fileName) )
	{
		error( KIO::ERR_IS_DIRECTORY, url.prettyURL() );
		return;
	}

	if ( !ResolveObject ( fileName, &ui) )
	{
		kDebug() << "kio_msits::get: could not resolve filename " << fileName << endl;
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyURL() );
		return;
	}

    QByteArray buf (ui.length);

	if ( RetrieveObject (&ui, (unsigned char*) buf.data(), 0, ui.length) == 0 )
	{
		kDebug() << "kio_msits::get: could not retrieve filename " << fileName << endl;
        error( KIO::ERR_NO_CONTENT, url.prettyURL() );
		return;
	}

    totalSize( ui.length );
    KMimeMagicResult * result = KMimeMagic::self()->findBufferFileType( buf, fileName );
    kDebug() << "Emitting mimetype " << result->mimeType() << endl;

	mimeType( result->mimeType() );
    data( buf );
	processedSize( ui.length );

    finished();
}


bool ProtocolMSITS::parseLoadAndLookup ( const KUrl& url, QString& abspath )
{
	kDebug() << "ProtocolMSITS::parseLoadAndLookup (const KUrl&) " << url.path() << endl;

	int pos = url.path().indexOf ("::");

	if ( pos == -1 )
	{
		error( KIO::ERR_MALFORMED_URL, url.prettyURL() );
		return false;
	}

	QString filename = url.path().left (pos);
	abspath = url.path().mid (pos + 2); // skip ::

	kDebug() << "ProtocolMSITS::parseLoadAndLookup: filename " << filename << ", path " << abspath << endl;

    if ( filename.isEmpty() )
    {
		error( KIO::ERR_MALFORMED_URL, url.prettyURL() );
        return false;
    }

	// If the file has been already loaded, nothing to do.
	if ( m_chmFile && filename == m_openedFile )
		return true;

    kDebug() << "Opening a new CHM file " << filename << endl;

	// First try to open a temporary file
	chmFile * tmpchm;

	if ( (tmpchm = chm_open ( QFile::encodeName (filename))) == 0 )
	{
		error( KIO::ERR_COULD_NOT_READ, url.prettyURL() );
        return false;
	}

	// Replace an existing file by a new one
	if ( m_chmFile )
		chm_close (m_chmFile);
	
	m_chmFile = tmpchm;
	m_openedFile = filename;
    
	kDebug() << "A CHM file " << filename << " has beed opened successfully" << endl;
    return true;
}

/*
 * Shamelessly stolen from a KDE KIO tutorial
 */
static void app_entry(UDSEntry& e, unsigned int uds, const QString& str)
{
	e.insert(uds, str);
}

 // appends an int with the UDS-ID uds
 static void app_entry(UDSEntry& e, unsigned int uds, long l)
 {
	e.insert(uds, l);
}

// internal function
// fills a directory item with its name and size
static void app_dir(UDSEntry& e, const QString & name)
{
	e.clear();
	app_entry(e, KIO::UDS_NAME, name);
	app_entry(e, KIO::UDS_FILE_TYPE, S_IFDIR);
	app_entry(e, KIO::UDS_SIZE, 1);
}

// internal function
// fills a file item with its name and size
static void app_file(UDSEntry& e, const QString & name, size_t size)
{
	e.clear();
	app_entry(e, KIO::UDS_NAME, name);
	app_entry(e, KIO::UDS_FILE_TYPE, S_IFREG);
	app_entry(e, KIO::UDS_SIZE, size);
}

void ProtocolMSITS::stat (const KUrl & url)
{
	QString fileName;
	chmUnitInfo ui;

    kDebug() << "kio_msits::stat (const KUrl& url) " << url.path() << endl;

	if ( !parseLoadAndLookup ( url, fileName ) )
		return;	// error() has been called by parseLoadAndLookup

	if ( !ResolveObject ( fileName, &ui ) )
	{
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyURL() );
		return;
	}

	kDebug() << "kio_msits::stat: adding an entry for " << fileName << endl;
	UDSEntry entry;

	if ( isDirectory ( fileName ) )
		app_dir(entry, fileName);
	else
		app_file(entry, fileName, ui.length);
 
	statEntry (entry);

	finished();
}


// A local CHMLIB enumerator
static int chmlib_enumerator (struct chmFile *, struct chmUnitInfo *ui, void *context)
{
	((QVector<QString> *) context)->push_back (QString::fromLocal8Bit (ui->path));
	return CHM_ENUMERATOR_CONTINUE;
}


void ProtocolMSITS::listDir (const KUrl & url)
{
	QString filepath;

    kDebug() << "kio_msits::listDir (const KUrl& url) " << url.path() << endl;

	if ( !parseLoadAndLookup ( url, filepath ) )
		return;	// error() has been called by parseLoadAndLookup

	filepath += "/";

	if ( !isDirectory (filepath) )
	{
		error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path());
		return;
	}

    kDebug() << "kio_msits::listDir: enumerating directory " << filepath << endl;

	QVector<QString> listing;

	if ( chm_enumerate_dir ( m_chmFile,
							filepath.local8Bit(),
							CHM_ENUMERATE_NORMAL | CHM_ENUMERATE_FILES | CHM_ENUMERATE_DIRS,
							chmlib_enumerator,
							&listing ) != 1 )
	{
		error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path());
		return;
	}

//	totalFiles(listing.size());
	UDSEntry entry;
	unsigned int striplength = filepath.length();

	for ( unsigned int i = 0; i < listing.size(); i++ )
	{
		// Strip the direcroty name
		QString ename = listing[i].mid (striplength);

		if ( isDirectory ( ename ) )
			app_dir(entry, ename);
		else
			app_file(entry, ename, 0);
 
		listEntry(entry, false);
	}

	listEntry(entry, true);
	finished();
}
