/***************************************************************************
 *   Copyright (C) 2004-2007 by Georgy Yunaev, gyunaev@ulduzsoft.com       *
 *   Portions Copyright (C) 2003  Razvan Cojocaru <razvanco@gmx.net>       *  
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

#include <sys/types.h>

#include <QCursor>
#include <QFile>
#include <QApplication>
#include <QByteArray>
#include <QPixmap>
#include <QVector>

#include "chm_lib.h"
#include "bitfiddle.h"
#include "libchmfile.h"
#include "libchmurlfactory.h"
#include "libchmfileimpl.h"

// Big-enough buffer size for use with various routines.
#define BUF_SIZE 4096
#define COMMON_BUF_LEN 1025

#define TOPICS_ENTRY_LEN 16
#define URLTBL_ENTRY_LEN 12

//#define DEBUGPARSER(A)	qDebug A
#define DEBUGPARSER(A)	;

class KCHMShowWaitCursor
{
	public:
		KCHMShowWaitCursor () { QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) ); }
		~KCHMShowWaitCursor () { QApplication::restoreOverrideCursor(); }
};


LCHMFileImpl::LCHMFileImpl( )
{
	m_chmFile = NULL;
	m_filename = m_font = QString::null;
	
	m_entityDecodeMap.clear();
	m_textCodec = 0;
	m_textCodecForSpecialFiles = 0;
	m_detectedLCID = 0;
	m_currentEncoding = 0;
}


LCHMFileImpl::~ LCHMFileImpl( )
{
	closeAll();
}


bool LCHMFileImpl::loadFile( const QString & archiveName )
{
	QString filename;
			
	// If the file has a file:// prefix, remove it
	if ( archiveName.startsWith( QLatin1String("file://") ) )
		filename = archiveName.mid( 7 ); // strip it
	else
		filename = archiveName;
	
	if( m_chmFile )
		closeAll();

	m_chmFile = chm_open( QFile::encodeName(filename).constData() );
	
	if ( m_chmFile == NULL )
		return false;

	m_filename = filename;
	
	// Reset encoding
	m_textCodec = 0;
	m_textCodecForSpecialFiles = 0;
	m_currentEncoding = 0;
	
	// Get information from /#WINDOWS and /#SYSTEM files (encoding, title, context file and so)
	// and guess the encoding
	getInfoFromWindows();
	getInfoFromSystem();
	guessTextEncoding();

	// Check whether the search tables are present
	if ( ResolveObject(QStringLiteral("/#TOPICS"), &m_chmTOPICS)
			&& ResolveObject(QStringLiteral("/#STRINGS"), &m_chmSTRINGS)
			&& ResolveObject(QStringLiteral("/#URLTBL"), &m_chmURLTBL)
			&& ResolveObject(QStringLiteral("/#URLSTR"), &m_chmURLSTR) )
	{
		m_lookupTablesValid = true;
		fillTopicsUrlMap();
	}
	else
		m_lookupTablesValid = false;

	if ( m_lookupTablesValid && ResolveObject (QStringLiteral("/$FIftiMain"), &m_chmFIftiMain) )
		m_searchAvailable = true;
	else
		m_searchAvailable = false;

	// Some CHM files have toc and index files, but do not set the name properly.
	// Some heuristics here.
	chmUnitInfo tui;
	
	if ( m_topicsFile.isEmpty() && ResolveObject(QStringLiteral("/toc.hhc"), &tui) )
		m_topicsFile = "/toc.hhc";
	
	if ( m_indexFile.isEmpty() && ResolveObject(QStringLiteral("/index.hhk"), &tui) )
		m_indexFile = "/index.hhk";
	
	return true;
}


void LCHMFileImpl::closeAll( )
{
	if ( m_chmFile == NULL )
		return;

	chm_close( m_chmFile );
	
	m_chmFile = NULL;
	m_filename = m_font = QString::null;
	
	m_home.clear();
	m_topicsFile.clear();
	m_indexFile.clear();
	
	m_entityDecodeMap.clear();
	m_textCodec = 0;
	m_textCodecForSpecialFiles = 0;
	m_detectedLCID = 0;
	m_currentEncoding = 0;
}


QString LCHMFileImpl::decodeEntity( const QString & entity )
{
	// Set up m_entityDecodeMap characters according to current textCodec
	if ( m_entityDecodeMap.isEmpty() )
	{
		m_entityDecodeMap[QStringLiteral("AElig")]	= encodeWithCurrentCodec ("\306"); // capital AE diphthong (ligature)
		m_entityDecodeMap[QStringLiteral("Aacute")]	= encodeWithCurrentCodec ("\301"); // capital A, acute accent
		m_entityDecodeMap[QStringLiteral("Acirc")]	= encodeWithCurrentCodec ("\302"); // capital A, circumflex accent
		m_entityDecodeMap[QStringLiteral("Agrave")]	= encodeWithCurrentCodec ("\300"); // capital A, grave accent
		m_entityDecodeMap[QStringLiteral("Aring")]	= encodeWithCurrentCodec ("\305"); // capital A, ring
		m_entityDecodeMap[QStringLiteral("Atilde")]	= encodeWithCurrentCodec ("\303"); // capital A, tilde
		m_entityDecodeMap[QStringLiteral("Auml")]	= encodeWithCurrentCodec ("\304"); // capital A, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("Ccedil")]	= encodeWithCurrentCodec ("\307"); // capital C, cedilla
		m_entityDecodeMap[QStringLiteral("Dstrok")]	= encodeWithCurrentCodec ("\320"); // whatever
		m_entityDecodeMap[QStringLiteral("ETH")]	= encodeWithCurrentCodec ("\320"); // capital Eth, Icelandic
		m_entityDecodeMap[QStringLiteral("Eacute")]	= encodeWithCurrentCodec ("\311"); // capital E, acute accent
		m_entityDecodeMap[QStringLiteral("Ecirc")]	= encodeWithCurrentCodec ("\312"); // capital E, circumflex accent
		m_entityDecodeMap[QStringLiteral("Egrave")]	= encodeWithCurrentCodec ("\310"); // capital E, grave accent
		m_entityDecodeMap[QStringLiteral("Euml")]	= encodeWithCurrentCodec ("\313"); // capital E, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("Iacute")]	= encodeWithCurrentCodec ("\315"); // capital I, acute accent
		m_entityDecodeMap[QStringLiteral("Icirc")]	= encodeWithCurrentCodec ("\316"); // capital I, circumflex accent
		m_entityDecodeMap[QStringLiteral("Igrave")]	= encodeWithCurrentCodec ("\314"); // capital I, grave accent
		m_entityDecodeMap[QStringLiteral("Iuml")]	= encodeWithCurrentCodec ("\317"); // capital I, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("Ntilde")]	= encodeWithCurrentCodec ("\321"); // capital N, tilde
		m_entityDecodeMap[QStringLiteral("Oacute")]	= encodeWithCurrentCodec ("\323"); // capital O, acute accent
		m_entityDecodeMap[QStringLiteral("Ocirc")]	= encodeWithCurrentCodec ("\324"); // capital O, circumflex accent
		m_entityDecodeMap[QStringLiteral("Ograve")]	= encodeWithCurrentCodec ("\322"); // capital O, grave accent
		m_entityDecodeMap[QStringLiteral("Oslash")]	= encodeWithCurrentCodec ("\330"); // capital O, slash
		m_entityDecodeMap[QStringLiteral("Otilde")]	= encodeWithCurrentCodec ("\325"); // capital O, tilde
		m_entityDecodeMap[QStringLiteral("Ouml")]	= encodeWithCurrentCodec ("\326"); // capital O, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("THORN")]	= encodeWithCurrentCodec ("\336"); // capital THORN, Icelandic
		m_entityDecodeMap[QStringLiteral("Uacute")]	= encodeWithCurrentCodec ("\332"); // capital U, acute accent
		m_entityDecodeMap[QStringLiteral("Ucirc")]	= encodeWithCurrentCodec ("\333"); // capital U, circumflex accent
		m_entityDecodeMap[QStringLiteral("Ugrave")]	= encodeWithCurrentCodec ("\331"); // capital U, grave accent
		m_entityDecodeMap[QStringLiteral("Uuml")]	= encodeWithCurrentCodec ("\334"); // capital U, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("Yacute")]	= encodeWithCurrentCodec ("\335"); // capital Y, acute accent
		m_entityDecodeMap[QStringLiteral("OElig")]	= encodeWithCurrentCodec ("\338"); // capital Y, acute accent
		m_entityDecodeMap[QStringLiteral("oelig")]	= encodeWithCurrentCodec ("\339"); // capital Y, acute accent
						
		m_entityDecodeMap[QStringLiteral("aacute")]	= encodeWithCurrentCodec ("\341"); // small a, acute accent
		m_entityDecodeMap[QStringLiteral("acirc")]	= encodeWithCurrentCodec ("\342"); // small a, circumflex accent
		m_entityDecodeMap[QStringLiteral("aelig")]	= encodeWithCurrentCodec ("\346"); // small ae diphthong (ligature)
		m_entityDecodeMap[QStringLiteral("agrave")]	= encodeWithCurrentCodec ("\340"); // small a, grave accent
		m_entityDecodeMap[QStringLiteral("aring")]	= encodeWithCurrentCodec ("\345"); // small a, ring
		m_entityDecodeMap[QStringLiteral("atilde")]	= encodeWithCurrentCodec ("\343"); // small a, tilde
		m_entityDecodeMap[QStringLiteral("auml")]	= encodeWithCurrentCodec ("\344"); // small a, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("ccedil")]	= encodeWithCurrentCodec ("\347"); // small c, cedilla
		m_entityDecodeMap[QStringLiteral("eacute")]	= encodeWithCurrentCodec ("\351"); // small e, acute accent
		m_entityDecodeMap[QStringLiteral("ecirc")]	= encodeWithCurrentCodec ("\352"); // small e, circumflex accent
		m_entityDecodeMap[QStringLiteral("Scaron")]	= encodeWithCurrentCodec ("\352"); // small e, circumflex accent
		m_entityDecodeMap[QStringLiteral("egrave")]	= encodeWithCurrentCodec ("\350"); // small e, grave accent
		m_entityDecodeMap[QStringLiteral("eth")]	= encodeWithCurrentCodec ("\360"); // small eth, Icelandic
		m_entityDecodeMap[QStringLiteral("euml")]	= encodeWithCurrentCodec ("\353"); // small e, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("iacute")]	= encodeWithCurrentCodec ("\355"); // small i, acute accent
		m_entityDecodeMap[QStringLiteral("icirc")]	= encodeWithCurrentCodec ("\356"); // small i, circumflex accent
		m_entityDecodeMap[QStringLiteral("igrave")]	= encodeWithCurrentCodec ("\354"); // small i, grave accent
		m_entityDecodeMap[QStringLiteral("iuml")]	= encodeWithCurrentCodec ("\357"); // small i, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("ntilde")]	= encodeWithCurrentCodec ("\361"); // small n, tilde
		m_entityDecodeMap[QStringLiteral("oacute")]	= encodeWithCurrentCodec ("\363"); // small o, acute accent
		m_entityDecodeMap[QStringLiteral("ocirc")]	= encodeWithCurrentCodec ("\364"); // small o, circumflex accent
		m_entityDecodeMap[QStringLiteral("ograve")]	= encodeWithCurrentCodec ("\362"); // small o, grave accent
		m_entityDecodeMap[QStringLiteral("oslash")]	= encodeWithCurrentCodec ("\370"); // small o, slash
		m_entityDecodeMap[QStringLiteral("otilde")]	= encodeWithCurrentCodec ("\365"); // small o, tilde
		m_entityDecodeMap[QStringLiteral("ouml")]	= encodeWithCurrentCodec ("\366"); // small o, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("szlig")]	= encodeWithCurrentCodec ("\337"); // small sharp s, German (sz ligature)
		m_entityDecodeMap[QStringLiteral("thorn")]	= encodeWithCurrentCodec ("\376"); // small thorn, Icelandic
		m_entityDecodeMap[QStringLiteral("uacute")]	= encodeWithCurrentCodec ("\372"); // small u, acute accent
		m_entityDecodeMap[QStringLiteral("ucirc")]	= encodeWithCurrentCodec ("\373"); // small u, circumflex accent
		m_entityDecodeMap[QStringLiteral("ugrave")]	= encodeWithCurrentCodec ("\371"); // small u, grave accent
		m_entityDecodeMap[QStringLiteral("uuml")]	= encodeWithCurrentCodec ("\374"); // small u, dieresis or umlaut mark
		m_entityDecodeMap[QStringLiteral("yacute")]	= encodeWithCurrentCodec ("\375"); // small y, acute accent
		m_entityDecodeMap[QStringLiteral("yuml")]	= encodeWithCurrentCodec ("\377"); // small y, dieresis or umlaut mark
	
		m_entityDecodeMap[QStringLiteral("iexcl")]	= encodeWithCurrentCodec ("\241");
		m_entityDecodeMap[QStringLiteral("cent")]	= encodeWithCurrentCodec ("\242");
		m_entityDecodeMap[QStringLiteral("pound")]	= encodeWithCurrentCodec ("\243");
		m_entityDecodeMap[QStringLiteral("curren")]	= encodeWithCurrentCodec ("\244");
		m_entityDecodeMap[QStringLiteral("yen")]	= encodeWithCurrentCodec ("\245");
		m_entityDecodeMap[QStringLiteral("brvbar")]	= encodeWithCurrentCodec ("\246");
		m_entityDecodeMap[QStringLiteral("sect")]	= encodeWithCurrentCodec ("\247");
		m_entityDecodeMap[QStringLiteral("uml")]	= encodeWithCurrentCodec ("\250");
		m_entityDecodeMap[QStringLiteral("ordf")]	= encodeWithCurrentCodec ("\252");
		m_entityDecodeMap[QStringLiteral("laquo")]	= encodeWithCurrentCodec ("\253");
		m_entityDecodeMap[QStringLiteral("not")]	= encodeWithCurrentCodec ("\254");
		m_entityDecodeMap[QStringLiteral("shy")]	= encodeWithCurrentCodec ("\255");
		m_entityDecodeMap[QStringLiteral("macr")]	= encodeWithCurrentCodec ("\257");
		m_entityDecodeMap[QStringLiteral("deg")]	= encodeWithCurrentCodec ("\260");
		m_entityDecodeMap[QStringLiteral("plusmn")]	= encodeWithCurrentCodec ("\261");
		m_entityDecodeMap[QStringLiteral("sup1")]	= encodeWithCurrentCodec ("\271");
		m_entityDecodeMap[QStringLiteral("sup2")]	= encodeWithCurrentCodec ("\262");
		m_entityDecodeMap[QStringLiteral("sup3")]	= encodeWithCurrentCodec ("\263");
		m_entityDecodeMap[QStringLiteral("acute")]	= encodeWithCurrentCodec ("\264");
		m_entityDecodeMap[QStringLiteral("micro")]	= encodeWithCurrentCodec ("\265");
		m_entityDecodeMap[QStringLiteral("para")]	= encodeWithCurrentCodec ("\266");
		m_entityDecodeMap[QStringLiteral("middot")]	= encodeWithCurrentCodec ("\267");
		m_entityDecodeMap[QStringLiteral("cedil")]	= encodeWithCurrentCodec ("\270");
		m_entityDecodeMap[QStringLiteral("ordm")]	= encodeWithCurrentCodec ("\272");
		m_entityDecodeMap[QStringLiteral("raquo")]	= encodeWithCurrentCodec ("\273");
		m_entityDecodeMap[QStringLiteral("frac14")]	= encodeWithCurrentCodec ("\274");
		m_entityDecodeMap[QStringLiteral("frac12")]	= encodeWithCurrentCodec ("\275");
		m_entityDecodeMap[QStringLiteral("frac34")]	= encodeWithCurrentCodec ("\276");
		m_entityDecodeMap[QStringLiteral("iquest")]	= encodeWithCurrentCodec ("\277");
		m_entityDecodeMap[QStringLiteral("times")]	= encodeWithCurrentCodec ("\327");
		m_entityDecodeMap[QStringLiteral("divide")]	= encodeWithCurrentCodec ("\367");
				
		m_entityDecodeMap[QStringLiteral("copy")]	= encodeWithCurrentCodec ("\251"); // copyright sign
		m_entityDecodeMap[QStringLiteral("reg")]	= encodeWithCurrentCodec ("\256"); // registered sign
		m_entityDecodeMap[QStringLiteral("nbsp")]	= encodeWithCurrentCodec ("\240"); // non breaking space

		m_entityDecodeMap[QStringLiteral("fnof")]	= QChar((unsigned short) 402);
				
		m_entityDecodeMap[QStringLiteral("Delta")]	= QChar((unsigned short) 916);
		m_entityDecodeMap[QStringLiteral("Pi")]	= QChar((unsigned short) 928);
		m_entityDecodeMap[QStringLiteral("Sigma")]	= QChar((unsigned short) 931);
		
		m_entityDecodeMap[QStringLiteral("beta")]	= QChar((unsigned short) 946);
		m_entityDecodeMap[QStringLiteral("gamma")]	= QChar((unsigned short) 947);
		m_entityDecodeMap[QStringLiteral("delta")]	= QChar((unsigned short) 948);
		m_entityDecodeMap[QStringLiteral("eta")]	= QChar((unsigned short) 951);
		m_entityDecodeMap[QStringLiteral("theta")]	= QChar((unsigned short) 952);
		m_entityDecodeMap[QStringLiteral("lambda")]	= QChar((unsigned short) 955);
		m_entityDecodeMap[QStringLiteral("mu")]	= QChar((unsigned short) 956);
		m_entityDecodeMap[QStringLiteral("nu")]	= QChar((unsigned short) 957);
		m_entityDecodeMap[QStringLiteral("pi")]	= QChar((unsigned short) 960);
		m_entityDecodeMap[QStringLiteral("rho")]	= QChar((unsigned short) 961);
		
		m_entityDecodeMap[QStringLiteral("lsquo")]	= QChar((unsigned short) 8216);
		m_entityDecodeMap[QStringLiteral("rsquo")]	= QChar((unsigned short) 8217);
		m_entityDecodeMap[QStringLiteral("rdquo")]	= QChar((unsigned short) 8221);
		m_entityDecodeMap[QStringLiteral("bdquo")]	= QChar((unsigned short) 8222);
		m_entityDecodeMap[QStringLiteral("trade")]  = QChar((unsigned short) 8482);
		m_entityDecodeMap[QStringLiteral("ldquo")]  = QChar((unsigned short) 8220);
		m_entityDecodeMap[QStringLiteral("ndash")]  = QChar((unsigned short) 8211);
		m_entityDecodeMap[QStringLiteral("mdash")]  = QChar((unsigned short) 8212);
		m_entityDecodeMap[QStringLiteral("bull")]  = QChar((unsigned short) 8226);
		m_entityDecodeMap[QStringLiteral("hellip")]  = QChar((unsigned short) 8230);
		m_entityDecodeMap[QStringLiteral("emsp")]  = QChar((unsigned short) 8195);
		m_entityDecodeMap[QStringLiteral("rarr")]  = QChar((unsigned short) 8594);
		m_entityDecodeMap[QStringLiteral("rArr")]  = QChar((unsigned short) 8658);
		m_entityDecodeMap[QStringLiteral("crarr")]  = QChar((unsigned short) 8629);
		m_entityDecodeMap[QStringLiteral("le")]  = QChar((unsigned short) 8804);
		m_entityDecodeMap[QStringLiteral("ge")]  = QChar((unsigned short) 8805);
		m_entityDecodeMap[QStringLiteral("lte")]  = QChar((unsigned short) 8804); // wrong, but used somewhere
		m_entityDecodeMap[QStringLiteral("gte")]  = QChar((unsigned short) 8805); // wrong, but used somewhere
		m_entityDecodeMap[QStringLiteral("dagger")]  = QChar((unsigned short) 8224);
		m_entityDecodeMap[QStringLiteral("Dagger")]  = QChar((unsigned short) 8225);
		m_entityDecodeMap[QStringLiteral("euro")]  = QChar((unsigned short) 8364);
		m_entityDecodeMap[QStringLiteral("asymp")]  = QChar((unsigned short) 8776);
		m_entityDecodeMap[QStringLiteral("isin")]  = QChar((unsigned short) 8712);
		m_entityDecodeMap[QStringLiteral("notin")]  = QChar((unsigned short) 8713);
		m_entityDecodeMap[QStringLiteral("prod")]  = QChar((unsigned short) 8719);
		m_entityDecodeMap[QStringLiteral("ne")]  = QChar((unsigned short) 8800);
				
		m_entityDecodeMap[QStringLiteral("amp")]	= QStringLiteral("&");	// ampersand
		m_entityDecodeMap[QStringLiteral("gt")] = QStringLiteral(">");	// greater than
		m_entityDecodeMap[QStringLiteral("lt")] = QStringLiteral("<"); 	// less than
		m_entityDecodeMap[QStringLiteral("quot")] = QStringLiteral("\""); // double quote
		m_entityDecodeMap[QStringLiteral("apos")] = QStringLiteral("'"); 	// single quote
		m_entityDecodeMap[QStringLiteral("frasl")]  = QStringLiteral("/");
		m_entityDecodeMap[QStringLiteral("minus")]  = QStringLiteral("-");
		m_entityDecodeMap[QStringLiteral("oplus")] = QStringLiteral("+");
		m_entityDecodeMap[QStringLiteral("Prime")] = QStringLiteral("\"");
	}

	// If entity is an ASCII code like &#12349; - just decode it
    if ( entity[0] == QLatin1Char('#') )
	{
		bool valid;
		unsigned int ascode = entity.midRef(1).toUInt( &valid );
						
		if ( !valid )
		{
			qWarning ( "LCHMFileImpl::decodeEntity: could not decode HTML entity '%s'", qPrintable( entity ) );
			return QString::null;
		}

		return (QString) (QChar( ascode ));
	}
	else
	{
		QMap<QString, QString>::const_iterator it = m_entityDecodeMap.constFind( entity );

		if ( it == m_entityDecodeMap.constEnd() )
		{
			qWarning ("LCHMFileImpl::decodeEntity: could not decode HTML entity '%s'", qPrintable( entity ));
			return QString::null;
		}
		
		return *it;
	}
}


inline int LCHMFileImpl::findStringInQuotes (const QString& tag, int offset, QString& value, bool firstquote, bool decodeentities)
{
    int qbegin = tag.indexOf (QLatin1Char('"'), offset);
	
	if ( qbegin == -1 )
		qFatal ("LCHMFileImpl::findStringInQuotes: cannot find first quote in <param> tag: '%s'", qPrintable( tag ));

    int qend = firstquote ? tag.indexOf (QLatin1Char('"'), qbegin + 1) : tag.lastIndexOf (QLatin1Char('"'));

	if ( qend == -1 || qend <= qbegin )
		qFatal ("LCHMFileImpl::findStringInQuotes: cannot find last quote in <param> tag: '%s'", qPrintable( tag ));

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
                if ( tag[i] == QLatin1Char('&') ) // HTML entity starts
					fill_entity = true;
				else
					value.append (tag[i]);
			}
			else
			{
                if ( tag[i] == QLatin1Char(';') ) // HTML entity ends
				{
					// If entity is an ASCII code, just decode it
					QString decode = decodeEntity( htmlentity );
					
					if ( decode.isNull() )
						break;
					
					value.append ( decode );
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


bool LCHMFileImpl::searchWord (const QString& text, 
							   bool wholeWords, 
		   					   bool titlesOnly, 
			                   LCHMSearchProgressResults& results,  
					           bool phrase_search)
{
	bool partial = false;

	if ( text.isEmpty() || !m_searchAvailable )
		return false;

    QString searchword = QString::fromLocal8Bit( convertSearchWord (text) );

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
	uint32_t node_offset = UINT32ARRAY(cursor32);

	cursor32 = header + 0x2e;
	uint32_t node_len = UINT32ARRAY(cursor32);

	unsigned char* cursor16 = header + 0x18;
	uint16_t tree_depth = UINT16ARRAY(cursor16);

	unsigned char word_len, pos;
	QString word;
	uint32_t i = sizeof(uint16_t);
	uint16_t free_space;

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

		i = sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
		uint64_t wlc_count, wlc_size;
		uint32_t wlc_offset;

		while (i < node_len - free_space)
		{
			word_len = *(buffer.data() + i);
			pos = *(buffer.data() + i + 1);

			char *wrd_buf = new char[word_len];
			memcpy (wrd_buf, buffer.data() + i + 2, word_len - 1);
			wrd_buf[word_len - 1] = 0;

			if ( pos == 0 )
                word = QString::fromLocal8Bit(wrd_buf, word_len);
			else
                word = word.mid (0, pos) + QString::fromLocal8Bit((const char*) wrd_buf, word_len);

			delete[] wrd_buf;

			i += 2 + word_len;
			unsigned char title = *(buffer.data() + i - 1);

			size_t encsz;
			wlc_count = be_encint (buffer.data() + i, encsz);
			i += encsz;
		
			cursor32 = buffer.data() + i;
			wlc_offset = UINT32ARRAY(cursor32);

			i += sizeof(uint32_t) + sizeof(uint16_t);
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


bool LCHMFileImpl::ResolveObject(const QString& fileName, chmUnitInfo *ui) const
{
	return m_chmFile != NULL 
			&& ::chm_resolve_object(m_chmFile, qPrintable( fileName ), ui) ==
			CHM_RESOLVE_SUCCESS;
}


size_t LCHMFileImpl::RetrieveObject(const chmUnitInfo *ui, unsigned char *buffer,
								LONGUINT64 fileOffset, LONGINT64 bufferSize) const
{
	return ::chm_retrieve_object(m_chmFile, const_cast<chmUnitInfo*>(ui),
								 buffer, fileOffset, bufferSize);
}


inline uint32_t LCHMFileImpl::GetLeafNodeOffset(const QString& text,
											 uint32_t initialOffset,
			uint32_t buffSize,
   uint16_t treeDepth)
{
	uint32_t test_offset = 0;
	unsigned char* cursor16, *cursor32;
	unsigned char word_len, pos;
	uint32_t i = sizeof(uint16_t);
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
		uint16_t free_space = UINT16ARRAY(cursor16);

		while (i < buffSize - free_space )
		{
			word_len = *(buffer.data() + i);
			pos = *(buffer.data() + i + 1);

			char *wrd_buf = new char[word_len];
			memcpy ( wrd_buf, buffer.data() + i + 2, word_len - 1 );
			wrd_buf[word_len - 1] = 0;

			if ( pos == 0 )
                word = QString::fromLocal8Bit(wrd_buf, word_len);
			else
                word = word.mid(0, pos) + QString::fromLocal8Bit((const char*) wrd_buf, word_len);

			delete[] wrd_buf;

			if ( text <= word )
			{
				cursor32 = buffer.data() + i + word_len + 1;
				initialOffset = UINT32ARRAY(cursor32);
				break;
			}

			i += word_len + sizeof(unsigned char) +
					sizeof(uint32_t) + sizeof(uint16_t);
		}
	}

	if ( initialOffset == test_offset )
		return 0;

	return initialOffset;
}


inline bool LCHMFileImpl::ProcessWLC (uint64_t wlc_count, uint64_t wlc_size,
								    uint32_t wlc_offset, unsigned char ds,
		  							unsigned char dr, unsigned char cs,
									unsigned char cr, unsigned char ls,
 									unsigned char lr,
 									LCHMSearchProgressResults& results,
 									bool phrase_search)
{
	int wlc_bit = 7;
	uint64_t index = 0, count;
	size_t length, off = 0;
	QVector<unsigned char> buffer (wlc_size);
	unsigned char *cursor32;

	unsigned char entry[TOPICS_ENTRY_LEN];
	unsigned char combuf[13];

	if ( RetrieveObject (&m_chmFIftiMain, buffer.data(), wlc_offset, wlc_size) == 0 )
		return false;

	for ( uint64_t i = 0; i < wlc_count; ++i )
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

		LCHMSearchProgressResult progres;

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
		
		for (uint64_t j = 0; j < count; ++j)
		{
			uint64_t lcode = sr_int (buffer.data() + off, &wlc_bit, ls, lr, length);
			off += length;
			
			if ( phrase_search )
				progres.offsets.push_back (lcode);
		}
		
		results.push_back (progres);
	}

	return true;
}


bool LCHMFileImpl::getInfoFromWindows()
{
#define WIN_HEADER_LEN 0x08
	unsigned char buffer[BUF_SIZE];
	unsigned int factor;
	chmUnitInfo ui;
	long size = 0;

	if ( ResolveObject(QStringLiteral("/#WINDOWS"), &ui) )
	{
		if ( !RetrieveObject(&ui, buffer, 0, WIN_HEADER_LEN) )
			return false;

		uint32_t entries = get_int32_le( (uint32_t *)(buffer) );
		uint32_t entry_size = get_int32_le( (uint32_t *)(buffer + 0x04) );
		
		QVector<unsigned char> uptr(entries * entry_size);
		unsigned char* raw = (unsigned char*) uptr.data();
		
		if ( !RetrieveObject (&ui, raw, 8, entries * entry_size) )
			return false;

		if( !ResolveObject (QStringLiteral("/#STRINGS"), &ui) )
			return false;

		for ( uint32_t i = 0; i < entries; ++i )
		{
			uint32_t offset = i * entry_size;
			
			uint32_t off_title = get_int32_le( (uint32_t *)(raw + offset + 0x14) );
			uint32_t off_home = get_int32_le( (uint32_t *)(raw + offset + 0x68) );
			uint32_t off_hhc = get_int32_le( (uint32_t *)(raw + offset + 0x60) );
			uint32_t off_hhk = get_int32_le( (uint32_t *)(raw + offset + 0x64) );

			factor = off_title / 4096;

			if ( size == 0 ) 
				size = RetrieveObject(&ui, buffer, factor * 4096, BUF_SIZE);

			if ( size && off_title )
				m_title = QByteArray( (const char*) (buffer + off_title % 4096) );

			if ( factor != off_home / 4096)
			{
				factor = off_home / 4096;		
				size = RetrieveObject (&ui, buffer, factor * 4096, BUF_SIZE);
			}
			
			if ( size && off_home )
				m_home = QByteArray("/") + QByteArray( (const char*) buffer + off_home % 4096);

			if ( factor != off_hhc / 4096)
			{
				factor = off_hhc / 4096;
				size = RetrieveObject(&ui, buffer, factor * 4096, BUF_SIZE);
			}
		
			if ( size && off_hhc )
				m_topicsFile = QByteArray("/") + QByteArray((const char*) buffer + off_hhc % 4096);

			if ( factor != off_hhk / 4096)
			{
				factor = off_hhk / 4096;
				size = RetrieveObject (&ui, buffer, factor * 4096, BUF_SIZE);
			}

			if ( size && off_hhk )
				m_indexFile = QByteArray("/") + QByteArray((const char*) buffer + off_hhk % 4096);
		}
	}
	return true;
}



bool LCHMFileImpl::getInfoFromSystem()
{
	unsigned char buffer[BUF_SIZE];
	chmUnitInfo ui;
	
	int index = 0;
	unsigned char* cursor = NULL, *p;
	uint16_t value = 0;
	long size = 0;

	// Run the first loop to detect the encoding. We need this, because title could be
	// already encoded in user encoding. Same for file names
	if ( !ResolveObject (QStringLiteral("/#SYSTEM"), &ui) )
		return false;

	// Can we pull BUFF_SIZE bytes of the #SYSTEM file?
	if ( (size = RetrieveObject (&ui, buffer, 4, BUF_SIZE)) == 0 )
		return false;

	buffer[size - 1] = 0;

	// First loop to detect the encoding
	for ( index = 0; index < (size - 1 - (long)sizeof(uint16_t)) ;)
	{
		cursor = buffer + index;
		value = UINT16ARRAY(cursor);

		switch(value)
		{
			case 0:
				index += 2;
				cursor = buffer + index;
			
				if(m_topicsFile.isEmpty())
					m_topicsFile = QByteArray("/") + QByteArray((const char*) buffer + index + 2);
				
				break;
			
			case 1:
				index += 2;
				cursor = buffer + index;

				if(m_indexFile.isEmpty())
					m_indexFile = QByteArray("/") + QByteArray((const char*)buffer + index + 2);
				break;
		
			case 2:
				index += 2;
				cursor = buffer + index;
				
				if(m_home.isEmpty() || m_home == "/")
					m_home = QByteArray("/") + QByteArray((const char*) buffer + index + 2);
				break;
			
			case 3:
				index += 2;
				cursor = buffer + index;
				m_title = QByteArray( (const char*) (buffer + index + 2) );
				break;

			case 4:
				index += 2;
				cursor = buffer + index;

				p = buffer + index + 2;
				m_detectedLCID = (short) (p[0] | (p[1]<<8));
			
				break;

			case 6:
				index += 2;
				cursor = buffer + index;

				if ( m_topicsFile.isEmpty() )
				{
					QString topicAttempt = QStringLiteral("/"), tmp;
                    topicAttempt += QString::fromLocal8Bit ((const char*) buffer +index +2);

                    tmp = topicAttempt + QStringLiteral(".hhc");
				
					if ( ResolveObject( tmp, &ui) )
						m_topicsFile = qPrintable( tmp );

                    tmp = topicAttempt + QStringLiteral(".hhk");
				
					if ( ResolveObject( tmp, &ui) )
						m_indexFile = qPrintable( tmp );
				}
				break;

			case 16:
				index += 2;
				cursor = buffer + index;

                m_font = QString::fromLocal8Bit ((const char*) buffer + index + 2);
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

 
QByteArray LCHMFileImpl::convertSearchWord( const QString & src )
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
		return (QByteArray) qPrintable( src.toLower() );

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



void LCHMFileImpl::getSearchResults( const LCHMSearchProgressResults& tempres, 
									 QStringList * results, 
		  							 unsigned int limit_results )
{
	unsigned char combuf [COMMON_BUF_LEN];
	QMap<uint32_t, uint32_t> urlsmap;  // used to prevent duplicated urls
	
	for ( int i = 0; i < tempres.size(); i++ )
	{
		if ( urlsmap.find (tempres[i].urloff) != urlsmap.end() )
			continue;
		
		urlsmap[tempres[i].urloff] = 1;
		
		if ( RetrieveObject (&m_chmURLSTR, combuf, tempres[i].urloff + 8, COMMON_BUF_LEN - 1) == 0 )
			continue;

		combuf[COMMON_BUF_LEN - 1] = 0;
        results->push_back( LCHMUrlFactory::makeURLabsoluteIfNeeded( QString::fromLocal8Bit((const char*) combuf) ) );
		
		if ( --limit_results == 0 )
			break;
	}
}


QString LCHMFileImpl::normalizeUrl( const QString & path ) const
{
    int pos = path.indexOf (QLatin1Char('#'));
	QString fixedpath = pos == -1 ? path : path.left (pos);
	
	return LCHMUrlFactory::makeURLabsoluteIfNeeded( fixedpath );
}


/*
 * FIXME: <OBJECT type="text/sitemap"><param name="Merge" value="hhaxref.chm::/HHOCX_c.hhc"></OBJECT>
 *  (from htmlhelp.chm)
*/
bool LCHMFileImpl::parseFileAndFillArray( const QString & file, QVector< LCHMParsedEntry > * data, bool asIndex )
{
	QString src;
	const int MAX_NEST_DEPTH = 256;

	if ( !getFileContentAsString( &src, file ) || src.isEmpty() )
		return false;

	KCHMShowWaitCursor wc;
		
/*
	// Save the index for debugging purposes
	QFile outfile( "parsed.htm" );
	
	if ( outfile.open( IO_WriteOnly ) )
	{
		QTextStream textstream( &outfile );
		textstream << src;
		outfile.close();
	}
*/
	
	unsigned int defaultimagenum = asIndex ? LCHMBookIcons::IMAGE_INDEX : LCHMBookIcons::IMAGE_AUTO;
	int pos = 0, indent = 0, root_indent_offset = 0;
	bool in_object = false, root_indent_offset_set = false;
	
	LCHMParsedEntry entry;
	entry.imageid = defaultimagenum;
	
	// Split the HHC file by HTML tags
	int stringlen = src.length();
	
    while ( pos < stringlen && (pos = src.indexOf (QLatin1Char('<'), pos)) != -1 )
	{
		int i, word_end = 0;
		
		for ( i = ++pos; i < stringlen; i++ )
		{
			// If a " or ' is found, skip to the next one.
            if ( (src[i] == QLatin1Char('"') || src[i] == QLatin1Char('\'')) )
			{
				// find where quote ends, either by another quote, or by '>' symbol (some people don't know HTML)
				int nextpos = src.indexOf (src[i], i+1);
                if ( nextpos == -1 	&& (nextpos = src.indexOf (QLatin1Char('>'), i+1)) == -1 )
				{
					qWarning ("LCHMFileImpl::ParseHhcAndFillTree: corrupted TOC: %s", qPrintable( src.mid(i) ));
					return false;
				}

				i =  nextpos;
			}
            else if ( src[i] == QLatin1Char('>')  )
				break;
            else if ( !src[i].isLetterOrNumber() && src[i] != QLatin1Char('/') && !word_end )
				word_end = i;
		}
		
		QString tagword, tag = src.mid (pos, i - pos);
		 
		if ( word_end )
			tagword = src.mid (pos, word_end - pos).toLower();
		else
			tagword = tag.toLower();

		//qDebug ("tag: '%s', tagword: '%s'\n", qPrintable( tag ), qPrintable( tagword ) );
						
		// <OBJECT type="text/sitemap"> - a topic entry
		if ( tagword == QLatin1String("object") && tag.indexOf (QStringLiteral("text/sitemap"), 0, Qt::CaseInsensitive ) != -1 )
			in_object = true;
		else if ( tagword == QLatin1String("/object") && in_object ) 
		{
			// a topic entry closed. Add a tree item
			if ( !entry.name.isEmpty() )
			{
				if ( !root_indent_offset_set )
				{
					root_indent_offset_set = true;
					root_indent_offset = indent;
					
					if ( root_indent_offset > 1 )
						qWarning("CHM has improper index; root indent offset is %d", root_indent_offset);
				}
				
				// Trim the entry name
				entry.name = entry.name.trimmed();
				
				int real_indent = indent - root_indent_offset;
				
				entry.indent = real_indent;
				data->push_back( entry );
			}
			else
			{
				if ( !entry.urls.isEmpty() )
					qDebug ("LCHMFileImpl::ParseAndFillTopicsTree: <object> tag with url \"%s\" is parsed, but name is empty.", qPrintable( entry.urls[0] ));
				else
					qDebug ("LCHMFileImpl::ParseAndFillTopicsTree: <object> tag is parsed, but both name and url are empty.");	
			}

			entry.name = QString::null;
			entry.urls.clear();
			entry.imageid = defaultimagenum;
			in_object = false;
		}
		else if ( tagword == QLatin1String("param") && in_object )
		{
			// <param name="Name" value="First Page">
			int offset; // strlen("param ")
			QString name_pattern = QStringLiteral("name="), value_pattern = QStringLiteral("value=");
			QString pname, pvalue;

			if ( (offset = tag.indexOf (name_pattern, 0, Qt::CaseInsensitive )) == -1 )
				qFatal ("LCHMFileImpl::ParseAndFillTopicsTree: bad <param> tag '%s': no name=\n", qPrintable( tag ));

			// offset+5 skips 'name='
			offset = findStringInQuotes (tag, offset + name_pattern.length(), pname, true, false);
			pname = pname.toLower();

			if ( (offset = tag.indexOf(value_pattern, offset, Qt::CaseInsensitive )) == -1 )
				qFatal ("LCHMFileImpl::ParseAndFillTopicsTree: bad <param> tag '%s': no value=\n", qPrintable( tag ));

			// offset+6 skips 'value='
			findStringInQuotes (tag, offset + value_pattern.length(), pvalue, false, true);

			//qDebug ("<param>: name '%s', value '%s'", qPrintable( pname ), qPrintable( pvalue ));

			if ( pname == QLatin1String("name") )
			{
				// Some help files contain duplicate names, where the second name is empty. Work it around by keeping the first one
				if ( !pvalue.isEmpty() )
					entry.name = pvalue;
			}
			else if ( pname == QLatin1String("local") )
			{
				// Check for URL duplication
				QString url = LCHMUrlFactory::makeURLabsoluteIfNeeded( pvalue );
				
				if ( !entry.urls.contains( url ) )
					entry.urls.push_back( url );
			}
			else if ( pname == QLatin1String("see also") && asIndex && entry.name != pvalue )
                entry.urls.push_back (QLatin1Char(':') + pvalue);
			else if ( pname == QLatin1String("imagenumber") )
			{
				bool bok;
				int imgnum = pvalue.toInt (&bok);
	
				if ( bok && imgnum >= 0 && imgnum < LCHMBookIcons::MAX_BUILTIN_ICONS )
					entry.imageid = imgnum;
			}
		}
		else if ( tagword == QLatin1String("ul") ) // increase indent level
		{
			// Fix for buggy help files		
			if ( ++indent >= MAX_NEST_DEPTH )
				qFatal("LCHMFileImpl::ParseAndFillTopicsTree: max nest depth (%d) is reached, error in help file", MAX_NEST_DEPTH);

			// This intended to fix <ul><ul>, which was seen in some buggy chm files,
			// and brokes rootentry[indent-1] check
		}
		else if ( tagword == QLatin1String("/ul") ) // decrease indent level
		{
			if ( --indent < root_indent_offset )
				indent = root_indent_offset;
			
			DEBUGPARSER(("</ul>: new intent is %d\n", indent - root_indent_offset));
		}

		pos = i;	
	}
	
	return true;
}


