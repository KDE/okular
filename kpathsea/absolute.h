/* absolute.h: Declare absolute filename predicate.

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

#ifndef KPATHSEA_ABSOLUTE_H
#define KPATHSEA_ABSOLUTE_H

#include <kpathsea/types.h>
#include <kpathsea/c-proto.h>


/* True if FILENAME is absolute (/foo) or, if RELATIVE_OK is true,
   explicitly relative (./foo, ../foo), else false (foo).  */

extern boolean kpse_absolute_p P2H(const_string filename, boolean relative_ok);

#endif /* not KPATHSEA_ABSOLUTE_H */
