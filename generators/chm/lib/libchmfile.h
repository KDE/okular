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

#ifndef INCLUDE_LIBCHMFILE_H
#define INCLUDE_LIBCHMFILE_H

#include <QString>
#include <QMap>
#include <QVector>
#include <QTextCodec> 
#include <QPixmap>

#include "libchmtextencoding.h"


//! Contains different (non-standard) image types
namespace LCHMBookIcons
{
	const int IMAGE_NONE = -1;
	const int IMAGE_AUTO = -2;
	const int IMAGE_INDEX = -3;
	
	const int MAX_BUILTIN_ICONS = 42;
}


//! Contains a single index or TOC entry. See LCHMFile::parseTOC() and LCHMFile::parseIndex()
typedef struct
{
	//! Entry name
	QString		name;
	
	//! Entry URLs. The TOC entry should have only one URL; the index entry could have several.
	QStringList	urls;
	
	//! Associated image number. Used for TOC only; indexes does not have the image. 
	//! Use LCHMFile::getBookIconPixmap() to get associated pixmap icon
	int			imageid;
	
	//! Indentation level for this entry.
	int			indent;

} LCHMParsedEntry;


// forward declaration
class LCHMFileImpl;

//! CHM files processor, heavily based on chmlib. Used search code from xchm.
class LCHMFile
{
	public:
		//! Default constructor and destructor.
		LCHMFile();
		~LCHMFile();
		
		/*!
		 * \brief Attempts to load a .chm file.
		 * \param archiveName The .chm filename.
		 * \return true on success, false on failure.
		 *
		 * Loads a CHM file. Could internally load more than one file, if files linked to 
		 * this one are present locally (like MSDN).
		 * \ingroup init
		 */
		bool loadFile( const QString& archiveName );

		/*!
		 * \brief Closes all the files, and frees the appropriate data.
		 * \ingroup init
		 */
		void closeAll();
		
		/*!
		 * \brief Gets the title name of the opened .chm.
		 * \return The name of the opened document, or an empty string if no .chm has been loaded.
		 * \ingroup information
		 */
		QString title() const;
		
		/*!
		 * \brief Gets the URL of the default page in the chm archive.
		 * \return The home page name, with a '/' added in front and relative to
		 *         the root of the archive filesystem. If no .chm has been opened,
		 *         returns "/".
		 * \ingroup information
		 */
		QString homeUrl() const;
		
		/*!
		 * \brief Checks whether the Table of Contents is present in this file.
		 * \return true if it is available; false otherwise.
		 * \ingroup information
		 */
		bool  hasTableOfContents() const;
		
		/*!
		 * \brief Checks whether the Index Table is present in this file.
		 * \return true if it is available; false otherwise.
		 * \ingroup information
		 */
		bool  hasIndexTable() const;
		
		/*!
		 * \brief Checks whether the Search Table is available in this file.
		 * \return true if it is available; false otherwise.
		 * \ingroup information
		 *
		 * If the search table is not available, the search is not possible.
		 */
		bool  hasSearchTable() const;
		
		/*!
		 * \brief Parses the Table of Contents (TOC)
		 * \param topics A pointer to the container which will store the parsed results. 
		 *               Will be cleaned before parsing.
		 * \return true if the tree is present and parsed successfully, false otherwise.
		 *         The parser is built to be error-prone, however it still can abort with qFatal()
		 *         by really buggy chm file; please report a bug if the file is opened ok under Windows.
		 * \ingroup fileparsing
		 */
		bool parseTableOfContents( QVector< LCHMParsedEntry > * topics ) const;

		/*!
		 * \brief Parses the Index Table
		 * \param indexes A pointer to the container which will store the parsed results. 
		 *               Will be cleaned before parsing.
		 * \return true if the tree is present and parsed successfully, false otherwise.
		 *         The parser is built to be error-prone, however it still can abort with qFatal()
		 *         by really buggy chm file; so far it never happened on indexes.
		 * \ingroup fileparsing
		 */
		bool parseIndex( QVector< LCHMParsedEntry > * indexes ) const;