bool LCHMFileImpl::getFileContentAsBinary( QByteArray * data, const QString & url ) const
{
	chmUnitInfo ui;

	if( !ResolveObject( url, &ui ) )
		return false;

	data->resize( ui.length );
			
	if ( RetrieveObject( &ui, (unsigned char*) data->data(), 0, ui.length ) )
		return true;
	else
		return false;
}

	
bool LCHMFileImpl::getFileContentAsString( QString * str, const QString & url, bool internal_encoding )
{
	QByteArray buf;
	
	if ( getFileContentAsBinary( &buf, url ) )
	{
		unsigned int length = buf.size();
		
		if ( length > 0 )
		{
			buf.resize( length + 1 );
            buf [length] = '\0';
			
            *str = internal_encoding ? QString::fromLocal8Bit( buf.constData() ) :  encodeWithCurrentCodec( buf.constData() );
			return true;
		}
	}
	
	return false;
}


QString LCHMFileImpl::getTopicByUrl( const QString & url ) const
{
	QMap< QString, QString >::const_iterator it = m_url2topics.find( url );
	
	if ( it == m_url2topics.end() )
		return QString::null;
	
	return it.value();
}


extern "C" int chm_enumerator_callback( struct chmFile*, struct chmUnitInfo *ui, void *context )
{
    ((QStringList*) context)->push_back(QString::fromLocal8Bit( ui->path ) );
	return CHM_ENUMERATOR_CONTINUE;
}

