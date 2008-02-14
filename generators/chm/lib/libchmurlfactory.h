/***************************************************************************
 *   Copyright (C) 2004-2007 by Georgy Yunaev, gyunaev@ulduzsoft.com       *
 *   Please do not use email address above for bug reports; see            *
 *   the README file                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QDir>
#include <QString>
#include <QRegExp>

namespace LCHMUrlFactory
{
	
static inline bool isRemoteURL( const QString & url, QString & protocol )
{
	// Check whether the URL is external
	QRegExp uriregex ( "^(\\w+):\\/\\/" );
	QRegExp mailtoregex ( "^(mailto):" );

	// mailto: can also have different format, so handle it
	if ( url.startsWith( "mailto:" ) )
	{
		protocol = "mailto";
		return true;
	}
	else if ( uriregex.indexIn( url ) != -1 )
	{
		QString proto = uriregex.cap ( 1 ).toLower();
	
		// Filter the URLs which need to be opened by a browser
		if ( proto == "http" 
						|| proto == "ftp"
						|| proto == "mailto"
						|| proto == "news" )
		{
			protocol = proto;
			return true;
		}
	}

	return false;
}

// Some JS urls start with javascript://
static inline bool isJavascriptURL( const QString & url )
{
	return url.startsWith ("javascript://");
}

// Parse urls like "ms-its:file name.chm::/topic.htm"
static inline bool isNewChmURL( const QString & url, QString & chmfile, QString & page )
{
	QRegExp uriregex ( "^ms-its:(.*)::(.*)$" );
	uriregex.setCaseSensitivity( Qt::CaseInsensitive );

	if ( uriregex.indexIn ( url ) != -1 )
	{
		chmfile = uriregex.cap ( 1 );
		page = uriregex.cap ( 2 );
	
		return true;
	}

	return false;
}

static inline QString makeURLabsoluteIfNeeded( const QString & url )
{
	QString p1, p2, newurl = url;

	if ( !isRemoteURL (url, p1)
	&& !isJavascriptURL (url)
	&& !isNewChmURL (url, p1, p2) )
	{
		newurl = QDir::cleanPath (url);

		// Normalize url, so it becomes absolute
		if ( newurl[0] != '/' )
			newurl = "/" + newurl;
	}

	//qDebug ("makeURLabsolute (%s) -> (%s)", url.ascii(), newurl.ascii());
	return newurl;
}


// Returns a special string, which allows the kio-slave, or viewwindow-browser iteraction 
// to regognize our own internal urls, which is necessary to show image-only pages.
static inline QString getInternalUriExtension()
{
	return ".KCHMVIEWER_SPECIAL_HANDLER";
}


static inline bool handleFileType( const QString& link, QString& generated )
{
	QString intext = getInternalUriExtension();
	
	if ( !link.endsWith( intext ) )
		return false;

	QString filelink = link.left( link.length() - intext.length() );
	generated = "<html><body><img src=\"" + filelink + "\"></body></html>";
	return true;
}


}
