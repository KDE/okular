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

#include <qmessagebox.h> 
#include <qstring.h>
#include <qregexp.h>
#include <qmap.h>
#include <qeventloop.h>
#include <qdom.h>
#include <qfile.h>
#include <kurl.h>
#include <k3listview.h>

#include "xchmfile.h"
#include "iconstorage.h"
#include "bitfiddle.h"
#include "kchmurl.h"
#include "kchmtreeviewitem.h"

// Big-enough buffer size for use with various routines.
#define BUF_SIZE 4096
#define COMMON_BUF_LEN 1025

#define TOPICS_ENTRY_LEN 16
#define URLTBL_ENTRY_LEN 12

// A little helper to show wait cursor
#include <qcursor.h>
#include <qapplication.h>


class KCHMShowWaitCursor
{
public:
	KCHMShowWaitCursor () { QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) ); }
	~KCHMShowWaitCursor () { QApplication::restoreOverrideCursor(); }
};


inline int CHMFile::findStringInQuotes (const QString& tag, int offset, QString& value, bool firstquote, bool decodeentities)
{
	// Set up m_entityDecodeMap characters according to current textCodec
	if ( m_entityDecodeMap.isEmpty() )
	{
		m_entityDecodeMap["AElig"]	= encodeWithCurrentCodec ("\306"); // capital AE diphthong (ligature)
		m_entityDecodeMap["Aacute"]	= encodeWithCurrentCodec ("\301"); // capital A, acute accent
		m_entityDecodeMap["Acirc"]	= encodeWithCurrentCodec ("\302"); // capital A, circumflex accent
		m_entityDecodeMap["Agrave"]	= encodeWithCurrentCodec ("\300"); // capital A, grave accent
		m_entityDecodeMap["Aring"]	= encodeWithCurrentCodec ("\305"); // capital A, ring
		m_entityDecodeMap["Atilde"]	= encodeWithCurrentCodec ("\303"); // capital A, tilde
		m_entityDecodeMap["Auml"]	= encodeWithCurrentCodec ("\304"); // capital A, dieresis or umlaut mark
		m_entityDecodeMap["Ccedil"]	= encodeWithCurrentCodec ("\307"); // capital C, cedilla
		m_entityDecodeMap["Dstrok"]	= encodeWithCurrentCodec ("\320"); // whatever
		m_entityDecodeMap["ETH"]	= encodeWithCurrentCodec ("\320"); // capital Eth, Icelandic
		m_entityDecodeMap["Eacute"]	= encodeWithCurrentCodec ("\311"); // capital E, acute accent
		m_entityDecodeMap["Ecirc"]	= encodeWithCurrentCodec ("\312"); // capital E, circumflex accent
		m_entityDecodeMap["Egrave"]	= encodeWithCurrentCodec ("\310"); // capital E, grave accent
		m_entityDecodeMap["Euml"]	= encodeWithCurrentCodec ("\313"); // capital E, dieresis or umlaut mark
		m_entityDecodeMap["Iacute"]	= encodeWithCurrentCodec ("\315"); // capital I, acute accent
		m_entityDecodeMap["Icirc"]	= encodeWithCurrentCodec ("\316"); // capital I, circumflex accent
		m_entityDecodeMap["Igrave"]	= encodeWithCurrentCodec ("\314"); // capital I, grave accent
		m_entityDecodeMap["Iuml"]	= encodeWithCurrentCodec ("\317"); // capital I, dieresis or umlaut mark
		m_entityDecodeMap["Ntilde"]	= encodeWithCurrentCodec ("\321"); // capital N, tilde
		m_entityDecodeMap["Oacute"]	= encodeWithCurrentCodec ("\323"); // capital O, acute accent
		m_entityDecodeMap["Ocirc"]	= encodeWithCurrentCodec ("\324"); // capital O, circumflex accent
		m_entityDecodeMap["Ograve"]	= encodeWithCurrentCodec ("\322"); // capital O, grave accent
		m_entityDecodeMap["Oslash"]	= encodeWithCurrentCodec ("\330"); // capital O, slash
		m_entityDecodeMap["Otilde"]	= encodeWithCurrentCodec ("\325"); // capital O, tilde
		m_entityDecodeMap["Ouml"]	= encodeWithCurrentCodec ("\326"); // capital O, dieresis or umlaut mark
		m_entityDecodeMap["THORN"]	= encodeWithCurrentCodec ("\336"); // capital THORN, Icelandic
		m_entityDecodeMap["Uacute"]	= encodeWithCurrentCodec ("\332"); // capital U, acute accent
		m_entityDecodeMap["Ucirc"]	= encodeWithCurrentCodec ("\333"); // capital U, circumflex accent
		m_entityDecodeMap["Ugrave"]	= encodeWithCurrentCodec ("\331"); // capital U, grave accent
		m_entityDecodeMap["Uuml"]	= encodeWithCurrentCodec ("\334"); // capital U, dieresis or umlaut mark
		m_entityDecodeMap["Yacute"]	= encodeWithCurrentCodec ("\335"); // capital Y, acute accent
		
		m_entityDecodeMap["aacute"]	= encodeWithCurrentCodec ("\341"); // small a, acute accent
		m_entityDecodeMap["acirc"]	= encodeWithCurrentCodec ("\342"); // small a, circumflex accent
		m_entityDecodeMap["aelig"]	= encodeWithCurrentCodec ("\346"); // small ae diphthong (ligature)
		m_entityDecodeMap["agrave"]	= encodeWithCurrentCodec ("\340"); // small a, grave accent
		m_entityDecodeMap["aring"]	= encodeWithCurrentCodec ("\345"); // small a, ring
		m_entityDecodeMap["atilde"]	= encodeWithCurrentCodec ("\343"); // small a, tilde
		m_entityDecodeMap["auml"]	= encodeWithCurrentCodec ("\344"); // small a, dieresis or umlaut mark
		m_entityDecodeMap["ccedil"]	= encodeWithCurrentCodec ("\347"); // small c, cedilla
		m_entityDecodeMap["eacute"]	= encodeWithCurrentCodec ("\351"); // small e, acute accent
		m_entityDecodeMap["ecirc"]	= encodeWithCurrentCodec ("\352"); // small e, circumflex accent
		m_entityDecodeMap["egrave"]	= encodeWithCurrentCodec ("\350"); // small e, grave accent
		m_entityDecodeMap["eth"]	= encodeWithCurrentCodec ("\360"); // small eth, Icelandic
		m_entityDecodeMap["euml"]	= encodeWithCurrentCodec ("\353"); // small e, dieresis or umlaut mark
		m_entityDecodeMap["iacute"]	= encodeWithCurrentCodec ("\355"); // small i, acute accent
		m_entityDecodeMap["icirc"]	= encodeWithCurrentCodec ("\356"); // small i, circumflex accent
		m_entityDecodeMap["igrave"]	= encodeWithCurrentCodec ("\354"); // small i, grave accent
		m_entityDecodeMap["iuml"]	= encodeWithCurrentCodec ("\357"); // small i, dieresis or umlaut mark
		m_entityDecodeMap["ntilde"]	= encodeWithCurrentCodec ("\361"); // small n, tilde
		m_entityDecodeMap["oacute"]	= encodeWithCurrentCodec ("\363"); // small o, acute accent
		m_entityDecodeMap["ocirc"]	= encodeWithCurrentCodec ("\364"); // small o, circumflex accent
		m_entityDecodeMap["ograve"]	= encodeWithCurrentCodec ("\362"); // small o, grave accent
		m_entityDecodeMap["oslash"]	= encodeWithCurrentCodec ("\370"); // small o, slash
		m_entityDecodeMap["otilde"]	= encodeWithCurrentCodec ("\365"); // small o, tilde
		m_entityDecodeMap["ouml"]	= encodeWithCurrentCodec ("\366"); // small o, dieresis or umlaut mark
		m_entityDecodeMap["szlig"]	= encodeWithCurrentCodec ("\337"); // small sharp s, German (sz ligature)
		m_entityDecodeMap["thorn"]	= encodeWithCurrentCodec ("\376"); // small thorn, Icelandic
		m_entityDecodeMap["uacute"]	= encodeWithCurrentCodec ("\372"); // small u, acute accent
		m_entityDecodeMap["ucirc"]	= encodeWithCurrentCodec ("\373"); // small u, circumflex accent
		m_entityDecodeMap["ugrave"]	= encodeWithCurrentCodec ("\371"); // small u, grave accent
		m_entityDecodeMap["uuml"]	= encodeWithCurrentCodec ("\374"); // small u, dieresis or umlaut mark
		m_entityDecodeMap["yacute"]	= encodeWithCurrentCodec ("\375"); // small y, acute accent
		m_entityDecodeMap["yuml"]	= encodeWithCurrentCodec ("\377"); // small y, dieresis or umlaut mark

		m_entityDecodeMap["iexcl"]	= encodeWithCurrentCodec ("\241");
		m_entityDecodeMap["cent"]	= encodeWithCurrentCodec ("\242");
		m_entityDecodeMap["pound"]	= encodeWithCurrentCodec ("\243");
		m_entityDecodeMap["curren"]	= encodeWithCurrentCodec ("\244");
		m_entityDecodeMap["yen"]	= encodeWithCurrentCodec ("\245");
		m_entityDecodeMap["brvbar"]	= encodeWithCurrentCodec ("\246");
		m_entityDecodeMap["sect"]	= encodeWithCurrentCodec ("\247");
		m_entityDecodeMap["uml"]	= encodeWithCurrentCodec ("\250");
		m_entityDecodeMap["ordf"]	= encodeWithCurrentCodec ("\252");
		m_entityDecodeMap["laquo"]	= encodeWithCurrentCodec ("\253");
		m_entityDecodeMap["not"]	= encodeWithCurrentCodec ("\254");
		m_entityDecodeMap["shy"]	= encodeWithCurrentCodec ("\255");
		m_entityDecodeMap["macr"]	= encodeWithCurrentCodec ("\257");
		m_entityDecodeMap["deg"]	= encodeWithCurrentCodec ("\260");
		m_entityDecodeMap["plusmn"]	= encodeWithCurrentCodec ("\261");
		m_entityDecodeMap["sup1"]	= encodeWithCurrentCodec ("\271");
		m_entityDecodeMap["sup2"]	= encodeWithCurrentCodec ("\262");
		m_entityDecodeMap["sup3"]	= encodeWithCurrentCodec ("\263");
		m_entityDecodeMap["acute"]	= encodeWithCurrentCodec ("\264");
		m_entityDecodeMap["micro"]	= encodeWithCurrentCodec ("\265");
		m_entityDecodeMap["para"]	= encodeWithCurrentCodec ("\266");
		m_entityDecodeMap["middot"]	= encodeWithCurrentCodec ("\267");
		m_entityDecodeMap["cedil"]	= encodeWithCurrentCodec ("\270");
		m_entityDecodeMap["ordm"]	= encodeWithCurrentCodec ("\272");
		m_entityDecodeMap["raquo"]	= encodeWithCurrentCodec ("\273");
		m_entityDecodeMap["frac14"]	= encodeWithCurrentCodec ("\274");
		m_entityDecodeMap["frac12"]	= encodeWithCurrentCodec ("\275");
		m_entityDecodeMap["frac34"]	= encodeWithCurrentCodec ("\276");
		m_entityDecodeMap["iquest"]	= encodeWithCurrentCodec ("\277");
		m_entityDecodeMap["times"]	= encodeWithCurrentCodec ("\327");
		m_entityDecodeMap["divide"]	= encodeWithCurrentCodec ("\367");

 		m_entityDecodeMap["copy"]	= encodeWithCurrentCodec ("\251"); // copyright sign
		m_entityDecodeMap["reg"]	= encodeWithCurrentCodec ("\256"); // registered sign
		m_entityDecodeMap["nbsp"]	= encodeWithCurrentCodec ("\240"); // non breaking space

		m_entityDecodeMap["rsquo"]	= QChar((unsigned short) 8217);
		m_entityDecodeMap["rdquo"]	= QChar((unsigned short) 8221);
		m_entityDecodeMap["trade"]  = QChar((unsigned short) 8482);
		m_entityDecodeMap["ldquo"]  = QChar((unsigned short) 8220);
		m_entityDecodeMap["mdash"]  = QChar((unsigned short) 8212);
				
		m_entityDecodeMap["amp"]	= "&";	// ampersand
		m_entityDecodeMap["gt"] = ">";	// greater than
		m_entityDecodeMap["lt"] = "<"; 	// less than
		m_entityDecodeMap["quot"] = "\""; // double quote
		m_entityDecodeMap["apos"] = "'"; 	// single quote
	}
	int qbegin = tag.indexOf ('"', offset);
	if ( qbegin == -1 )
		qFatal ("CHMFile::findStringInQuotes: cannot find first quote in <param> tag: '%s'", tag.toAscii().constData());

	int qend = firstquote ? tag.indexOf ('"', qbegin + 1) : tag.lastIndexOf ('"');

	if ( qend == -1 || qend <= qbegin )
		qFatal ("CHMFile::findStringInQuotes: cannot find last quote in <param> tag: '%s'", tag.toAscii().constData());

	// If we do not need to decode HTML entities, just return.
	if ( decodeentities )
	{
		QString htmlentity = QString::null;
		bool fill_entity = false;
	
		value.reserve (qend - qbegin); // to avoid multiple memory allocations
	
		for ( int i = qbegin + 1; i < qend; i++ )
		{
			if ( !fill_entity )
			{
				if ( tag[i] == '&' ) // HTML entity starts
					fill_entity = true;
				else
					value.append (tag[i]);
			}
			else
			{
				if ( tag[i] == ';' ) // HTML entity ends
				{
					QMap<QString, QString>::const_iterator it = m_entityDecodeMap.find (htmlentity);
					
					if ( it == m_entityDecodeMap.end() )
					{
						qWarning ("CHMFile::DecodeHTMLUnicodeEntity: could not decode HTML entity '%s', abort decoding.", htmlentity.toAscii().constData());
						break;
					}
	
					value.append (it.value());
					htmlentity = QString::null;
					fill_entity = false;
				}
				else
					htmlentity.append (tag[i]);
			}
		}
	}
	else
		value = tag.mid (qbegin + 1, qend - qbegin - 1);

	return qend + 1;
}