bool LCHMFileImpl::enumerateFiles( QStringList * files )
{
	files->clear();
	return chm_enumerate( m_chmFile, CHM_ENUMERATE_ALL, chm_enumerator_callback, files );
}

const QPixmap * LCHMFileImpl::getBookIconPixmap( unsigned int imagenum )
{
	return m_imagesKeeper.getImage( imagenum );
}

bool LCHMFileImpl::setCurrentEncoding( const LCHMTextEncoding * encoding )
{
	m_currentEncoding = encoding;
	return changeFileEncoding( encoding->qtcodec );
}


bool LCHMFileImpl::guessTextEncoding( )
{
	const LCHMTextEncoding * enc = 0;

	if ( !m_detectedLCID || (enc = lookupByLCID (m_detectedLCID)) == 0 )
		qFatal ("Could not detect text encoding by LCID");
	
	if ( changeFileEncoding (enc->qtcodec) )
	{
		m_currentEncoding = enc;
		return true;
	}
	
	return false;
}

bool LCHMFileImpl::changeFileEncoding( const char *qtencoding  )
{
	// Encoding could be either simple Qt codepage, or set like CP1251/KOI8, which allows to
	// set up encodings separately for text (first) and internal files (second)
	const char * p = strchr( qtencoding, '/' );
	if ( p )
	{
		char buf[128]; // much bigger that any encoding possible. No DoS; all encodings are hardcoded.
		strcpy( buf, qtencoding );
		buf[p - qtencoding] = '\0';
		
		m_textCodec = QTextCodec::codecForName( buf );
	
		if ( !m_textCodec )
		{
			qWarning( "Could not set up Text Codec for encoding '%s'", buf );
			return false;
		}
		
		m_textCodecForSpecialFiles = QTextCodec::codecForName( p + 1 );
	
		if ( !m_textCodecForSpecialFiles )
		{
			qWarning( "Could not set up Text Codec for encoding '%s'", p + 1 );
			return false;
		}
	}
	else
	{
		m_textCodecForSpecialFiles = m_textCodec = QTextCodec::codecForName (qtencoding);
	
		if ( !m_textCodec )
		{
			qWarning( "Could not set up Text Codec for encoding '%s'", qtencoding );
			return false;
		}
	}
	
	m_entityDecodeMap.clear();
	return true;
}


