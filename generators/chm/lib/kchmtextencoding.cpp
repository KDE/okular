/***************************************************************************
 *   Copyright (C) 2005 by Georgy Yunaev                                   *
 *   tim@krasnogorsk.ru                                                    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "kchmtextencoding.h"

static const KCHMTextEncoding::text_encoding_t text_encoding_table [] = 
{
	{	"Afrikaans",0,			0x0436,	1252,	0,		"CP1252"	},
	{	"Albanian",	0,			0x041C,	1250,	238,	"CP1250"	},
	{	"Arabic",	"Algeria",	0x1401,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Bahrain",	0x3C01,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Egypt",	0x0C01,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Iraq",		0x0801,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Jordan",	0x2C01,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Kuwait",	0x3401,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Lebanon",	0x3001,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Libya",	0x1001,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Morocco",	0x1801,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Oman",		0x2001,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Qatar",	0x4001,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Saudi Arabia",	0x0401,	1256,	0,	"CP1256"	},
	{	"Arabic",	"Syria",	0x2801,	1256,	0,		"CP1256"	},
	{	"Arabic",	"Tunisia",	0x1C01,	1256,	0,		"CP1256"	},
	{	"Arabic",	"United Arab Emirates",	0x3801,	1256,	178,	"CP1256"	},
	{	"Arabic",	"Yemen",	0x2401,	1256,	0,		"CP1256"	},
	{	"Armenian",	0,			0x042B,	0,		0,		"Latin1"	},
	{	"Azeri",	"Cyrillic",	0x082C,	1251,	0,		"CP1251"	},
	{	"Azeri",	"Latin",	0x042C,	1254,	162,	"CP1254"	},
	{	"Basque",	0,			0x042D,	1252,	0,		"CP1252"	},
	{	"Belarusian",	0,		0x0423,	1251,	0,		"CP1251"	},
	{	"Bulgarian",	0,		0x0402,	1251,	0,		"CP1251"	},
	{	"Catalan",	0,	0x0403,	1252,	0,	"CP1252"	},
	{	"Chinese",	"China",	0x0804,	936,	134,	"GBK"		},
	{	"Chinese",	"Hong Kong SAR",	0x0C04,	950,	136,	"Big5"	},
	{	"Chinese",	"Macau SAR",	0x1404,	950,	136,	"Big5"	},
	{	"Chinese",	"Singapore",	0x1004,	936,	134,	"GB2313"	},
	{	"Chinese",	"Taiwan",	0x0404,	950,	136,	"Big5"	}, // traditional
	{	"Croatian",	0,	0x041A,	1250,	238,	"CP1250"	},
	{	"Czech",	0,	0x0405,	1250,	238,	"CP1250"	},
	{	"Danish",	0,	0x0406,	1252,	0,	"CP1252"	},
	{	"Dutch",	"Belgium",	0x0813,	1252,	0,	"CP1252"	},
	{	"Dutch",	"The Netherlands",	0x0413,	1252,	0,	"CP1252"	},
	{	"English",	"Australia",	0x0C09,	1252,	0,	"CP1252"	},
	{	"English",	"Belize",	0x2809,	1252,	0,	"CP1252"	},
	{	"English",	"Canada",	0x1009,	1252,	0,	"CP1252"	},
	{	"English",	"Caribbean",	0x2409,	1252,	0,	"CP1252"	},
	{	"English",	"Ireland",	0x1809,	1252,	0,	"CP1252"	},
	{	"English",	"Jamaica",	0x2009,	1252,	0,	"CP1252"	},
	{	"English",	"New Zealand",	0x1409,	1252,	0,	"CP1252"	},
	{	"English",	"Phillippines",	0x3409,	1252,	0,	"CP1252"	},
	{	"English",	"South Africa",	0x1C09,	1252,	0,	"CP1252"	},
	{	"English",	"Trinidad",	0x2C09,	1252,	0,	"CP1252"	},
	{	"English",	"United Kingdom",	0x0809,	1252,	0,	"CP1252"	},
	{	"English",	"United States",	0x0409,	1252,	0,	"CP1252"	},
	{	"Estonian",	0,	0x0425,	1257,	186,	"CP1257"	},
	{	"FYRO Macedonian",	0,	0x042F,	1251,	0,		"CP1251"	},
	{	"Faroese",	0,	0x0438,	1252,	0,	"CP1252"	},
	{	"Farsi",	0,	0x0429,	1256,	178,	"CP1256"	},
	{	"Finnish",	0,	0x040B,	1252,	0,	"CP1252"	},
	{	"French",	"Belgium",	0x080C,	1252,	0,	"CP1252"	},
	{	"French",	"Canada",	0x0C0C,	1252,	0,	"CP1252"	},
	{	"French",	"France",	0x040C,	1252,	0,	"CP1252"	},
	{	"French",	"Luxembourg",	0x140C,	1252,	0,	"CP1252"	},
	{	"French",	"Switzerland",	0x100C,	1252,	0,	"CP1252"	},
	{	"German",	"Austria",	0x0C07,	1252,	0,	"CP1252"	},
	{	"German",	"Germany",	0x0407,	1252,	0,	"CP1252"	},
	{	"German",	"Liechtenstein",	0x1407,	1252,	0,	"CP1252"	},
	{	"German",	"Luxembourg",	0x1007,	1252,	0,	"CP1252"	},
	{	"German",	"Switzerland",	0x0807,	1252,	0,	"CP1252"	},
	{	"Greek",	0,	0x0408,	1253,	161,	"CP1253"	},
	{	"Hebrew",	0,	0x040D,	1255,	177,	"CP1255"	},
	{	"Hindi",	0,	0x0439,	0,	0,	"Latin1"	},
	{	"Hungarian",	0,	0x040E,	1250,	238,	"CP1250"	},
	{	"Icelandic",	0,	0x040F,	1252,	0,	"CP1252"	},
	{	"Indonesian",	0,	0x0421,	1252,	0,	"CP1252"	},
	{	"Italian",	"Italy",	0x0410,	1252,	0,	"CP1252"	},
	{	"Italian",	"Switzerland",	0x0810,	1252,	0,	"CP1252"	},
	{	"Japanese",	0,	0x0411,	932,	128,	"Shift-JIS"	},
	{	"Korean",	0,	0x0412,	949,	129,	"eucKR"	},
	{	"Latvian",	0,	0x0426,	1257,	186,	"CP1257"	},
	{	"Lithuanian",	0,	0x0427,	1257,	186,	"CP1257"	},
	{	"Malay",	"Brunei",	0x083E,	1252,	0,	"CP1252"	},
	{	"Malay",	"Malaysia",	0x043E,	1252,	0,	"CP1252"	},
	{	"Maltese",	0,	0x043A,	0,	0,	"Latin1"	},
	{	"Marathi",	0,	0x044E,	0,	0,	"Latin1"	},
	{	"Norwegian",	"Bokmal",	0x0414,	1252,	0,	"CP1252"	},
	{	"Norwegian",	"Nynorsk",	0x0814,	1252,	0,	"CP1252"	},
	{	"Polish",	0,	0x0415,	1250,	238,	"CP1250"	},
	{	"Portuguese",	"Brazil",	0x0416,	1252,	0,	"CP1252"	},
	{	"Portuguese",	"Portugal",	0x0816,	1252,	0,	"CP1252"	},
	{	"Romanian",	"Romania",	0x0418,	1250,	238,	"CP1250"	},
	{	"Russian",	0,	0x0419,	1251,	204,	"CP1251"	},
	{	"Sanskrit",	0,	0x044F,	0,	0,	"Latin1"	},
	{	"Serbian",	"Cyrillic",	0x0C1A,	1251,	0,		"CP1251"	},
	{	"Serbian",	"Latin",	0x081A,	1250,	238,	"CP1250"	},
	{	"Setsuana",	0,	0x0432,	1252,	0,	"CP1252"	},
	{	"Slovak",	0,	0x041B,	1250,	238,	"CP1250"	},
	{	"Slovenian",	0,	0x0424,	1250,	238,	"CP1250"	},
	{	"Spanish",	"Argentina",	0x2C0A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Bolivia",	0x400A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Chile",	0x340A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Colombia",	0x240A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Costa Rica",	0x140A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Dominican Republic",	0x1C0A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Ecuador",	0x300A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"El Salvador",	0x440A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Guatemala",	0x100A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Honduras",	0x480A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Mexico",	0x080A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Nicaragua",	0x4C0A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Panama",	0x180A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Paraguay",	0x3C0A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Peru",	0x280A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Puerto Rico",	0x500A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Spain",	0x0C0A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Uruguay",	0x380A,	1252,	0,	"CP1252"	},
	{	"Spanish",	"Venezuela",	0x200A,	1252,	0,	"CP1252"	},
	{	"Swahili",	0,	0x0441,	1252,	0,	"CP1252"	},
	{	"Swedish",	"Finland",	0x081D,	1252,	0,	"CP1252"	},
	{	"Swedish",	"Sweden",	0x041D,	1252,	0,	"CP1252"	},
	{	"Tamil",	0,	0x0449,	0,	0,	"TSCII"	},
	{	"Tatar",	0,	0x0444,	1251,	204,	"CP1251"	},
	{	"Thai",	0,	0x041E,	874,	222,	"TIS-620"	},
	{	"Turkish",	0,	0x041F,	1254,	162,	"CP1254"	},
	{	"Ukrainian",	0,	0x0422,	1251,	0,	"CP1251"	},
	{	"Urdu",	0,	0x0420,	1256,	178,	"CP1256"	},
	{	"Uzbek",	"Cyrillic",	0x0843,	1251,	0,	"CP1251"	},
	{	"Uzbek",	"Latin",	0x0443,	1254,	162,	"CP1254"	},
	{	"Vietnamese",	0,	0x042A,	1258,	163,	"CP1258"	},
	{	"Xhosa",	0,	0x0434,	1252,	0,	"CP1252"	},
	{	"Zulu",	0,	0x0435,	1252,	0,	"CP1252"	},
	{	0,		0,	0,		0,	0,	0	}
};


const KCHMTextEncoding::text_encoding_t * KCHMTextEncoding::getTextEncoding( )
{
	return text_encoding_table;
}

const KCHMTextEncoding::text_encoding_t * KCHMTextEncoding::lookupByLCID( short lcid )
{
	for ( const text_encoding_t * t = text_encoding_table; t->charset; t++ )
		if ( t->winlcid == lcid )
			return t;
			
	return 0;
}

const KCHMTextEncoding::text_encoding_t * KCHMTextEncoding::lookupByWinCharset( int charset )
{
	for ( const text_encoding_t * t = text_encoding_table; t->charset; t++ )
		if ( t->wincharset == charset )
			return t;
			
	return 0;
}