/*!
  Insert the \p url into the two maps \p UrlPage and \p PageUrl , taking care of
  checking if \p url is already in the maps and if the maps contain \p url
  without the <tt>#ref</tt> part of the url. Also increment \p num if the
  insertion is done.
  \note We suppose the two maps are kept in syncro.
 */
static void insertIntoUrlMaps(
	QMap <QString, int> &UrlPage, QMap <int,QString> &PageUrl,
	const QString &url, int &num )
{
	// url already there, abort insertion
	if ( UrlPage.contains(url) ) return;

	// check whether the map contains the url, stripped of the #ref part
	QString tmpurl = url;
	tmpurl.remove( QLatin1String("#")+KUrl("file:"+url).ref() );
	if ( UrlPage.contains(tmpurl) ) return;

	// insert the url into the maps, but insert always the variant without
	// the #ref part
	UrlPage.insert(tmpurl,num);
	PageUrl.insert(num,tmpurl);
	num++;
}


CHMFile::CHMFile()
	: m_chmFile(NULL), m_home("/")
{
	m_textCodec = 0;
	m_currentEncoding = 0;
	m_detectedLCID = 0;
	m_lookupTablesValid = false;
}


CHMFile::CHMFile(const QString& archiveName)
	: m_chmFile(NULL), m_home("/")
{
	LoadCHM(archiveName);
}


