/* fontmap.h: declarations for reading a file to define additional font names.

Copyright (C) 1993, 94, 95 Free Software Foundation, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef FONTMAP_H
#define FONTMAP_H

#include <kpathsea/c-proto.h>
#include <kpathsea/hash.h>
#include <kpathsea/types.h>


/* Look up KEY in all texfonts.map's in the glyph_format path, and
   return a null-terminated list of all matching entries, or NULL.  */
extern string *kpse_fontmap_lookup P1H(const_string key);

#endif /* not FONTMAP_H */
