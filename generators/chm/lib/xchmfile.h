/***************************************************************************
 *   Copyright (C) 2005 by Georgy Yunaev                                   *
 *   tim@krasnogorsk.ru                                                    *
 *                                                                         *
 *   Copyright (C) 2003  Razvan Cojocaru <razvanco@gmx.net>                *
 *   XML-RPC/Context ID code contributed by Eamon Millman / PCI Geomatics  *
 *   <millman@pcigeomatics.com>                                            *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef INCLUDE_CHMFILE_H
#define INCLUDE_CHMFILE_H

#include <sys/types.h>

#include <qbytearray.h>
#include <qstring.h>
#include <qmap.h>
#include <qvector.h>
#include <qtextcodec.h>

#include "kchmtextencoding.h"

#include "chm_lib.h"
// #include 

class TGenerator;
class K3ListView;
class KCHMSearchResult
{
	public:
		inline KCHMSearchResult() {}
		inline KCHMSearchResult ( const QString& t, const QString& u ) :
			title(t), url(u) {};
		QString	title;
		QString	url;
};

class KCHMSearchProgressResult
{
	public:
		inline KCHMSearchProgressResult() {}
		inline KCHMSearchProgressResult( u_int32_t t, u_int32_t u ) : titleoff(t),urloff(u) {}
		
		QVector<u_int64_t>		offsets;
		u_int32_t					titleoff;
		u_int32_t					urloff;
};

typedef QVector<KCHMSearchProgressResult>	KCHMSearchProgressResults_t;
typedef QVector<KCHMSearchResult> 			KCHMSearchResults_t;


class KCHMParsedIndexEntry
{
	public:
		KCHMParsedIndexEntry () { m_imagenum = -1; }
		KCHMParsedIndexEntry ( QString name, KCHMParsedIndexEntry * parent = 0, QString url = QString::null, int imagenum = -1 ) : m_parent(parent), m_name (name), m_url (url), m_imagenum (imagenum) {}
		
	public:
		KCHMParsedIndexEntry	*	m_parent;
		QString						m_name;
		QString						m_url;
		int							m_imagenum;
};

typedef QVector<KCHMParsedIndexEntry> 		KCHMParsedIndexEntry_t;
class QDomDocument;

//! Maximum allowed number of search-returned items.
#define MAX_SEARCH_RESULTS 512


//! Mostly a C++ wrapper around the CHMLIB facilities. Concrete class.
class CHMFile
{
    friend class TGenerator;
public:
	//! Default constructor.
	CHMFile();
	
	/*!
	  \brief This constructor attempts to open the .chm file passed as
	  it's argument. If it fails, IsOk() will return false.
	  \param archiveName The .chm filename on disk.
	 */
	CHMFile(const QString& archiveName);

	//! Destructor. If a file has been succesfully opened, it closes it.
	~CHMFile();

	/*!
	  \brief Gets the name of the default page in the archive.
	  \return The home page name, with a '/' added in front and
	  relative to the root of the archive filesystem. If no .chm
	  has been opened, the returned value is "/".
	 */
	QString HomePage() const { return encodeWithCurrentCodec(m_home); }

	/*!
	  \brief Gets name of the .hhc file in the archive that
	  could potentially be used to generate content information from.
	  \return The topics file name, with a '/' added in front and
	  relative to the root of the archive filesystem. If no .chm
	  has been opened, the return value is an empty string.
	*/
	QString TopicsFile() const { return encodeWithCurrentCodec(m_topicsFile); }

	/*!
	  \brief Gets the filename of the currently opened .chm file.
	  \return The filename of the currently opened archive, relative
	  to the root of the filesystem, or the empty string if no
	  archive has been opened.
	 */
	QString ArchiveName() const { return m_filename; }

	/*!
	  \brief Gets name of the .hhk file in the archive that
	  could potentially be used to generate content information from.
	  \return The index file name, with a '/' added in front and
	  relative to the root of the archive filesystem. If no .chm
	  has been opened, the return value is an empty string.
	 */
	QString IndexFile() const { return encodeWithCurrentCodec(m_indexFile); }

	/*!
	  \brief Gets the name of the opened .chm.
	  \return The name of the opened document, or an empty string
	  if no .chm has been loaded.
	*/
	QString Title() const { return encodeWithCurrentCodec(m_title); }

	/*!
	  \brief Gets the name of the default font.
	  \return The name of the default font or the empty string if
	  no default font was selected.
	*/
	QString DefaultFont() const { return m_font; }

	/*!
	  \brief Checks if the last attempt to load a .chm file was
	  succesful.
	  \return true, if the last attempt to load a .chm file was
	  succesful, false otherwise.
	 */
	bool IsOk() const { return m_chmFile != NULL; }

	/*!
	  \brief Attempts to load a .chm file from disk.
	  \param archiveName The .chm filename on disk.
	  \return true on success, false on failure.
	 */
	bool LoadCHM(const QString& archiveName);

	//! Closes the currently opened .chm, or does nothing if none opened.
	void CloseCHM();

	/*!
	  \brief Attempts to fill a QListView by parsing the topics file.
	  \param toBuild Pointer to the QListView tree to be filled.
	  If the topics file is not available, the
	  tree is unmodified. The tree must be empty before passing it
	  to this function.
	  \return true if it's possible to build the tree, false otherwise.
	 */
	bool ParseAndFillTopicsTree(QDomDocument*toBuild);

	/*!
	  \brief Attempts to fill a QListView by parsing the index file.
	  \param indexlist Pointer to the document to be filled.
	  If the index file is not available, the list control
	  is unmodified. The document must be empty before passing it
	  to this function.
	  \return true if it's possible to build the tree, false otherwise.
	 */
	bool ParseAndFillIndex (K3ListView *indexlist);

	/*!
	  \brief Fast search using the $FIftiMain file in the .chm.
	  \param word The text we're looking for.
	  \param wholeWords Are we looking for whole words only?
	  \param titlesOnly Are we looking for titles only?
	  \param results A string-string hashmap that will hold
	  the results in case of successful search. The keys are
	  the URLs and the values are the page titles.
	  \param phrase_search Indicates that word offset information should be kept.
	  \return true if the search found something, false otherwise.
	 */
	bool SearchWord (const QString& word, bool wholeWords, bool titlesOnly, KCHMSearchProgressResults_t& results, bool phrase_search);

	/*!
	\brief Finalize the search, resolves all the and generate the results.
	\param tempres Temporary search results from SearchWord.
	\param results A string-string hashmap that will hold the results in case of successful search.
			The keys are the URLs and the values are the page titles.
	\param limit_results Maximum number of results to return.
	*/
	void GetSearchResults ( const KCHMSearchProgressResults_t& tempres, KCHMSearchResults_t& results, unsigned int limit_results = 500 );

	/*!
	  \brief Looks up fileName in the archive.
	  \param fileName The file name to look up in the archive,
	  qualified with '/' standing for the root of the archive
	  filesystem.
	  \param ui A pointer to CHMLIB specific data about the file.
	  The parameter gets filled with useful data if the lookup 
	  was succesful.
	  \return true if the file exists in the archive, false otherwise.
	 */
	bool ResolveObject(const QString& fileName, chmUnitInfo *ui);

	/*!
	  \brief Retrieves an uncompressed chunk of a file in the .chm.
	  \param ui Pointer to a CHMLIB specific data structure obtained
	  from a succesful call to ResolveObject().
	  \param buffer The buffer to place the chunk into.
	  \param fileOffset Where does the chunk we want begin in the file?
	  \param bufferSize The size of the buffer.
	  \return 0 on error, length of chunk retrieved otherwise.
	 */
	size_t RetrieveObject(chmUnitInfo *ui, unsigned char *buffer, LONGUINT64 fileOffset, LONGINT64 bufferSize);

	//! Puts in the str parameter the contents of the file referred by ui.
	bool GetFileContentAsString (QString& str, chmUnitInfo *ui);
	
	//! Puts in the str parameter the contents of the file referred by location.
	bool GetFileContentAsString (QString& str, QString location);

	/*! 
	 * Puts in the str parameter the contents of the file filename referred by location.
	 * If file has not been opened, it is opened, and added into m_chmLoadedFiles.
	 */
	bool GetFileContentAsString (QString& str, QString filename, QString location);

	/*! 
	 * Enumerate every file in the CHM archive, and store its path into the supplied vector.
	 */
	bool enumerateArchive (QVector<QString>& files);
	
	/*! 
	 * Find the tree item specified by URL.
	 * Used to find the tree item related to the linked page.
	 */
    int getPageNum( const QString & str ) const;
    QString getUrlForPage( int p ) const;

	//! Returns true if built-in search tree is present.
	bool  isSearchAvailable () { return m_searchAvailable; }
	
	//! Returns current CHM file text encoding.
	const KCHMTextEncoding::text_encoding_t * getCurrentEncoding() { return m_currentEncoding; }

	//! Sets the CHM file text encoding.
	bool setCurrentEncoding ( const KCHMTextEncoding::text_encoding_t * enc);

	//! Gets the CHM file pointer. Used when more than one CHM file is loaded (i.e. cross-links in CHM files)
	CHMFile * getCHMfilePointer ( const QString& filename );

	//! Returns the topic from #TOPICS
	QString		getTopicByUrl ( const QString& url );