CHMFile::~CHMFile()
{
	CloseCHM();
}


bool CHMFile::LoadCHM(const QString&  archiveName)
{
	if(m_chmFile)
		CloseCHM();

	m_chmFile = chm_open (QFile::encodeName(archiveName));
	
	if(m_chmFile == NULL)
		return false;

	m_filename = archiveName;
	
	// Every CHM has its own encoding
	m_textCodec = 0;
	m_currentEncoding = 0;
	
	// Get information from /#WINDOWS and /#SYSTEM files (encoding, title, context file and so)
	InfoFromWindows();
	InfoFromSystem();

	guessTextEncoding();

	if ( ResolveObject("/#TOPICS", &m_chmTOPICS)
	&& ResolveObject("/#STRINGS", &m_chmSTRINGS)
	&& ResolveObject("/#URLTBL", &m_chmURLTBL)
	&& ResolveObject("/#URLSTR", &m_chmURLSTR) )
		m_lookupTablesValid = true;
	else
		m_lookupTablesValid = false;

	if ( m_lookupTablesValid && ResolveObject ("/$FIftiMain", &m_chmFIftiMain) )
		m_searchAvailable = true;
	else
		m_searchAvailable = false;
	
	return true;
}



void CHMFile::CloseCHM()
{
	if ( m_chmFile == NULL )
		return;

	chm_close(m_chmFile);
	
	m_chmFile = NULL;
	m_home = "/";
	m_filename = m_home = m_topicsFile = m_indexFile = m_font = QString::null;
	
	m_PageUrl.clear();
	m_UrlPage.clear();
	m_entityDecodeMap.clear();
	m_textCodec = 0;
	m_detectedLCID = 0;
	m_currentEncoding = 0;

	for ( chm_loaded_files_t::iterator it = m_chmLoadedFiles.begin(); it != m_chmLoadedFiles.end(); ++it )
		delete it.value();
}

/*
 * FIXME: <OBJECT type="text/sitemap"><param name="Merge" value="hhaxref.chm::/HHOCX_c.hhc"></OBJECT>
 *  (from htmlhelp.chm)
 */
bool CHMFile::ParseHhcAndFillTree (const QString& file, QDomDocument *tree, bool asIndex)
{
	chmUnitInfo ui;
	const int MAX_NEST_DEPTH = 256;

	if(file.isEmpty() || !ResolveObject(file, &ui))
		return false;

	QString src;
	GetFileContentAsString(src, &ui);

	if(src.isEmpty())
		return false;

	unsigned int defaultimagenum = asIndex ? KCHMImageType::IMAGE_INDEX : KCHMImageType::IMAGE_AUTO;
	unsigned int imagenum = defaultimagenum;
	int pos = 0, indent = 0;
    int pnum = 1;
	bool in_object = false, root_created = false;
 	bool add2treemap = asIndex ? false : m_PageUrl.isEmpty() ; // do not add to the map during index search
	QString name;
	QStringList urls;
	QDomNode * rootentry[MAX_NEST_DEPTH];
	QDomNode * lastchild[MAX_NEST_DEPTH];
	
	memset (lastchild, 0, sizeof(*lastchild));
	memset (rootentry, 0, sizeof(*rootentry));
	
	// Split the HHC file by HTML tags
	int stringlen = src.length();
	
	while ( pos < stringlen 
	&& (pos = src.indexOf ('<', pos)) != -1 )
	{
		int i, word_end = 0;
		
		for ( i = ++pos; i < stringlen; i++ )
		{
			// If a " or ' is found, skip to the next one.
			if ( (src[i] == '"' || src[i] == '\'') )
			{
				// find where quote ends, either by another quote, or by '>' symbol (some people don't know HTML)
				int nextpos = src.indexOf (src[i], i+1);
				if ( nextpos == -1 	&& (nextpos = src.indexOf ('>', i+1)) == -1 )
				{
					qWarning ("CHMFile::ParseHhcAndFillTree: corrupted TOC: %s", src.mid(i).toAscii().constData());
					return false;
				}

				i =  nextpos;
			}
			else if ( src[i] == '>'  )
				break;
			else if ( !src[i].isLetterOrNumber() && src[i] != '/' && !word_end )
				word_end = i;
		}
		
		QString tagword, tag = src.mid (pos, i - pos);
		 
		if ( word_end )
			tagword = src.mid (pos, word_end - pos).toLower();
		else
			tagword = tag.toLower();

// qDebug ("tag: '%s', tagword: '%s'\n", tag.toAscii().constData(), tagword.toAscii().constData());
	 					
		// <OBJECT type="text/sitemap"> - a topic entry
		if ( tagword == "object" && tag.indexOf ("text/sitemap", 0, Qt::CaseInsensitive) != -1 )
			in_object = true;
		else if ( tagword == "/object" && in_object ) 
		{
			// a topic entry closed. Add a tree item
			if ( !name.isNull() )
			{
				QDomElement * item = new QDomElement();

				if ( !root_created )
					indent = 0;

				QString url = urls.join ("|");

				// Add item into the tree
				if ( !indent )
				{
					*item = tree->createElement(name);
					item->setAttribute("ViewportName",url);
					item->setAttribute("Icon",imagenum);
                    tree->appendChild(*item);
				}
				else
				{
					if ( !rootentry[indent-1] )
						qFatal("CHMFile::ParseAndFillTopicsTree: child entry %d with no root entry!", indent-1);

					*item = tree->createElement(name);
					item->setAttribute("ViewportName",url);
					item->setAttribute("Icon",imagenum);
                    rootentry[indent-1]->appendChild(*item);
				}

				lastchild[indent] = item;

				if ( indent == 0 || !rootentry[indent] )
				{
					rootentry[indent] = item;
					root_created = true;
				}

				// There are no 'titles' in index file
				if ( add2treemap  )
				{
					insertIntoUrlMaps(m_UrlPage,m_PageUrl,url,pnum);
				}
			}
			else
			{
				if ( !urls.isEmpty() )
					qDebug ("CHMFile::ParseAndFillTopicsTree: <object> tag with url \"%s\" is parsed, but name is empty.", urls[0].toAscii().constData());
				else
					qDebug ("CHMFile::ParseAndFillTopicsTree: <object> tag is parsed, but both name and url are empty.");	
			}

			name = QString::null;
			urls.clear();
			in_object = false;
			imagenum = defaultimagenum;
		}
		else if ( tagword == "param" && in_object )
		{
			// <param name="Name" value="First Page">
			int offset; // strlen("param ")
			QString name_pattern = "name=", value_pattern = "value=";
			QString pname, pvalue;

			if ( (offset = tag.indexOf (name_pattern, 0, Qt::CaseInsensitive)) == -1 )
				qFatal ("CHMFile::ParseAndFillTopicsTree: bad <param> tag '%s': no name=\n", tag.toAscii().constData());

			// offset+5 skips 'name='
			offset = findStringInQuotes (tag, offset + name_pattern.length(), pname, true, false);
			pname = pname.toLower();

			if ( (offset = tag.indexOf (value_pattern, offset, Qt::CaseInsensitive)) == -1 )
				qFatal ("CHMFile::ParseAndFillTopicsTree: bad <param> tag '%s': no value=\n", tag.toAscii().constData());

			// offset+6 skips 'value='
			findStringInQuotes (tag, offset + value_pattern.length(), pvalue, false, true);

// qDebug ("<param>: name '%s', value '%s'", pname.toAscii().constData(), pvalue.toAscii().constData());

			if ( pname == "name" )
				name = pvalue;
			else if ( pname == "local" )
				urls.push_back (KCHMUrl::makeURLabsoluteIfNeeded(pvalue));
			else if ( pname == "see also" && asIndex && name != pvalue )
				urls.push_back (KCHMUrl::makeURLabsoluteIfNeeded(":" + pvalue));
			else if ( pname == "imagenumber" )
			{
				bool bok;
				int imgnum = pvalue.toInt (&bok);
	
				if ( bok && imgnum >= 0 && (unsigned) imgnum < MAX_BUILTIN_ICONS )
					imagenum = imgnum;
			}
		}
		else if ( tagword == "ul" ) // increase indent level
		{
			// Fix for buggy help files		
			if ( ++indent >= MAX_NEST_DEPTH )
				qFatal("CHMFile::ParseAndFillTopicsTree: max nest depth (%d) is reached, error in help file", MAX_NEST_DEPTH);
				
			lastchild[indent] = 0;
			rootentry[indent] = 0;
		}
		else if ( tagword == "/ul" ) // decrease indent level
		{
			if ( --indent < 0 )
				indent = 0;
				
			rootentry[indent] = 0;
		}

		pos = i;	
	}
	
	return true;
}

