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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

char *
_spectre_strdup (const char *str)
{
	size_t len;
	char *copy;

	if (!str)
		return NULL;

	len = strlen (str) + 1;

	copy = malloc (len);
	if (!copy)
		return NULL;

	memcpy (copy, str, len);

	return copy; 
}

#define TOLOWER(c) (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))
int
_spectre_strncasecmp (const char *s1,
		      const char *s2,
		      size_t      n)
{
	int c1, c2;

	while (n && *s1 && *s2)	{
		n -= 1;
		c1 = (int)(unsigned char) TOLOWER (*s1);
		c2 = (int)(unsigned char) TOLOWER (*s2);
		if (c1 != c2)
			return (c1 - c2);
		s1++;
		s2++;
	}

	return (n) ? (((int) (unsigned char) *s1) - ((int) (unsigned char) *s2)) : 0;
}

int
_spectre_strcasecmp (const char *s1,
		     const char *s2)
{
	int c1, c2;

	while (*s1 && *s2) {
		c1 = (int)(unsigned char) TOLOWER (*s1);
		c2 = (int)(unsigned char) TOLOWER (*s2);
		if (c1 != c2)
			return (c1 - c2);
		s1++;
		s2++;
	}

	return (((int)(unsigned char) *s1) - ((int)(unsigned char) *s2));
}
