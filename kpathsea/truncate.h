/* truncate.h: truncate too-long components in a filename.

Copyright (C) 1993 Karl Berry.

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

#ifndef KPATHSEA_TRUNCATE_H
#define KPATHSEA_TRUNCATE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* Truncate any component in NAME longer than the system NAME_MAX, and
   return the result as a malloced string.  If none, return a copy of
   NAME.  */
extern string kpse_truncate_filename P1H(const_string name);

#endif /* not KPATHSEA_TRUNCATE_H */