bool CHMFile::ParseHhcAndFillTree (const QString& file, K3ListView *tree, bool asIndex)
{
	chmUnitInfo ui;
	const int MAX_NEST_DEPTH = 256;

	if(file.isEmpty() || !ResolveObject(file, &ui))
		return false;

	QString src;
	GetFileContentAsString(src, &ui);

	if(src.isEmpty())
		return false;

	unsigned int defaultimagenum = asIndex ? KCHMImageType::IMAGE_INDEX : KCHMImageType::IMAGE_AUTO;
	unsigned int imagenum = defaultimagenum;
	int pos = 0, indent = 0;
	bool in_object = false, root_created = false;
	QString name;
	QStringList urls;
	KCHMMainTreeViewItem * rootentry[MAX_NEST_DEPTH];
	KCHMMainTreeViewItem * lastchild[MAX_NEST_DEPTH];
	
	memset (lastchild, 0, sizeof(*lastchild));
	memset (rootentry, 0, sizeof(*rootentry));
	
	// Split the HHC file by HTML tags
	int stringlen = src.length();
	
	while ( pos < stringlen 
	&& (pos = src.indexOf ('<', pos)) != -1 )
	{
		int i, word_end = 0;
		
		for ( i = ++pos; i < stringlen; i++ )
		{
			// If a " or ' is found, skip to the next one.
			if ( (src[i] == '"' || src[i] == '\'') )
			{
				// find where quote ends, either by another quote, or by '>' symbol (some people don't know HTML)
				int nextpos = src.indexOf (src[i], i+1);
				if ( nextpos == -1 	&& (nextpos = src.indexOf ('>', i+1)) == -1 )
				{
					qWarning ("CHMFile::ParseHhcAndFillTree: corrupted TOC: %s", src.mid(i).toAscii().constData());
					return false;
				}

				i =  nextpos;
			}
			else if ( src[i] == '>'  )
				break;
			else if ( !src[i].isLetterOrNumber() && src[i] != '/' && !word_end )
				word_end = i;
		}
		
		QString tagword, tag = src.mid (pos, i - pos);
		 
		if ( word_end )
			tagword = src.mid (pos, word_end - pos).toLower();
		else
			tagword = tag.toLower();

//qDebug ("tag: '%s', tagword: '%s'\n", tag.toAscii().constData(), tagword.toAscii().constData());
						
		// <OBJECT type="text/sitemap"> - a topic entry
		if ( tagword == "object" && tag.indexOf ("text/sitemap", 0, Qt::CaseInsensitive) != -1 )
			in_object = true;
		else if ( tagword == "/object" && in_object ) 
		{
			// a topic entry closed. Add a tree item
			if ( !name.isNull() )
			{
				KCHMMainTreeViewItem * item;

				if ( !root_created )
					indent = 0;

				QString url = urls.join ("|");

				// Add item into the tree
				if ( !indent )
				{
					item = new KCHMMainTreeViewItem (tree, lastchild[indent], name, url, imagenum);
				}
				else
				{
					if ( !rootentry[indent-1] )
						qFatal("CHMFile::ParseAndFillTopicsTree: child entry %d with no root entry!", indent-1);

					item = new KCHMMainTreeViewItem (rootentry[indent-1], lastchild[indent], name, url,  imagenum);
				}

				lastchild[indent] = item;

				if ( indent == 0 || !rootentry[indent] )
				{
					rootentry[indent] = item;
					root_created = true;

					if ( asIndex  )
						rootentry[indent]->setOpen(true);
				}
			}
			else
			{
				if ( !urls.isEmpty() )
					qDebug ("CHMFile::ParseAndFillTopicsTree: <object> tag with url \"%s\" is parsed, but name is empty.", urls[0].toAscii().constData());
				else
					qDebug ("CHMFile::ParseAndFillTopicsTree: <object> tag is parsed, but both name and url are empty.");	
			}

			name = QString::null;
			urls.clear();
			in_object = false;
			imagenum = defaultimagenum;
		}
		else if ( tagword == "param" && in_object )
		{
			// <param name="Name" value="First Page">
			int offset; // strlen("param ")
			QString name_pattern = "name=", value_pattern = "value=";
			QString pname, pvalue;

			if ( (offset = tag.indexOf (name_pattern, 0, Qt::CaseInsensitive)) == -1 )
				qFatal ("CHMFile::ParseAndFillTopicsTree: bad <param> tag '%s': no name=\n", tag.toAscii().constData());

			// offset+5 skips 'name='
			offset = findStringInQuotes (tag, offset + name_pattern.length(), pname, true, false);
			pname = pname.toLower();

			if ( (offset = tag.indexOf (value_pattern, offset, Qt::CaseInsensitive)) == -1 )
				qFatal ("CHMFile::ParseAndFillTopicsTree: bad <param> tag '%s': no value=\n", tag.toAscii().constData());

			// offset+6 skips 'value='
			findStringInQuotes (tag, offset + value_pattern.length(), pvalue, false, true);

//qDebug ("<param>: name '%s', value '%s'", pname.toAscii().constData(), pvalue.toAscii().constData());

			if ( pname == "name" )
				name = pvalue;
			else if ( pname == "local" )
				urls.push_back (KCHMUrl::makeURLabsoluteIfNeeded (pvalue));
			else if ( pname == "see also" && asIndex && name != pvalue )
				urls.push_back (":" + pvalue);
			else if ( pname == "imagenumber" )
			{
				bool bok;
				int imgnum = pvalue.toInt (&bok);
	
				if ( bok && imgnum >= 0 && (unsigned) imgnum < MAX_BUILTIN_ICONS )
					imagenum = imgnum;
			}
		}
		else if ( tagword == "ul" ) // increase indent level
		{
			// Fix for buggy help files		
			if ( ++indent >= MAX_NEST_DEPTH )
				qFatal("CHMFile::ParseAndFillTopicsTree: max nest depth (%d) is reached, error in help file", MAX_NEST_DEPTH);
				
			lastchild[indent] = 0;
			rootentry[indent] = 0;
		}
		else if ( tagword == "/ul" ) // decrease indent level
		{
			if ( --indent < 0 )
				indent = 0;
				
			rootentry[indent] = 0;
		}

		pos = i;	
	}
	
	return true;
}


