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

#ifndef LIBCHMTEXTENCODING_H
#define LIBCHMTEXTENCODING_H


/*!
 * Represents a text encoding of CHM file; also has some useful routines.
 */
typedef struct
{
	const char	*	family;			//! Cyrillic, Western, Greek... NULL pointer represents the end of table.
	const char	*	qtcodec;		//! Qt text codec to use
	const short	*	lcids;			//! List of LCIDs to use for this codepage. Ends with LCID 0.
} LCHMTextEncoding;


#endif /* LIBCHMTEXTENCODING_H */