		/*!
		 * \brief Retrieves the content from url in current chm file to QString.
		 * \param str A string where the retrieved content should be stored.
		 * \param url An URL in chm file to retrieve content from. Must be absolute.
		 * \return true if the content is successfully received; false otherwise.
		 *
		 * This function retrieves the file content (mostly for HTML pages) from the chm archive
		 * opened by load() function. Because the content in chm file is not stored in Unicode, it 
		 * will be recoded according to current encoding. Do not use for binary data.
		 *
		 * \sa setCurrentEncoding() currentEncoding() getFileContentAsBinary()
		 * \ingroup dataretrieve
		 */
		bool getFileContentAsString( QString * str, const QString& url );

		/*!
		 * \brief Retrieves the content from url in current chm file to QByteArray.
		 * \param data A data array where the retrieved content should be stored.
		 * \param url An URL in chm file to retrieve content from. Must be absolute.
		 * \return true if the content is successfully received; false otherwise.
		 *
		 * This function retrieves the file content from the chm archive opened by load() 
		 * function. The content is not encoded.
		 *
		 * \sa getFileContentAsString()
		 * \ingroup dataretrieve
		 */
		bool getFileContentAsBinary( QByteArray * data, const QString& url );
		
		/*!
		 * \brief Retrieves the content size.
		 * \param size A pointer where the size will be stored.
		 * \param url An URL in chm file to retrieve content from. Must be absolute.
		 * \return true if the content size is successfully stored; false otherwise.
		 *
		 * \ingroup dataretrieve
		 */
		bool getFileSize( unsigned int * size, const QString& url );
		
		/*!
		 * \brief Obtains the list of all the files in current chm file archive.
		 * \param files An array to store list of URLs (file names) present in chm archive.
		 * \return true if the enumeration succeed; false otherwise (I could hardly imagine a reason).
		 *
		 * \ingroup dataretrieve
		 */
		bool enumerateFiles( QStringList * files );
	
		/*!
		 * \brief Gets the Title of the HTML page referenced by url.
		 * \param url An URL in chm file to get title from. Must be absolute.
		 * \return The title, or QString::null if the URL cannot be found or not a HTML page.
		 *
		 * \ingroup dataretrieve
		 */
		QString		getTopicByUrl ( const QString& url );
	
		/*!
		 * \brief Gets the appropriate CHM pixmap icon.
		 * \param imagenum The image number from TOC.
		 * \return The pixmap to show in TOC tree.
		 *
		 * \ingroup dataretrieve
		 */
		const QPixmap * getBookIconPixmap( unsigned int imagenum );
		
		/*!
		 * \brief Normalizes the URL, converting relatives, adding "/" in front and removing ..
		 * \param url The URL to normalize.
		 * \return The normalized, cleaned up URL.
		 *
		 * \ingroup dataretrieve
		 */
		QString normalizeUrl( const QString& url ) const;
		
		/*!
		 * \brief Gets the current CHM archive encoding (set or autodetected)
		 * \return The current encoding.
		 *
		 * \ingroup encoding
		 */
		const LCHMTextEncoding * currentEncoding() const;
		
		/*!
		 * \brief Sets the CHM archive encoding to use
		 * \param encoding An encoding to use.
		 *
		 * \ingroup encoding
		 */
		bool setCurrentEncoding ( const LCHMTextEncoding * encoding );
		
		/*!
		 * \brief Execute a search query, return the results.
		 * \param query A search query.
		 * \param results An array to store URLs where the query was found.
		 * \return true if search was successful (this does not mean that it returned any results); 
		 *         false otherwise.
		 *
		 * This function executes a standard search query. The query should consist of one of more 
		 *  words separated by a space with a possible prefix. A prefix may be:
		 *   +   Plus indicates that the word is required; any page without this word is excluded from the result.
		 *   -   Minus indicates that the word is required to be absent; any page with this word is excluded from
		 *       the result.
		 *   "." Quotes indicates a phrase. Anything between quotes is a phrase, which is set of space-separated
		 *       words. Will be in result only if the words in phrase are in page in the same sequence, and
		 *       follow each other.
		 *
		 *   If there is no prefix, the word considered as required.
		 * \ingroup search
		 */
		bool	searchQuery ( const QString& query, QStringList * results, unsigned int limit = 100 );
		
		//! Access to implementation
		LCHMFileImpl * impl()	{ return m_impl; }
		
	private:
		//! No copy construction allowed.
		LCHMFile( const LCHMFile& );
		
		//! No assignments allowed.
		LCHMFile& operator=( const LCHMFile& );
		
		//! Implementation
		LCHMFileImpl *	m_impl;
};


#endif // INCLUDE_LIBCHMFILE_H
