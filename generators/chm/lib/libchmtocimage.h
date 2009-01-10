
#ifndef LCHMTOCIMAGEKEEPER_H
#define LCHMTOCIMAGEKEEPER_H
//Added by qt3to4:
#include <QPixmap>
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

#include <QPixmap>

//! This class is used to retrieve the book TOC icons associated with images
class LCHMTocImageKeeper
{
	public:
		LCHMTocImageKeeper();
		const QPixmap * getImage( int id );
		
	private:
		QPixmap	m_images[LCHMBookIcons::MAX_BUILTIN_ICONS];
};
#endif
