/* concatn.h: concatenate a variable number of strings.
   This is a separate include file only because I don't see the point of
   having every source file include <c-vararg.h>.  The declarations for
   the other concat routines are in lib.h.

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

#ifndef KPATHSEA_CONCATN_H
#define KPATHSEA_CONCATN_H

#include <kpathsea/c-proto.h>
#include <kpathsea/c-vararg.h>
#include <kpathsea/types.h>

/* Concatenate a null-terminated list of strings and return the result
   in malloc-allocated memory.  */
extern string concatn PVAR1H(const_string str1);

#endif /* not KPATHSEA_CONCATN_H */


