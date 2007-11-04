/* This file is part of Libspectre.
 * 
 * Copyright (C) 2007 Albert Astals Cid <aacid@kde.org>
 * Copyright (C) 2007 Carlos Garcia Campos <carlosgc@gnome.org>
 *
 * Libspectre is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Libspectre is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define _GNU_SOURCE
#include <stdio.h>

#include "spectre-utils.h"

static char *
spectre_strdup_vprintf (const char *format,
			va_list     args)
{
	char *string = NULL;
	int len = vasprintf (&string, format, args);
	
	if (len < 0)
		string = NULL;

	return string;
}

char *
_spectre_strdup_printf (const char *format, ...)
{
	char *buffer;
	va_list args;
	
	va_start (args, format);
	buffer = spectre_strdup_vprintf (format, args);
	va_end (args);
	
	return buffer;
}
