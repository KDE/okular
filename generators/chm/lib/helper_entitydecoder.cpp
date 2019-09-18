/*
 *  Kchmviewer - a CHM and EPUB file viewer with broad language support
 *  Copyright (C) 2004-2014 George Yunaev, gyunaev@ulduzsoft.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QTextCodec>

#include "helper_entitydecoder.h"


HelperEntityDecoder::HelperEntityDecoder(QTextCodec *encoder)
{
	changeEncoding( encoder );

}

static inline QString encodeWithCodec( QTextCodec *encoder, const QByteArray& str )
{
	return (encoder ? encoder->toUnicode( str.constData () ) : str);
}

void HelperEntityDecoder::changeEncoding(QTextCodec *encoder)
{
	// Set up m_entityDecodeMap characters according to current textCodec
	m_entityDecodeMap.clear();

	m_entityDecodeMap["AElig"]	= encodeWithCodec( encoder, "\306"); // capital AE diphthong (ligature)
	m_entityDecodeMap["Aacute"]	= encodeWithCodec( encoder, "\301"); // capital A, acute accent
	m_entityDecodeMap["Acirc"]	= encodeWithCodec( encoder, "\302"); // capital A, circumflex accent
	m_entityDecodeMap["Agrave"]	= encodeWithCodec( encoder, "\300"); // capital A, grave accent
	m_entityDecodeMap["Aring"]	= encodeWithCodec( encoder, "\305"); // capital A, ring
	m_entityDecodeMap["Atilde"]	= encodeWithCodec( encoder, "\303"); // capital A, tilde
	m_entityDecodeMap["Auml"]	= encodeWithCodec( encoder, "\304"); // capital A, dieresis or umlaut mark
	m_entityDecodeMap["Ccedil"]	= encodeWithCodec( encoder, "\307"); // capital C, cedilla
	m_entityDecodeMap["Dstrok"]	= encodeWithCodec( encoder, "\320"); // whatever
	m_entityDecodeMap["ETH"]	= encodeWithCodec( encoder, "\320"); // capital Eth, Icelandic
	m_entityDecodeMap["Eacute"]	= encodeWithCodec( encoder, "\311"); // capital E, acute accent
	m_entityDecodeMap["Ecirc"]	= encodeWithCodec( encoder, "\312"); // capital E, circumflex accent
	m_entityDecodeMap["Egrave"]	= encodeWithCodec( encoder, "\310"); // capital E, grave accent
	m_entityDecodeMap["Euml"]	= encodeWithCodec( encoder, "\313"); // capital E, dieresis or umlaut mark
	m_entityDecodeMap["Iacute"]	= encodeWithCodec( encoder, "\315"); // capital I, acute accent
	m_entityDecodeMap["Icirc"]	= encodeWithCodec( encoder, "\316"); // capital I, circumflex accent
	m_entityDecodeMap["Igrave"]	= encodeWithCodec( encoder, "\314"); // capital I, grave accent
	m_entityDecodeMap["Iuml"]	= encodeWithCodec( encoder, "\317"); // capital I, dieresis or umlaut mark
	m_entityDecodeMap["Ntilde"]	= encodeWithCodec( encoder, "\321"); // capital N, tilde
	m_entityDecodeMap["Oacute"]	= encodeWithCodec( encoder, "\323"); // capital O, acute accent
	m_entityDecodeMap["Ocirc"]	= encodeWithCodec( encoder, "\324"); // capital O, circumflex accent
	m_entityDecodeMap["Ograve"]	= encodeWithCodec( encoder, "\322"); // capital O, grave accent
	m_entityDecodeMap["Oslash"]	= encodeWithCodec( encoder, "\330"); // capital O, slash
	m_entityDecodeMap["Otilde"]	= encodeWithCodec( encoder, "\325"); // capital O, tilde
	m_entityDecodeMap["Ouml"]	= encodeWithCodec( encoder, "\326"); // capital O, dieresis or umlaut mark
	m_entityDecodeMap["THORN"]	= encodeWithCodec( encoder, "\336"); // capital THORN, Icelandic
	m_entityDecodeMap["Uacute"]	= encodeWithCodec( encoder, "\332"); // capital U, acute accent
	m_entityDecodeMap["Ucirc"]	= encodeWithCodec( encoder, "\333"); // capital U, circumflex accent
	m_entityDecodeMap["Ugrave"]	= encodeWithCodec( encoder, "\331"); // capital U, grave accent
	m_entityDecodeMap["Uuml"]	= encodeWithCodec( encoder, "\334"); // capital U, dieresis or umlaut mark
	m_entityDecodeMap["Yacute"]	= encodeWithCodec( encoder, "\335"); // capital Y, acute accent
	m_entityDecodeMap["OElig"]	= encodeWithCodec( encoder, "\338"); // capital Y, acute accent
	m_entityDecodeMap["oelig"]	= encodeWithCodec( encoder, "\339"); // capital Y, acute accent

	m_entityDecodeMap["aacute"]	= encodeWithCodec( encoder, "\341"); // small a, acute accent
	m_entityDecodeMap["acirc"]	= encodeWithCodec( encoder, "\342"); // small a, circumflex accent
	m_entityDecodeMap["aelig"]	= encodeWithCodec( encoder, "\346"); // small ae diphthong (ligature)
	m_entityDecodeMap["agrave"]	= encodeWithCodec( encoder, "\340"); // small a, grave accent
	m_entityDecodeMap["aring"]	= encodeWithCodec( encoder, "\345"); // small a, ring
	m_entityDecodeMap["atilde"]	= encodeWithCodec( encoder, "\343"); // small a, tilde
	m_entityDecodeMap["auml"]	= encodeWithCodec( encoder, "\344"); // small a, dieresis or umlaut mark
	m_entityDecodeMap["ccedil"]	= encodeWithCodec( encoder, "\347"); // small c, cedilla
	m_entityDecodeMap["eacute"]	= encodeWithCodec( encoder, "\351"); // small e, acute accent
	m_entityDecodeMap["ecirc"]	= encodeWithCodec( encoder, "\352"); // small e, circumflex accent
	m_entityDecodeMap["Scaron"]	= encodeWithCodec( encoder, "\352"); // small e, circumflex accent
	m_entityDecodeMap["egrave"]	= encodeWithCodec( encoder, "\350"); // small e, grave accent
	m_entityDecodeMap["eth"]	= encodeWithCodec( encoder, "\360"); // small eth, Icelandic
	m_entityDecodeMap["euml"]	= encodeWithCodec( encoder, "\353"); // small e, dieresis or umlaut mark
	m_entityDecodeMap["iacute"]	= encodeWithCodec( encoder, "\355"); // small i, acute accent
	m_entityDecodeMap["icirc"]	= encodeWithCodec( encoder, "\356"); // small i, circumflex accent
	m_entityDecodeMap["igrave"]	= encodeWithCodec( encoder, "\354"); // small i, grave accent
	m_entityDecodeMap["iuml"]	= encodeWithCodec( encoder, "\357"); // small i, dieresis or umlaut mark
	m_entityDecodeMap["ntilde"]	= encodeWithCodec( encoder, "\361"); // small n, tilde
	m_entityDecodeMap["oacute"]	= encodeWithCodec( encoder, "\363"); // small o, acute accent
	m_entityDecodeMap["ocirc"]	= encodeWithCodec( encoder, "\364"); // small o, circumflex accent
	m_entityDecodeMap["ograve"]	= encodeWithCodec( encoder, "\362"); // small o, grave accent
	m_entityDecodeMap["oslash"]	= encodeWithCodec( encoder, "\370"); // small o, slash
	m_entityDecodeMap["otilde"]	= encodeWithCodec( encoder, "\365"); // small o, tilde
	m_entityDecodeMap["ouml"]	= encodeWithCodec( encoder, "\366"); // small o, dieresis or umlaut mark
	m_entityDecodeMap["szlig"]	= encodeWithCodec( encoder, "\337"); // small sharp s, German (sz ligature)
	m_entityDecodeMap["thorn"]	= encodeWithCodec( encoder, "\376"); // small thorn, Icelandic
	m_entityDecodeMap["uacute"]	= encodeWithCodec( encoder, "\372"); // small u, acute accent
	m_entityDecodeMap["ucirc"]	= encodeWithCodec( encoder, "\373"); // small u, circumflex accent
	m_entityDecodeMap["ugrave"]	= encodeWithCodec( encoder, "\371"); // small u, grave accent
	m_entityDecodeMap["uuml"]	= encodeWithCodec( encoder, "\374"); // small u, dieresis or umlaut mark
	m_entityDecodeMap["yacute"]	= encodeWithCodec( encoder, "\375"); // small y, acute accent
	m_entityDecodeMap["yuml"]	= encodeWithCodec( encoder, "\377"); // small y, dieresis or umlaut mark

	m_entityDecodeMap["iexcl"]	= encodeWithCodec( encoder, "\241");
	m_entityDecodeMap["cent"]	= encodeWithCodec( encoder, "\242");
	m_entityDecodeMap["pound"]	= encodeWithCodec( encoder, "\243");
	m_entityDecodeMap["curren"]	= encodeWithCodec( encoder, "\244");
	m_entityDecodeMap["yen"]	= encodeWithCodec( encoder, "\245");
	m_entityDecodeMap["brvbar"]	= encodeWithCodec( encoder, "\246");
	m_entityDecodeMap["sect"]	= encodeWithCodec( encoder, "\247");
	m_entityDecodeMap["uml"]	= encodeWithCodec( encoder, "\250");
	m_entityDecodeMap["ordf"]	= encodeWithCodec( encoder, "\252");
	m_entityDecodeMap["laquo"]	= encodeWithCodec( encoder, "\253");
	m_entityDecodeMap["not"]	= encodeWithCodec( encoder, "\254");
	m_entityDecodeMap["shy"]	= encodeWithCodec( encoder, "\255");
	m_entityDecodeMap["macr"]	= encodeWithCodec( encoder, "\257");
	m_entityDecodeMap["deg"]	= encodeWithCodec( encoder, "\260");
	m_entityDecodeMap["plusmn"]	= encodeWithCodec( encoder, "\261");
	m_entityDecodeMap["sup1"]	= encodeWithCodec( encoder, "\271");
	m_entityDecodeMap["sup2"]	= encodeWithCodec( encoder, "\262");
	m_entityDecodeMap["sup3"]	= encodeWithCodec( encoder, "\263");
	m_entityDecodeMap["acute"]	= encodeWithCodec( encoder, "\264");
	m_entityDecodeMap["micro"]	= encodeWithCodec( encoder, "\265");
	m_entityDecodeMap["para"]	= encodeWithCodec( encoder, "\266");
	m_entityDecodeMap["middot"]	= encodeWithCodec( encoder, "\267");
	m_entityDecodeMap["cedil"]	= encodeWithCodec( encoder, "\270");
	m_entityDecodeMap["ordm"]	= encodeWithCodec( encoder, "\272");
	m_entityDecodeMap["raquo"]	= encodeWithCodec( encoder, "\273");
	m_entityDecodeMap["frac14"]	= encodeWithCodec( encoder, "\274");
	m_entityDecodeMap["frac12"]	= encodeWithCodec( encoder, "\275");
	m_entityDecodeMap["frac34"]	= encodeWithCodec( encoder, "\276");
	m_entityDecodeMap["iquest"]	= encodeWithCodec( encoder, "\277");
	m_entityDecodeMap["times"]	= encodeWithCodec( encoder, "\327");
	m_entityDecodeMap["divide"]	= encodeWithCodec( encoder, "\367");

	m_entityDecodeMap["copy"]	= encodeWithCodec( encoder, "\251"); // copyright sign
	m_entityDecodeMap["reg"]	= encodeWithCodec( encoder, "\256"); // registered sign
	m_entityDecodeMap["nbsp"]	= encodeWithCodec( encoder, "\240"); // non breaking space

	m_entityDecodeMap["fnof"]	= QChar((unsigned short) 402);

	m_entityDecodeMap["Delta"]	= QChar((unsigned short) 916);
	m_entityDecodeMap["Pi"]	= QChar((unsigned short) 928);
	m_entityDecodeMap["Sigma"]	= QChar((unsigned short) 931);

	m_entityDecodeMap["beta"]	= QChar((unsigned short) 946);
	m_entityDecodeMap["gamma"]	= QChar((unsigned short) 947);
	m_entityDecodeMap["delta"]	= QChar((unsigned short) 948);
	m_entityDecodeMap["eta"]	= QChar((unsigned short) 951);
	m_entityDecodeMap["theta"]	= QChar((unsigned short) 952);
	m_entityDecodeMap["lambda"]	= QChar((unsigned short) 955);
	m_entityDecodeMap["mu"]	= QChar((unsigned short) 956);
	m_entityDecodeMap["nu"]	= QChar((unsigned short) 957);
	m_entityDecodeMap["pi"]	= QChar((unsigned short) 960);
	m_entityDecodeMap["rho"]	= QChar((unsigned short) 961);

	m_entityDecodeMap["lsquo"]	= QChar((unsigned short) 8216);
	m_entityDecodeMap["rsquo"]	= QChar((unsigned short) 8217);
	m_entityDecodeMap["rdquo"]	= QChar((unsigned short) 8221);
	m_entityDecodeMap["bdquo"]	= QChar((unsigned short) 8222);
	m_entityDecodeMap["trade"]  = QChar((unsigned short) 8482);
	m_entityDecodeMap["ldquo"]  = QChar((unsigned short) 8220);
	m_entityDecodeMap["ndash"]  = QChar((unsigned short) 8211);
	m_entityDecodeMap["mdash"]  = QChar((unsigned short) 8212);
	m_entityDecodeMap["bull"]  = QChar((unsigned short) 8226);
	m_entityDecodeMap["hellip"]  = QChar((unsigned short) 8230);
	m_entityDecodeMap["emsp"]  = QChar((unsigned short) 8195);
	m_entityDecodeMap["rarr"]  = QChar((unsigned short) 8594);
	m_entityDecodeMap["rArr"]  = QChar((unsigned short) 8658);
	m_entityDecodeMap["crarr"]  = QChar((unsigned short) 8629);
	m_entityDecodeMap["le"]  = QChar((unsigned short) 8804);
	m_entityDecodeMap["ge"]  = QChar((unsigned short) 8805);
	m_entityDecodeMap["lte"]  = QChar((unsigned short) 8804); // wrong, but used somewhere
	m_entityDecodeMap["gte"]  = QChar((unsigned short) 8805); // wrong, but used somewhere
	m_entityDecodeMap["dagger"]  = QChar((unsigned short) 8224);
	m_entityDecodeMap["Dagger"]  = QChar((unsigned short) 8225);
	m_entityDecodeMap["euro"]  = QChar((unsigned short) 8364);
	m_entityDecodeMap["asymp"]  = QChar((unsigned short) 8776);
	m_entityDecodeMap["isin"]  = QChar((unsigned short) 8712);
	m_entityDecodeMap["notin"]  = QChar((unsigned short) 8713);
	m_entityDecodeMap["prod"]  = QChar((unsigned short) 8719);
	m_entityDecodeMap["ne"]  = QChar((unsigned short) 8800);

	m_entityDecodeMap["amp"]	= "&";	// ampersand
	m_entityDecodeMap["gt"] = ">";	// greater than
	m_entityDecodeMap["lt"] = "<"; 	// less than
	m_entityDecodeMap["quot"] = "\""; // double quote
	m_entityDecodeMap["apos"] = "'"; 	// single quote
	m_entityDecodeMap["frasl"]  = "/";
	m_entityDecodeMap["minus"]  = "-";
	m_entityDecodeMap["oplus"] = "+";
	m_entityDecodeMap["Prime"] = "\"";
}


QString HelperEntityDecoder::decode( const QString &entity ) const
{
	// If entity is an ASCII code like &#12349; - just decode it
	if ( entity.isEmpty() )
	{
		return "";
	}
	else if ( entity[0] == '#' )
	{
		bool valid;
		unsigned int ascode = entity.mid(1).toUInt( &valid );

		if ( !valid )
		{
			qWarning ( "HelperEntityDecoder::decode: could not decode HTML entity '%s'", qPrintable( entity ) );
			return QString();
		}

		return (QString) (QChar( ascode ));
	}
	else
	{
		QMap<QString, QString>::const_iterator it = m_entityDecodeMap.find( entity );

		if ( it == m_entityDecodeMap.end() )
		{
			qWarning ("HelperEntityDecoder::decode: could not decode HTML entity '%s'", qPrintable( entity ));
			return "";
		}

		return *it;
	}
}