private:
	//! Parse the HHC or HHS file, and fill the context (asIndex is false) or index (asIndex is true) tree.
	bool  ParseHhcAndFillTree (const QString& file, QDomDocument *tree, bool asIndex);
    bool  ParseHhcAndFillTree (const QString& file, K3ListView *tree, bool asIndex);
	//! Parse the HHC or HHS file, and fill the data.
	bool ParseChmIndexFile ( const QString& file, bool asIndex, KCHMParsedIndexEntry_t & cont );

	//! Encode the string with the currently selected text codec, if possible. Or return as-is, if not.
	inline QString encodeWithCurrentCodec (const QString& str) const
	{
		return (m_textCodec ? m_textCodec->toUnicode (str.toLocal8Bit()) : str);
	}

	//! Encode the string with the currently selected text codec, if possible. Or return as-is, if not.
	inline QString encodeWithCurrentCodec (const char * str) const
	{
		return (m_textCodec ? m_textCodec->toUnicode (str) : (QString) str);
	}
	
	//! Helper. Translates from Win32 encodings to generic wxWidgets ones.
	const char * GetFontEncFromCharSet (const QString& font) const;

	//! Helper. Returns the $FIftiMain offset of leaf node or 0.
	u_int32_t GetLeafNodeOffset(const QString& text,
				    u_int32_t initalOffset,
				    u_int32_t buffSize,
				    u_int16_t treeDepth);

	//! Helper. Processes the word location code entries while searching.
	bool ProcessWLC(u_int64_t wlc_count, u_int64_t wlc_size,
			u_int32_t wlc_offset, unsigned char ds,
			unsigned char dr, unsigned char cs,
			unsigned char cr, unsigned char ls,
			unsigned char lr, KCHMSearchProgressResults_t& results,
			bool phrase_search);

	//! Looks up as much information as possible from #WINDOWS/#STRINGS.
	bool InfoFromWindows();

	//! Looks up as much information as possible from #SYSTEM.
	bool InfoFromSystem();
	
	//! Sets up textCodec
	void setupTextCodec (const char * name);

	//! Guess used text encoding, using m_detectedLCID and m_font. Set up m_textCodec
	bool guessTextEncoding ();

	//! Change the current CHM encoding
	bool  changeFileEncoding (const char *qtencoding);

	//! Convert the word, so it has an appropriate encoding
	QByteArray convertSearchWord ( const QString &src );

	/*!
	 * Helper procedure in TOC parsing, decodes the string between the quotes (first or last) with decoding HTML
	 * entities like &iacute;
	 */
	inline int findStringInQuotes (const QString& tag, int offset, QString& value, bool firstquote, bool decodeentities);