bool CHMFile::ParseAndFillTopicsTree(QDomDocument *tree)
{
	return ParseHhcAndFillTree (m_topicsFile, tree, false);
}

bool CHMFile::ParseAndFillIndex(K3ListView *indexlist)
{
	return ParseHhcAndFillTree (m_indexFile, indexlist, true);
}

bool CHMFile::SearchWord (const QString& text, bool wholeWords, bool titlesOnly, KCHMSearchProgressResults_t& results,  bool phrase_search)
{
	bool partial = false;

	if ( text.isEmpty() || !m_searchAvailable )
		return false;

	QString searchword = (QString) convertSearchWord (text);

#define FTS_HEADER_LEN 0x32
	unsigned char header[FTS_HEADER_LEN];

	if ( RetrieveObject (&m_chmFIftiMain, header, 0, FTS_HEADER_LEN) == 0 )
		return false;
	
	unsigned char doc_index_s = header[0x1E], doc_index_r = header[0x1F];
	unsigned char code_count_s = header[0x20], code_count_r = header[0x21];
	unsigned char loc_codes_s = header[0x22], loc_codes_r = header[0x23];

	if(doc_index_s != 2 || code_count_s != 2 || loc_codes_s != 2)
	{
		// Don't know how to use values other than 2 yet. Maybe next chmspec.
		return false;
	}

	unsigned char* cursor32 = header + 0x14;
	u_int32_t node_offset = UINT32ARRAY(cursor32);

	cursor32 = header + 0x2e;
	u_int32_t node_len = UINT32ARRAY(cursor32);

	unsigned char* cursor16 = header + 0x18;
	u_int16_t tree_depth = UINT16ARRAY(cursor16);

	unsigned char word_len, pos;
	QString word;
	u_int32_t i = sizeof(u_int16_t);
	u_int16_t free_space;

	QVector<unsigned char> buffer(node_len);

	node_offset = GetLeafNodeOffset (searchword, node_offset, node_len, tree_depth);

	if ( !node_offset )
		return false;

	do
	{
		// got a leaf node here.
		if ( RetrieveObject (&m_chmFIftiMain, buffer.data(), node_offset, node_len) == 0 )
			return false;

		cursor16 = buffer.data() + 6;
		free_space = UINT16ARRAY(cursor16);

		i = sizeof(u_int32_t) + sizeof(u_int16_t) + sizeof(u_int16_t);
		u_int64_t wlc_count, wlc_size;
		u_int32_t wlc_offset;

		while (i < node_len - free_space)
		{
			word_len = *(buffer.data() + i);
			pos = *(buffer.data() + i + 1);

			char *wrd_buf = new char[word_len];
			memcpy (wrd_buf, buffer.data() + i + 2, word_len - 1);
			wrd_buf[word_len - 1] = 0;

			if ( pos == 0 )
				word = wrd_buf;
			else
				word = word.mid (0, pos) + wrd_buf;

			delete[] wrd_buf;

			i += 2 + word_len;
			unsigned char title = *(buffer.data() + i - 1);

			size_t encsz;
			wlc_count = be_encint (buffer.data() + i, encsz);
			i += encsz;
		
			cursor32 = buffer.data() + i;
			wlc_offset = UINT32ARRAY(cursor32);

			i += sizeof(u_int32_t) + sizeof(u_int16_t);
			wlc_size =  be_encint (buffer.data() + i, encsz);
			i += encsz;

			cursor32 = buffer.data();
			node_offset = UINT32ARRAY(cursor32);
		
			if ( !title && titlesOnly )
				continue;

			if ( wholeWords && searchword == word )
				return ProcessWLC(wlc_count, wlc_size,
						  wlc_offset, doc_index_s,
						  doc_index_r,code_count_s,
						  code_count_r, loc_codes_s,
						  loc_codes_r, results, phrase_search);

			if ( !wholeWords )
			{
				if ( word.startsWith (searchword))
				{
					partial = true;
					
					ProcessWLC(wlc_count, wlc_size,
						   wlc_offset, doc_index_s,
						   doc_index_r,code_count_s,
						   code_count_r, loc_codes_s,
						   loc_codes_r, results, phrase_search);

				}
				else if ( QString::compare (searchword, word.mid(0, searchword.length())) < -1 )
					break;
			}
		}	
	}
	while ( !wholeWords && word.startsWith (searchword) && node_offset );
	
	return partial;
}


bool CHMFile::ResolveObject(const QString& fileName, chmUnitInfo *ui)
{
	return m_chmFile != NULL 
	&& ::chm_resolve_object(m_chmFile, fileName.toAscii().constData(), ui) ==
CHM_RESOLVE_SUCCESS;
}


size_t CHMFile::RetrieveObject(chmUnitInfo *ui, unsigned char *buffer,
							   LONGUINT64 fileOffset, LONGINT64 bufferSize)
{
	return ::chm_retrieve_object(m_chmFile, ui, buffer, fileOffset,
								 bufferSize);
}


inline u_int32_t CHMFile::GetLeafNodeOffset(const QString& text,
											 u_int32_t initialOffset,
											 u_int32_t buffSize,
											 u_int16_t treeDepth)
{
	u_int32_t test_offset = 0;
	unsigned char* cursor16, *cursor32;
	unsigned char word_len, pos;
	u_int32_t i = sizeof(u_int16_t);
	QVector<unsigned char> buffer(buffSize);
	QString word;
	
	while(--treeDepth)
	{
		if ( initialOffset == test_offset )
			return 0;

		test_offset = initialOffset;
		if ( RetrieveObject (&m_chmFIftiMain, buffer.data(), initialOffset, buffSize) == 0 )
			return 0;

		cursor16 = buffer.data();
		u_int16_t free_space = UINT16ARRAY(cursor16);

		while (i < buffSize - free_space )
		{
			word_len = *(buffer.data() + i);
			pos = *(buffer.data() + i + 1);

			char *wrd_buf = new char[word_len];
			memcpy ( wrd_buf, buffer.data() + i + 2, word_len - 1 );
			wrd_buf[word_len - 1] = 0;

			if ( pos == 0 )
				word = wrd_buf;
			else
				word = word.mid(0, pos) + wrd_buf;

			delete[] wrd_buf;

			if ( text <= word )
			{
				cursor32 = buffer.data() + i + word_len + 1;
				initialOffset = UINT32ARRAY(cursor32);
				break;
			}

			i += word_len + sizeof(unsigned char) +
				sizeof(u_int32_t) + sizeof(u_int16_t);
		}
	}

	if ( initialOffset == test_offset )
		return 0;

	return initialOffset;
}


