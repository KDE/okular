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

#ifndef SPECTRE_UTILS_H
#define SPECTRE_UTILS_H

#include <stdarg.h>

#include <libspectre/spectre-macros.h>

SPECTRE_BEGIN_DECLS

char *_spectre_strdup_printf (const char *format, ...);
char *_spectre_strdup        (const char *str);
int   _spectre_strncasecmp   (const char *s1,
			      const char *s2,
			      size_t      n);
int   _spectre_strcasecmp    (const char *s1,
			      const char *s2);

SPECTRE_END_DECLS

#endif /* SPECTRE_UTILS_H */

