/* readable.h: Is a file readable?

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

#ifndef KPATHSEA_READABLE_H
#define KPATHSEA_READABLE_H

#include <kpathsea/c-proto.h>
#include <kpathsea/types.h>

/* If NAME is readable and is a regular file, return it.  If the error is
   ENAMETOOLONG, truncate any too-long path components, and if the
   result is a readable file, return that.  Otherwise return NULL.  */
   
extern string kpse_readable_file P1H(const_string name);

#endif /* not KPATHSEA_READABLE_H */