inline bool CHMFile::ProcessWLC (u_int64_t wlc_count, u_int64_t wlc_size,
								u_int32_t wlc_offset, unsigned char ds,
								unsigned char dr, unsigned char cs,
								unsigned char cr, unsigned char ls,
								unsigned char lr,
								KCHMSearchProgressResults_t& results,
								bool phrase_search)
{
	int wlc_bit = 7;
	u_int64_t index = 0, count;
	size_t length, off = 0;
	QVector<unsigned char> buffer (wlc_size);
	unsigned char *cursor32;

	unsigned char entry[TOPICS_ENTRY_LEN];
	unsigned char combuf[13];

	if ( RetrieveObject (&m_chmFIftiMain, buffer.data(), wlc_offset, wlc_size) == 0 )
		return false;

	for ( u_int64_t i = 0; i < wlc_count; ++i )
	{
		if ( wlc_bit != 7 )
		{
			++off;
			wlc_bit = 7;
		}

		index += sr_int (buffer.data() + off, &wlc_bit, ds, dr, length);
		off += length;

		if ( RetrieveObject (&m_chmTOPICS, entry, index * 16, TOPICS_ENTRY_LEN) == 0 )
			return false;

		KCHMSearchProgressResult progres;

		cursor32 = entry + 4;
		progres.titleoff = UINT32ARRAY(cursor32);

		cursor32 = entry + 8;
		progres.urloff = UINT32ARRAY(cursor32);

		if ( RetrieveObject (&m_chmURLTBL, combuf, progres.urloff, 12) == 0 )
			return false;

		cursor32 = combuf + 8;
		progres.urloff = UINT32ARRAY (cursor32);

		count = sr_int (buffer.data() + off, &wlc_bit, cs, cr, length);
		off += length;

		if ( phrase_search )
			progres.offsets.reserve (count);
		
		for (u_int64_t j = 0; j < count; ++j)
		{
			u_int64_t lcode = sr_int (buffer.data() + off, &wlc_bit, ls, lr, length);
			off += length;
			
			if ( phrase_search )
				progres.offsets.push_back (lcode);
		}
		
		results.push_back (progres);
	}

	return true;
}


inline bool CHMFile::InfoFromWindows()
{
	#define WIN_HEADER_LEN 0x08
	unsigned char buffer[BUF_SIZE];
	unsigned int factor;
	chmUnitInfo ui;
	long size = 0;

	if ( ResolveObject("/#WINDOWS", &ui) )
	{
		if ( !RetrieveObject(&ui, buffer, 0, WIN_HEADER_LEN) )
			return false;

		u_int32_t entries = *(u_int32_t *)(buffer);
		FIXENDIAN32(entries);
		u_int32_t entry_size = *(u_int32_t *)(buffer + 0x04);
		FIXENDIAN32(entry_size);
		
		QByteArray uptr(entries * entry_size);
		unsigned char* raw = (unsigned char*) uptr.data();
		
		if ( !RetrieveObject (&ui, raw, 8, entries * entry_size) )
			return false;

		if( !ResolveObject ("/#STRINGS", &ui) )
			return false;

		for ( u_int32_t i = 0; i < entries; ++i )
		{
			u_int32_t offset = i * entry_size;
			
			u_int32_t off_title = *(u_int32_t *)(raw + offset + 0x14);
			FIXENDIAN32(off_title);

			u_int32_t off_home = *(u_int32_t *)(raw + offset + 0x68);
			FIXENDIAN32(off_home);

			u_int32_t off_hhc = *(u_int32_t *)(raw + offset + 0x60);
			FIXENDIAN32(off_hhc);
			
			u_int32_t off_hhk = *(u_int32_t *)(raw + offset + 0x64);
			FIXENDIAN32(off_hhk);

			factor = off_title / 4096;

			if ( size == 0 ) 
				size = RetrieveObject(&ui, buffer, factor * 4096, BUF_SIZE);

			if ( size && off_title )
				m_title = QString ((const char*) (buffer + off_title % 4096));

			if ( factor != off_home / 4096)
			{
				factor = off_home / 4096;		
				size = RetrieveObject (&ui, buffer, factor * 4096, BUF_SIZE);
			}
			
			if ( size && off_home )
				m_home = QString("/") + QString( (const char*) buffer + off_home % 4096);

			if ( factor != off_hhc / 4096)
			{
				factor = off_hhc / 4096;
				size = RetrieveObject(&ui, buffer, factor * 4096, BUF_SIZE);
			}
		
			if ( size && off_hhc )
				m_topicsFile = QString("/") + QString ((const char*) buffer + off_hhc % 4096);

			if ( factor != off_hhk / 4096)
			{
				factor = off_hhk / 4096;
				size = RetrieveObject (&ui, buffer, factor * 4096, BUF_SIZE);
			}

			if ( size && off_hhk )
				m_indexFile = QString("/") + QString((const char*) buffer + off_hhk % 4096);
		}
	}
	return true;
}



inline bool CHMFile::InfoFromSystem()
{
	unsigned char buffer[BUF_SIZE];
	chmUnitInfo ui;
	
	int index = 0;
	unsigned char* cursor = NULL;
	u_int16_t value = 0;

	long size = 0;

	// Run the first loop to detect the encoding. We need this, because title could be
	// already encoded in user encoding. Same for file names
	if ( !ResolveObject ("/#SYSTEM", &ui) )
		return false;

	// Can we pull BUFF_SIZE bytes of the #SYSTEM file?
	if ( (size = RetrieveObject (&ui, buffer, 4, BUF_SIZE)) == 0 )
		return false;

	buffer[size - 1] = 0;

	// First loop to detect the encoding
	for ( index = 0; index < (size - 1 - (long)sizeof(u_int16_t)) ;)
	{
		cursor = buffer + index;
		value = UINT16ARRAY(cursor);

		switch(value)
		{
		case 0:
			index += 2;
			cursor = buffer + index;
			
			if(m_topicsFile.isEmpty())
				m_topicsFile = QString("/") + QString((const char*) buffer + index + 2);
				
			break;
			
		case 1:
			index += 2;
			cursor = buffer + index;

			if(m_indexFile.isEmpty())
				m_indexFile = QString("/") + QString ((const char*)buffer + index + 2);
			break;
		
		case 2:
			index += 2;
			cursor = buffer + index;
				
			if(m_home.isEmpty() || m_home == "/")
				m_home = QString("/") + QString ((const char*) buffer + index + 2);
			break;
			
		case 3:
			index += 2;
			cursor = buffer + index;
			m_title = QString((const char*) (buffer + index + 2));
			break;

		case 4:
			index += 2;
			cursor = buffer + index;

			m_detectedLCID = (short) *((unsigned int*) (buffer + index + 2));
			break;

		case 6:
			index += 2;
			cursor = buffer + index;

			if(m_topicsFile.isEmpty()) {
				QString topicAttempt = "/", tmp;
				topicAttempt += QString ((const char*) buffer +index +2);

				tmp = topicAttempt + ".hhc";
				
				if ( ResolveObject (tmp.toAscii().constData(), &ui) )
					m_topicsFile = tmp;

				tmp = topicAttempt + ".hhk";
				
				if ( ResolveObject(tmp.toAscii().constData(), &ui) )
					m_indexFile = tmp;
			}
			break;

		case 16:
			index += 2;
			cursor = buffer + index;

			m_font = QString ((const char*) buffer + index + 2);
			break;
			
		default:
			index += 2;
			cursor = buffer + index;
		}

		value = UINT16ARRAY(cursor);
		index += value + 2;
	}
	
	return true;
}


