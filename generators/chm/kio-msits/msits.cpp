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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kurl.h>
#include <kmimetype.h>

#include <qfile.h>
#include <qdir.h>
#include <qbitarray.h>
#include <qvector.h>

#include "msits.h"
#include "libchmurlfactory.h"

using namespace KIO;

extern "C"
{
    int KDE_EXPORT kdemain( int argc, char **argv )
    {
		kDebug() << "*** kio_msits Init";

        KComponentData instance( "kio_msits" );

        if ( argc != 4 )
		{
            kDebug() << "Usage: kio_msits protocol domain-socket1 domain-socket2";
            exit (-1);
        }

        ProtocolMSITS slave ( argv[2], argv[3] );
        slave.dispatchLoop();

        kDebug() << "*** kio_msits Done";
        return 0;
    }
}

ProtocolMSITS::ProtocolMSITS (const QByteArray &pool_socket, const QByteArray &app_socket)
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
    return filename.endsWith( '/' );
}


void ProtocolMSITS::get( const KUrl& url )
{
	QString htmdata, fileName;
	chmUnitInfo ui;
	QByteArray buf;

    kDebug() << "kio_msits::get() " << url.path();

	if ( !parseLoadAndLookup ( url, fileName ) )
		return;	// error() has been called by parseLoadAndLookup

	kDebug() << "kio_msits::get: parseLoadAndLookup returned " << fileName;

	if ( LCHMUrlFactory::handleFileType( url.path(), htmdata ) )
	{
		buf = htmdata.toUtf8();
		kDebug() << "Using special handling for image pages: " << htmdata;
	}
	else
	{
		if ( isDirectory (fileName) )
		{
			error( KIO::ERR_IS_DIRECTORY, url.prettyUrl() );
			return;
		}

		if ( !ResolveObject ( fileName, &ui) )
		{
			kDebug() << "kio_msits::get: could not resolve filename " << fileName;
        	error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
			return;
		}

    	buf.resize( ui.length );

		if ( RetrieveObject (&ui, (unsigned char*) buf.data(), 0, ui.length) == 0 )
		{
			kDebug() << "kio_msits::get: could not retrieve filename " << fileName;
        	error( KIO::ERR_NO_CONTENT, url.prettyUrl() );
			return;
		}
	}

    totalSize( buf.size() );
    KMimeType::Ptr result = KMimeType::findByNameAndContent( fileName, buf );
    kDebug() << "Emitting mimetype " << result->name();

	mimeType( result->name() );
    data( buf );
	processedSize( buf.size() );

    finished();
}


bool ProtocolMSITS::parseLoadAndLookup ( const KUrl& url, QString& abspath )
{
	kDebug() << "ProtocolMSITS::parseLoadAndLookup (const KUrl&) " << url.path();

	int pos = url.path().indexOf ("::");

	if ( pos == -1 )
	{
		error( KIO::ERR_MALFORMED_URL, url.prettyUrl() );
		return false;
	}

	QString filename = url.path().left (pos);
	abspath = url.path().mid (pos + 2); // skip ::
	
	// Some buggy apps add ms-its:/ to the path as well
	if ( abspath.startsWith( "ms-its:" ) )
		abspath = abspath.mid( 7 );
			
	kDebug() << "ProtocolMSITS::parseLoadAndLookup: filename " << filename << ", path " << abspath;

    if ( filename.isEmpty() )
    {
		error( KIO::ERR_MALFORMED_URL, url.prettyUrl() );
        return false;
    }

	// If the file has been already loaded, nothing to do.
	if ( m_chmFile && filename == m_openedFile )
		return true;

    kDebug() << "Opening a new CHM file " << QFile::encodeName( QDir::toNativeSeparators( filename ) );

	// First try to open a temporary file
	chmFile * tmpchm;

	if( (tmpchm = chm_open ( QFile::encodeName( QDir::toNativeSeparators( filename) ) ) ) == 0 )
	{
		error( KIO::ERR_COULD_NOT_READ, url.prettyUrl() );
        return false;
	}

	// Replace an existing file by a new one
	if ( m_chmFile )
		chm_close (m_chmFile);
	
	m_chmFile = tmpchm;
	m_openedFile = filename;
    
	kDebug() << "A CHM file " << filename << " has beed opened successfully";
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
	app_entry(e, KIO::UDSEntry::UDS_NAME, name);
	app_entry(e, KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
	app_entry(e, KIO::UDSEntry::UDS_SIZE, 1);
}

// internal function
// fills a file item with its name and size
static void app_file(UDSEntry& e, const QString & name, size_t size)
{
	e.clear();
	app_entry(e, KIO::UDSEntry::UDS_NAME, name);
	app_entry(e, KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
	app_entry(e, KIO::UDSEntry::UDS_SIZE, size);
}

void ProtocolMSITS::stat (const KUrl & url)
{
	QString fileName;
	chmUnitInfo ui;

    kDebug() << "kio_msits::stat (const KUrl& url) " << url.path();

	if ( !parseLoadAndLookup ( url, fileName ) )
		return;	// error() has been called by parseLoadAndLookup

	if ( !ResolveObject ( fileName, &ui ) )
	{
        error( KIO::ERR_DOES_NOT_EXIST, url.prettyUrl() );
		return;
	}

	kDebug() << "kio_msits::stat: adding an entry for " << fileName;
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

    kDebug() << "kio_msits::listDir (const KUrl& url) " << url.path();

	if ( !parseLoadAndLookup ( url, filepath ) )
		return;	// error() has been called by parseLoadAndLookup

	filepath += '/';

	if ( !isDirectory (filepath) )
	{
		error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path());
		return;
	}

    kDebug() << "kio_msits::listDir: enumerating directory " << filepath;

	QVector<QString> listing;

	if ( chm_enumerate_dir ( m_chmFile,
							filepath.toLocal8Bit(),
							CHM_ENUMERATE_NORMAL | CHM_ENUMERATE_FILES | CHM_ENUMERATE_DIRS,
							chmlib_enumerator,
							&listing ) != 1 )
	{
		error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path());
		return;
	}

	UDSEntry entry;
	int striplength = filepath.length();

	for ( int i = 0; i < listing.size(); i++ )
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