void LCHMFileImpl::fillTopicsUrlMap()
{
	if ( !m_lookupTablesValid )
		return;

	// Read those tables
	QVector<unsigned char> topics( m_chmTOPICS.length ), urltbl( m_chmURLTBL.length ), urlstr( m_chmURLSTR.length ), strings( m_chmSTRINGS.length );

	if ( !RetrieveObject( &m_chmTOPICS, (unsigned char*) topics.data(), 0, m_chmTOPICS.length )
	|| !RetrieveObject( &m_chmURLTBL, (unsigned char*) urltbl.data(), 0, m_chmURLTBL.length )
	|| !RetrieveObject( &m_chmURLSTR, (unsigned char*) urlstr.data(), 0, m_chmURLSTR.length )
	|| !RetrieveObject( &m_chmSTRINGS, (unsigned char*) strings.data(), 0, m_chmSTRINGS.length ) )
		return;
	
	for ( unsigned int i = 0; i < m_chmTOPICS.length; i += TOPICS_ENTRY_LEN )
	{
		uint32_t off_title = get_int32_le( (uint32_t *)(topics.data() + i + 4) );
		uint32_t off_url = get_int32_le( (uint32_t *)(topics.data() + i + 8) );
		off_url = get_int32_le( (uint32_t *)( urltbl.data() + off_url + 8) ) + 8;

        QString url = LCHMUrlFactory::makeURLabsoluteIfNeeded( QString::fromLocal8Bit ( (const char*) urlstr.data() + off_url ) );

		if ( off_title < (uint32_t)strings.size() )
			m_url2topics[url] = encodeWithCurrentCodec ( (const char*) strings.data() + off_title );
		else
			m_url2topics[url] = QStringLiteral("Untitled");
	}
}


bool LCHMFileImpl::getFileSize(unsigned int * size, const QString & url)
{
	chmUnitInfo ui;

	if( !ResolveObject( url, &ui ) )
		return false;

	*size = ui.length;
	return true;
}