int CHMFile::getPageNum( const QString & str ) const
{
	QMap<QString,int>::const_iterator i=m_UrlPage.find (str);
	if ( i == m_UrlPage.end() )
		return 0;
	return i.value();
}

QString CHMFile::getUrlForPage( int p ) const
{
	QMap<int,QString>::const_iterator i=m_PageUrl.find (p);
	if ( i == m_PageUrl.end() )
		return 0;
	return i.value();
}

bool CHMFile::guessTextEncoding( )
{
	const KCHMTextEncoding::text_encoding_t * enc = 0;

/*
 * Skip encoding by font family; detect encoding by LCID seems to be more reliable
	// First try 'by font'
	int i, charset;
		
	if ( !m_font.isEmpty() )
	{
		if ( (i = m_font.findRev (',')) != -1
		&& (charset = m_font.mid (i+1).toUInt()) != 0 )
			enc = KCHMTextEncoding::lookupByWinCharset(charset);
	}

	// The next step - detect by LCID
	if ( !enc && m_detectedLCID )
		enc = KCHMTextEncoding::lookupByLCID (m_detectedLCID);
*/

	if ( !m_detectedLCID
	|| (enc = KCHMTextEncoding::lookupByLCID (m_detectedLCID)) == 0 )
		qFatal ("Could not detect text encoding by LCID");
	
	if ( changeFileEncoding (enc->qtcodec) )
	{
		m_currentEncoding = enc;
//		mainWindow->showInStatusBar (QString("Detected help file charset: ") +  enc->charset);
		return true;
	}
	
	return false;
}

bool CHMFile::changeFileEncoding( const char * qtencoding )
{
	// Set up encoding
	m_textCodec = QTextCodec::codecForName (qtencoding);
	
	if ( !m_textCodec )
	{
		qWarning ("Could not set up Text Codec for encoding '%s'", qtencoding);
		return false;
	}
	
	m_entityDecodeMap.clear();
	return true;
}


bool CHMFile::setCurrentEncoding( const KCHMTextEncoding::text_encoding_t * enc )
{
	m_currentEncoding = enc;
	return changeFileEncoding (enc->qtcodec);
}


bool CHMFile::GetFileContentAsString(QString& str, chmUnitInfo *ui)
{
	QByteArray buf (ui->length + 1);
			
	if ( RetrieveObject (ui, (unsigned char*) buf.data(), 0, ui->length) )
	{
		buf [(int)ui->length] = '\0';
		str = encodeWithCurrentCodec((const char*) buf);
		return true;
	}
	else
	{
		str = QString::null;
		return false;
	}
}


bool CHMFile::GetFileContentAsString( QString & str, QString filename, QString location )
{
	str = QString::null;

	if ( m_filename == filename )
		return GetFileContentAsString (str, location);

	// Load a file if it is not already loaded
	CHMFile * file = getCHMfilePointer (filename);

	if ( !file )
		return false;

	return file->GetFileContentAsString (str, location);
}


bool CHMFile::GetFileContentAsString (QString& str, QString location)
{
	chmUnitInfo ui;

	if( !ResolveObject(location, &ui) )
		return false;
		
	return GetFileContentAsString(str, &ui);
}


CHMFile * CHMFile::getCHMfilePointer( const QString & filename )
{
	if ( m_filename == filename )
		return this;

	// Load a file if it is not already loaded
	if ( m_chmLoadedFiles.find (filename) == m_chmLoadedFiles.end() )
	{
		CHMFile * newfile = new CHMFile;

		if ( !newfile->LoadCHM (filename) )
		{
			delete newfile;
			return 0;
		}

		m_chmLoadedFiles[filename] = newfile;
	}

	return m_chmLoadedFiles[filename];
}


static int chm_enumerator_callback (struct chmFile*, struct chmUnitInfo *ui, void *context)
{
	((QVector<QString> *) context)->push_back (ui->path);
    return CHM_ENUMERATOR_CONTINUE;
}

bool CHMFile::enumerateArchive( QVector< QString > & files )
{
	files.clear();
	return chm_enumerate (m_chmFile, CHM_ENUMERATE_ALL, chm_enumerator_callback, &files);
}

QByteArray CHMFile::convertSearchWord( const QString & src )
{
	static const char * searchwordtable[128] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "s", 0, "oe", 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "s", 0, "oe", 0, 0, "y",
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		"a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",
		"d", "n", "o", "o", "o", "o", "o", 0, "o", "u", "u", "u", "u", "y", "\xDE", "ss",
		"a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",
		"o", "n", "o", "o", "o", "o", "o", 0, "o", "u", "u", "u", "u", "y", "\xFE", "y"
	};

	if ( !m_textCodec )
		return src.toLower().toLocal8Bit();

	QByteArray dest = m_textCodec->fromUnicode (src);

	for ( int i = 0; i < dest.size(); i++ )
	{
		if ( dest[i] & 0x80 )
		{
			int index = dest[i] & 0x7F;
			if ( searchwordtable[index] )
				dest.replace (i, 1, searchwordtable[index]);
			else
				dest.remove (i, 1);
		}
	}

	return dest.toLower();
}



QString CHMFile::getTopicByUrl( const QString & search_url )
{
	if ( !m_lookupTablesValid )
		return QString::null;

	unsigned char buf[COMMON_BUF_LEN];
	int pos = search_url.indexOf ('#');
	QString fixedurl = pos == -1 ? search_url : search_url.left (pos);

	for ( unsigned int i = 0; i < m_chmTOPICS.length; i += TOPICS_ENTRY_LEN )
	{
		if ( RetrieveObject ( &m_chmTOPICS, buf, i, TOPICS_ENTRY_LEN) == 0 )
			return QString::null;

		u_int32_t off_title = *(u_int32_t *)(buf + 4);
		FIXENDIAN32(off_title);

		u_int32_t off_url = *(u_int32_t *)(buf + 8);
		FIXENDIAN32(url);

		QString topic, url;

		if ( RetrieveObject ( &m_chmURLTBL, buf, off_url, URLTBL_ENTRY_LEN) == 0 )
			return QString::null;

		off_url = *(u_int32_t *)(buf + 8);
		FIXENDIAN32(off_url);

		if ( RetrieveObject ( &m_chmURLSTR, buf, off_url + 8, sizeof(buf) - 1) == 0 )
			return false;

		buf[sizeof(buf) - 1] = '\0';
//		url = KCHMViewWindow::makeURLabsoluteIfNeeded ((const char*)buf);
        url = ((const char*)buf);
		if ( url != fixedurl )
			continue;

		if ( RetrieveObject ( &m_chmSTRINGS, buf, off_title, sizeof(buf) - 1) != 0 )
		{
			buf[sizeof(buf) - 1] = '\0';
			topic = encodeWithCurrentCodec ((const char*)buf);
		}
		else
			topic = "Untitled";

		return topic;
	}

	return QString::null;
}

