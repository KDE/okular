/* default.h: Declare default path expander.

Copyright (C) 1993, 94 Karl Berry.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef KPATHSEA_DEFAULT_H
#define KPATHSEA_DEFAULT_H

#include <kpathsea/types.h>
#include <kpathsea/c-proto.h>


/* Replace a leading or trailing or doubled : in PATH with DFLT.  If
   no extra colons, return PATH.  Only one extra colon is replaced.
   DFLT may not be NULL.  */

extern string kpse_expand_default P2H(const_string path, const_string dflt);

#endif /* not KPATHSEA_DEFAULT_H */
