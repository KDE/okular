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

#include "libchmfileimpl.h"
#include "libchmtextencoding.h"

// Do not try to embed those in the text_encoding_table, it does not work - at least with gcc.
static short lcid_arabic[] =
{
	0x1401, 0x3C01, 0x0C01, 0x0801, 0x2C01, 0x3401, 0x3001, 0x1001,  
	0x1801, 0x2001,	0x4001, 0x0401, 0x2801, 0x1C01,	0x3801,	0x2401,
	0x0429, 0x0420,
	0,
};

static short lcid_baltic[] =
{
	0x0425,	0x0426,	0x0427, 0
};

static short lcid_centralEuropean[] =
{
	0x041C,	0x041A,	0x0405,	0x040E,	0x0415,	0x0418,	0x081A, 0x041B,	0x0424, 0
};

static short lcid_ChineseSimplifiedGB18030[] =
{ 
	0x0404, 0 
};


static short lcid_ChineseSimplifiedGBK[] =
{
	0x0804, 0	
};

	
static short lcid_ChineseSimplifiedGB2313[] =
{ 
	0x1004, 0 
};

static short lcid_ChineseTraditionalBig5[] =
{
	0x0C04, 0x1404, 0x0404,	0
};

static short lcid_ChineseTraditionalBigHKSCS[] =
{
	0x0404,	0
};

static short lcid_CyrillicCP1251[] =
{
	0x082C, 0x0423, 0x0402, 0x042F, 0x0419, 0x0C1A, 0x0444, 0x0422, 0x0843,	0
};

static short lcid_CyrillicKOI8R[] =
{
	0x7001, // artifical LCID
	0
};

static short lcid_CyrillicBrokenCPKOI[] =
{
	0x7002, // artifical LCID
		0
};

static short lcid_CyrillicBrokenKOICP[] =
{
	0x7003, // artifical LCID
	0
};


static short lcid_Greek[] =
{
	0x0408,	0
};

static short lcid_Hebrew[] =
{
	0x040D,	0
};

static short lcid_Japanese_eucJP[] =
{
	0x0411,	0
};


static short lcid_Japanese_JIS7[] =
{
	0x0411,	0
};

static short lcid_Japanese_ShiftJIS[] =
{
	0x0411,	0
};

static short lcid_Korean_eucKR[] =
{
	0x0412,	0
};

static short lcid_TamilTSCII[] =
{
	0x0449,	0
};

static short lcid_ThaiTIS[] =
{
	0x041E,	0
};

static short lcid_UkrainianKOI[] =
{
	0x7006,	0
};

static short lcid_Turkish[] =
{
	0x042C, 0x041F, 0x0443,	0
};

static short lcid_Vietnamese[] =
{
	0x042A,	0
};

static short lcid_UnicodeUTF8[] =
{
	0x7004, // artifical LCID
	0
};

static short lcid_UnicodeUTF16[] =
{
	0x7005, // artifical LCID
	0
};

static short lcid_Western[] =
{
	0x0436, 0x042D, 0x0403, 0x0406, 0x0813, 0x0413, 0x0C09, 0x2809,
	0x1009, 0x2409, 0x1809, 0x2009, 0x1409, 0x3409, 0x1C09, 0x2C09,
	0x0809, 0x0409, 0x0438, 0x040B, 0x080C, 0x0C0C, 0x040C, 0x140C, 
	0x100C, 0x0C07, 0x0407, 0x1407, 0x1007, 0x0807, 0x040F, 0x0421,
	0x0410, 0x0810, 0x083E, 0x043E, 0x0414, 0x0814, 0x0416, 0x0816,
	0x0432, 0x2C0A, 0x400A, 0x340A, 0x240A, 0x140A, 0x1C0A, 0x300A,
	0x440A, 0x100A, 0x480A, 0x080A, 0x4C0A, 0x180A, 0x3C0A, 0x280A,
	0x500A, 0x0C0A, 0x380A, 0x200A, 0x0441, 0x081D, 0x041D, 0x0434,
	0x0435, 0x042B, 0x042C, 0x0439, 0x043A, 0x044E, 0x044F, 0x081A,
	0x0443,
	0
};