private:
// 	typedef QMap<QString, KCHMMainTreeViewItem*> KCHMTreeUrlMap_t;

	//! Pointer to the chmlib structure
	chmFile	*	m_chmFile;
	
	//! Opened file name
	QString  	m_filename;
	
	//! Home url, got from CHM file
	QString  	m_home;

	//! Context tree filename. Got from CHM file
	QString  	m_topicsFile;

	//! Index filename. Got from CHM file
	QString 	m_indexFile;

	//! Chm Title. Got from CHM file
	QString		m_title;

	//! Used by getTreeItem() to find the tree element quickly
    QMap <QString, int> m_UrlPage;
    QMap <int,QString> m_PageUrl;

	// Localization stuff
	//! LCID from CHM file, used in encoding detection
	short			m_detectedLCID;

	//! font charset from CHM file, used in encoding detection
	QString 		m_font;

	//! Chosen text codec
	QTextCodec	*	m_textCodec;

	//! Current encoding
	const KCHMTextEncoding::text_encoding_t * m_currentEncoding;

	typedef QMap<QString, CHMFile *> 		chm_loaded_files_t;

	//! Loaded chm files
	chm_loaded_files_t	m_chmLoadedFiles;

	//! Map to decode HTML entitles like &acute; based on current encoding
	QMap<QString, QString>					m_entityDecodeMap;

	//! true if /#TOPICS, /#STRINGS, /#URLTBL and  /#URLSTR are resolved, and the members below are valid
	bool		m_lookupTablesValid;

	//! pointer to /#TOPICS
	chmUnitInfo	m_chmTOPICS;

	//! pointer to /#STRINGS
	chmUnitInfo	m_chmSTRINGS;

	//! pointer to /#URLTBL
	chmUnitInfo	m_chmURLTBL;

	//! pointer to /#URLSTR
	chmUnitInfo	m_chmURLSTR;

	//! Indicates whether the built-in search is available. This is true only when m_lookupTablesValid
	//! is true, and m_chmFIftiMain is resolved.
	bool			m_searchAvailable;

	//! pointer to /$FIftiMain
	chmUnitInfo	m_chmFIftiMain;
	
private:
	//! No copy construction allowed.
	CHMFile(const CHMFile&);

	//! No assignments.
	CHMFile& operator=(const CHMFile&);
};


#endif // INCLUDE_CHMFILE_H