void CHMFile::GetSearchResults( const KCHMSearchProgressResults_t & tempres, KCHMSearchResults_t & results, unsigned int limit_results )
{
	unsigned char combuf [COMMON_BUF_LEN];
	QMap<u_int32_t, u_int32_t> urlsmap;  // used to prevent duplicated urls
	
	for ( int i = 0; i < tempres.size(); i++ )
	{
		if ( urlsmap.find (tempres[i].urloff) != urlsmap.end() )
			continue;
		
		urlsmap[tempres[i].urloff] = 1;
		
		KCHMSearchResult res;
		
		if ( RetrieveObject (&m_chmSTRINGS, combuf, tempres[i].titleoff, COMMON_BUF_LEN - 1) != 0 )
		{
			combuf[COMMON_BUF_LEN - 1] = 0;
			res.title = encodeWithCurrentCodec ((const char*)combuf);
		}
		else
			res.title = "Untitled";

		if ( RetrieveObject (&m_chmURLSTR, combuf, tempres[i].urloff + 8, COMMON_BUF_LEN - 1) == 0 )
			continue;

		combuf[COMMON_BUF_LEN - 1] = 0;
// 		res.url = KCHMViewWindow::makeURLabsoluteIfNeeded ((const char*) combuf);
		res.url = ((const char*)combuf);
		results.push_back (res);
		
		if ( --limit_results == 0 )
			break;
	}
}

/*
bool CHMFile::ParseChmIndexFile ( const QString& file, bool asIndex, KCHMParsedIndexEntry_t & cont )
{
	QString src;
	const int MAX_NEST_DEPTH = 256;

	if ( file.isEmpty() || !GetFileContentAsString ( src, file ) )
		return false;

	unsigned int defaultimagenum = asIndex ? KCHMImageType::IMAGE_INDEX : KCHMImageType::IMAGE_AUTO;
	unsigned int imagenum = defaultimagenum;
	int pos = 0, indent = 0;
	bool in_object = false, root_created = false;
	QString name;
	QStringList urls;
	KCHMParsedIndexEntry * rootentry[MAX_NEST_DEPTH];
	
	memset (rootentry, 0, sizeof(*rootentry));
	
	// Split the HHC file by HTML tags
	int stringlen = src.length();
	
	while ( pos < stringlen && (pos = src.find ('<', pos)) != -1 )
	{
		int i, word_end = 0;
		
		for ( i = ++pos; i < stringlen; i++ )
		{
			// If a " or ' is found, skip to the next one.
			if ( (src[i] == '"' || src[i] == '\'') )
			{
				// find where quote ends, either by another quote, or by '>' symbol (some people don't know HTML)
				int nextpos = src.find ( src[i], i+1 );
				if ( nextpos == -1 	&& ( nextpos = src.find ('>', i+1) ) == -1 )
				{
					qWarning ("CHMFile::ParseHhcAndFillTree: corrupted TOC: %s", src.mid(i).toAscii().constData());
					return false;
				}

				i =  nextpos;
			}
			else if ( src[i] == '>'  )
				break;
			else if ( !src[i].isLetterOrNumber() && src[i] != '/' && !word_end )
				word_end = i;
		}
		
		QString tagword, tag = src.mid (pos, i - pos);
		 
		if ( word_end )
			tagword = src.mid (pos, word_end - pos).toLower();
		else
			tagword = tag.toLower();

//qDebug ("tag: '%s', tagword: '%s'\n", tag.toAscii().constData(), tagword.toAscii().constData());
						
		// <OBJECT type="text/sitemap"> - a topic entry
		if ( tagword == "object" && tag.find ("text/sitemap", 0, false) != -1 )
			in_object = true;
		else if ( tagword == "/object" && in_object ) 
		{
			// a topic entry closed. Add a tree item
			if ( name )
			{
				KCHMParsedIndexEntry * item;

				if ( !root_created )
					indent = 0;

				QString url = urls.join ("|");

				// Add item into the tree
				if ( !indent )
				{
					item = new KCHMParsedIndexEntry (name, 0, url, imagenum);
				}
				else
				{
					if ( !rootentry[indent-1] )
						qFatal("CHMFile::ParseAndFillTopicsTree: child entry %d with no root entry!", indent-1);

					item = new KCHMParsedIndexEntry (name, rootentry[indent-1], url, imagenum);
				}

				if ( indent == 0 || !rootentry[indent] )
				{
					rootentry[indent] = item;
					root_created = true;
				}
			}
			else
			{
				if ( !urls.isEmpty() )
					qDebug ("CHMFile::ParseAndFillTopicsTree: <object> tag with url \"%s\" is parsed, but name is empty.", urls[0].toAscii().constData());
				else
					qDebug ("CHMFile::ParseAndFillTopicsTree: <object> tag is parsed, but both name and url are empty.");	
			}

			name = QString::null;
			urls.clear();
			in_object = false;
			imagenum = defaultimagenum;
		}
		else if ( tagword == "param" && in_object )
		{
			// <param name="Name" value="First Page">
			int offset; // strlen("param ")
			QString name_pattern = "name=", value_pattern = "value=";
			QString pname, pvalue;

			if ( (offset = tag.find (name_pattern, 0, false)) == -1 )
				qFatal ("CHMFile::ParseAndFillTopicsTree: bad <param> tag '%s': no name=\n", tag.toAscii().constData());

			// offset+5 skips 'name='
			offset = findStringInQuotes (tag, offset + name_pattern.length(), pname, true, false);
			pname = pname.toLower();

			if ( (offset = tag.find (value_pattern, offset, false)) == -1 )
				qFatal ("CHMFile::ParseAndFillTopicsTree: bad <param> tag '%s': no value=\n", tag.toAscii().constData());

			// offset+6 skips 'value='
			findStringInQuotes (tag, offset + value_pattern.length(), pvalue, false, true);

//qDebug ("<param>: name '%s', value '%s'", pname.toAscii().constData(), pvalue.toAscii().constData());

			if ( pname == "name" )
				name = pvalue;
			else if ( pname == "local" )
				urls.push_back (KCHMViewWindow::makeURLabsoluteIfNeeded (pvalue));
			else if ( pname == "see also" && asIndex && name != pvalue )
				urls.push_back (":" + pvalue);
			else if ( pname == "imagenumber" )
			{
				bool bok;
				int imgnum = pvalue.toInt (&bok);
	
				if ( bok && imgnum >= 0 && (unsigned) imgnum < MAX_BUILTIN_ICONS )
					imagenum = imgnum;
			}
		}
		else if ( tagword == "ul" ) // increase indent level
		{
			// Fix for buggy help files		
			if ( ++indent >= MAX_NEST_DEPTH )
				qFatal("CHMFile::ParseAndFillTopicsTree: max nest depth (%d) is reached, error in help file", MAX_NEST_DEPTH);
				
			rootentry[indent] = 0;
		}
		else if ( tagword == "/ul" ) // decrease indent level
		{
			if ( --indent < 0 )
				indent = 0;
				
			rootentry[indent] = 0;
		}

		pos = i;	
	}
	
	return true;

}
*/