static const LCHMTextEncoding text_encoding_table [] = 
{
	{	
		"Arabic",
		"CP1256",
		lcid_arabic
	},
	
	{	
		"Baltic",
		"CP1257",
		lcid_baltic
	},
		
	{	
		"Central European",		
		"CP1250",
		lcid_centralEuropean
	},
			
	{	
		"Chinese Simplified",	
		"GB18030",
		lcid_ChineseSimplifiedGB18030
	},	
		
	{	
		"Chinese Simplified",	
		"GBK",
		lcid_ChineseSimplifiedGBK
	},
		
	{	
		"Chinese Simplified",	
		"GB2313",
		lcid_ChineseSimplifiedGB2313
	},
		
	{	
		"Chinese Traditional",	
		"Big5",
		lcid_ChineseTraditionalBig5
	},
			
	{	
		"Chinese Traditional",	
		"Big5-HKSCS",
		lcid_ChineseTraditionalBigHKSCS
	},
		
	{	
		"Cyrillic",
		"CP1251",
		lcid_CyrillicCP1251
	},
		
	{	
		"Cyrillic",
		"KOI8-R",
		lcid_CyrillicKOI8R
	},
		
	{	
		"Cyrillic Broken",
		"CP1251/KOI8-R",
		lcid_CyrillicBrokenCPKOI
	},
		
	{	
		"Cyrillic Broken",
		"KOI8-R/CP1251",
		lcid_CyrillicBrokenKOICP
	},
		
	{	
		"Greek",
		"CP1253",
		lcid_Greek
	},
		
	{	
		"Hebrew",
		"CP1255",
		lcid_Hebrew
	},
		
	{	
		"Japanese",
		"eucJP",
		lcid_Japanese_eucJP
	},
		
	{	
		"Japanese",
		"JIS7",
		lcid_Japanese_JIS7
	},
		
	{	
		"Japanese",
		"Shift-JIS",
		lcid_Japanese_ShiftJIS
	},
		
	{	
		"Korean",
		"eucKR",
		lcid_Korean_eucKR
	},
		
	{	
		"Tamil",
		"TSCII",
		lcid_TamilTSCII
	},
		
	{	
		"Thai",
		"TIS-620",
		lcid_ThaiTIS
	},
		
	{	
		"Ukrainian",
		"KOI8-U",
		lcid_UkrainianKOI
	},
		
	{	
		"Turkish",
		"CP1254",
		lcid_Turkish
	},
		
	{	
		"Vietnamese",
		"CP1258",
		lcid_Vietnamese
	},
		
	{	
		"Unicode",
		"UTF-8",
		lcid_UnicodeUTF8
	},
		
	{	
		"Unicode",
		"UTF-16",
		lcid_UnicodeUTF16
	},
		
	{	
		"Western",
		"CP1252",
		lcid_Western
	},
		
	{	0,						0,					0	}
};


const LCHMTextEncoding * LCHMFileImpl::getTextEncodingTable()
{
	return text_encoding_table;
}

const LCHMTextEncoding * LCHMFileImpl::lookupByLCID( short lcid )
{
	for ( const LCHMTextEncoding * t = text_encoding_table; t->family; t++ )
	{
		for ( const short * lcids = t->lcids; *lcids; lcids++ )
			if ( *lcids == lcid )
				return t;
	}
	
	return 0;
}


int LCHMFileImpl::getEncodingIndex( const LCHMTextEncoding * enc)
{
	for ( int i = 0; text_encoding_table[i].family; i++ )
		if ( (text_encoding_table + i) == enc )
			return i;
			
	return -1;
}

const LCHMTextEncoding * LCHMFileImpl::lookupByQtCodec(const QString & codec)
{
	for ( const LCHMTextEncoding * t = text_encoding_table; t->family; t++ )
		if ( codec == t->qtcodec )
			return t;

	return 0;
}
