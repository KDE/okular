/***************************************************************************
 *   Copyright (C) 2005 by tim   *
 *   tim@krasnogorsk.ru   *
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
#ifndef ICON_STORAGE_H
#define ICON_STORAGE_H

#include <qmap.h>
#include <qpixmap.h>

static const unsigned int MAX_BUILTIN_ICONS = 42;

class KCHMIconStorage
{
public:
	typedef struct
	{
		unsigned int 	size;
		const char * 	data;
	} png_memory_image_t;

	enum pixmap_index_t
	{
		back = 1000,
		bookmark_add,
		fileopen,
		print,
		findnext,
		findprev,
		forward,
		gohome,
		viewsource,
		view_decrease,
		view_increase
	};

	const QPixmap * getBookIconPixmap (unsigned int id);
	const QPixmap * getToolbarPixmap (pixmap_index_t pix);

private:
	const QPixmap * returnOrLoadImage (unsigned int id, const png_memory_image_t * image);

	QMap<unsigned int, QPixmap*>	m_iconMap;
};

extern KCHMIconStorage	gIconStorage;

#endif /* ICON_STORAGE_H */
